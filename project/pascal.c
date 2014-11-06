/*
			Maciej Szeszko,
			id: ms335814,
			University of Warsaw
*/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "common.h"
#include "constants.h"
#include "err.h"

void readNPrintData(int readDsc) {
	int structSize = sizeof(struct TriangleCeofficient);
	struct TriangleCeofficient dataMsg;

	int readDataMsg;
	int writeDataMsg;

	/* Read data portions till EOF will be detected. */
	while ((readDataMsg = read(readDsc, &dataMsg, structSize))) {
		/* Error occured. */
		if (readDataMsg == -1)
			syserr("Error while reading triangle results, Pascal\n");
		
		/* By default `structSize` bytes should be read. */
		if (readDataMsg != structSize)
			syserr("Illegal struct size while reading triangle results in Pascal\n");
		
		/* Print ceofficient to STDOUT. */
		writeDataMsg = write(STDOUT_FILENO, &(dataMsg.ceofficient), sizeof(int));
		
		/* Handle write message.. */
		switch (writeDataMsg) {
			case -1:
				syserr("Error while writing triangle results to STDOUT\n");
			case 0:
				syserr("Written 0 bytes to STODUT, Pascal\n");
			case sizeof(int):
				/* OK */
				break;
			default:
				/* Exactly `coeffSize` bytes should be written to STDOUT. */
				syserr("Written unexpected number of bytes to STDOUT, Pascal\n");
		}
	}
}

void prepareNextRequestMsg(enum Token mode, int workersLeft,
	struct RequestMsg* requestMsg) {
	requestMsg->token = mode;
	requestMsg->workersLeft = workersLeft;
	requestMsg->previousCeofficient = 1;
}

void readWorkerResponse(int readDsc, enum Token* mode,
	struct RequestMsg* requestMsg, int* workers, int maxWorkers) {
	struct ConfirmationMsg confirmationMsg;
	int confirmationMsgSize = sizeof(struct ConfirmationMsg);

	switch (*mode) {
		case init:
			readConfirmationSymbol(readDsc, &confirmationMsg, confirmationMsgSize);
			*mode = compute;
			prepareNextRequestMsg(compute, 1, requestMsg);
			break;
		case compute:
			readConfirmationSymbol(readDsc, &confirmationMsg, confirmationMsgSize);
			if (*workers == maxWorkers) {
				*mode = gatherResults;
				prepareNextRequestMsg(gatherResults, *workers, requestMsg);
			} else {
				prepareNextRequestMsg(compute, requestMsg->workersLeft + 1,
					requestMsg);
			}
			break;
		case gatherResults:
			readNPrintData(readDsc);
			*mode = waitAndClose;
			prepareNextRequestMsg(waitAndClose, *workers, requestMsg);
			break;
		default:
			/* Just to avoid stupid warnings. */
			break;
		/*case waitAndClose:
			readConfirmationSymbol(readDsc, confirmationMsg, confirmationMsgSize);
			break;*/
	}  /* switch(mode) */
}

void sendCloseMsg(int writeDsc, struct RequestMsg* requestMsg,
	int requestMsgSize) {
	if (write(writeDsc, &requestMsg, requestMsgSize) == -1)
		syserr("Error while sending close message to the Worker.\n");
	if (wait(NULL) == -1)
		syserr("Error in child process, PASCAL.\n");
}

int main(int argc, char* argv[]) {
	/* Usage info in case of invalid parameters number. */
	if (argc != 2)
		fatal("Usage: %s <number of row in Pascals' Triangle>\n", argv[0]);
	
	/* Eliminate requests with illegal row number. */
	int targetRow = atoi(argv[1]);
	if (targetRow <= 0)
		fatal("Row number should be expressed with positive value!\n");

	int maxWorkers = targetRow;
	int workers = maxWorkers;

	/* Create and initialize pipes. */
	int readPipe[2];
	int writePipe[2];
	
	initParentPipes(readPipe, writePipe);

	/* Set flags and constants. */
	enum Token mode = init;
	int readDsc = readPipe[0];
	int writeDsc = writePipe[1];

	/* Initialize first Pascals' request. */
	struct RequestMsg requestMsg;
	int requestMsgSize = sizeof(struct RequestMsg);
	prepareNextRequestMsg(init, workers, &requestMsg);

	switch (fork()) {
		case -1:
			syserr("Error in fork, Pascal\n");
		case 0:
			/* Prepare first Worker process. */
			reassignChildDescriptor(STDOUT_FILENO, readPipe);
			reassignChildDescriptor(STDIN_FILENO, writePipe);
			
			/* Close pipes in Worker process - free resources. */
			closeUnusedChildDescriptors(readPipe, writePipe);
			
			execl("./worker", "worker", argv[1], (char*) NULL);
			syserr("Error in execl\n");
		default:
			prepareParentPipes(readPipe, writePipe);
			while (1) {
				/* Write directly to the FIRST Worker process. */
				if (write(writeDsc, &requestMsg, requestMsgSize) == -1)
					syserr("Error while sending request message to initial Worker.\n");

				/* Read Worker response. */
				readWorkerResponse(readDsc, &mode, &requestMsg, &workers, maxWorkers);

				/* Wait for Worker process. */
				if (mode == waitAndClose) {
					sendCloseMsg(writeDsc, &requestMsg, requestMsgSize);
					break;
				}
			}
			closeParentDescriptors(readDsc, writeDsc);
	}  /* switch(fork()) */
	
	exit(EXIT_SUCCESS);
}

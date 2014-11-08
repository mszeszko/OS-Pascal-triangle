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
	
	/* Read data portions till EOF will be detected. */
	while (1) { 
		/* Error occured. */
		if (read(readDsc, &dataMsg, structSize) == -1)
			syserr("Error while reading triangle results, Pascal\n");
		
		/* Print ceofficient to STDOUT. */
		printf("%d\n", dataMsg.ceofficient);
		
		/* Finish reading when last message has been already processed. */
		if (dataMsg.isLast == LAST_ONE)
			break;
	}
}

void prepareNextRequestMsg(enum Token mode, int workersLeft,
	struct RequestMsg* requestMsg) {
	requestMsg->token = mode;
	requestMsg->workersLeft = workersLeft;
	requestMsg->previousCeofficient = 0;
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
			*workers = 1;
			break;
		case compute:
			readConfirmationSymbol(readDsc, &confirmationMsg, confirmationMsgSize);
			if (*workers == maxWorkers) {
				*mode = gatherResults;
				prepareNextRequestMsg(gatherResults, *workers, requestMsg);
			} else {
				prepareNextRequestMsg(compute, ++(*workers),
					requestMsg);
			}
			break;
		case gatherResults:
			readNPrintData(readDsc);
			*mode = waitAndClose;
			prepareNextRequestMsg(waitAndClose, *workers, requestMsg);
			break;
		default: /* case waitAndClose: */
			/* Just to avoid stupid warnings. */
			break;
	}  /* switch(mode) */
}

void sendCloseMsg(int writeDsc, struct RequestMsg* requestMsg,
	int requestMsgSize) {
	if (write(writeDsc, requestMsg, requestMsgSize) == -1)
		syserr("Error while sending close message to the Worker.\n");
	if (wait(NULL) == -1)
		syserr("Error in child process, PASCAL.\n");
}

int main(int argc, char* argv[]) {	
	/* Declare FIRST in order to not receive unexpected warning:
			
				ISO C90 forbids mixed declarations and code.
	*/
	int targetRow;
	int maxWorkers;
	int workers;
	int readPipe[2];
	int writePipe[2];
	enum Token mode;
	int readDsc;
	int writeDsc;
	struct RequestMsg requestMsg;
	int requestMsgSize;

	/* Usage info in case of invalid parameters number. */
	if (argc != 2)
		fatal("Usage: %s <number of row in Pascals' Triangle>\n", argv[0]);

	/* Eliminate requests with illegal row number. */
	targetRow = atoi(argv[1]);
	if (targetRow <= 0)
		fatal("Row number should be expressed with positive value!\n"); 

	maxWorkers = targetRow;
	workers = maxWorkers;

	/* Create and initialize pipes. */
	initParentPipes(readPipe, writePipe);

	/* Set flags and constants. */
	mode = init;
	readDsc = readPipe[0];
	writeDsc = writePipe[1];

	/* Initialize first Pascals' request. */
	requestMsgSize = sizeof(struct RequestMsg);
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

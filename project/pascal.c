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

void readNPrintData(int readDsc, int writeDsc, struct RequestMsg* requestMsg) {
	int structSize = sizeof(struct TriangleCeofficient);
	struct TriangleCeofficient dataMsg;
	
	if (write(writeDsc, requestMsg, sizeof(requestMsg)) == -1)
		syserr("Error while requesting coefficients in Pascal process.\n");

	/* Read data portions till EOF will be detected. */
	while (1) { 
		/* Error occured. */
		if (read(readDsc, &dataMsg, structSize) == -1)
			syserr("Error while reading triangle results, Pascal\n");
		
		/* Print ceofficient to STDOUT. */
		printf("%ld\n", dataMsg.ceofficient);
		
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

void initProcessList(enum Token* mode, int readDsc, int writeDsc, 
	struct RequestMsg* requestMsg) {
	struct ConfirmationMsg confirmationMsg;
	int confirmationMsgSize = sizeof(struct ConfirmationMsg);
	int requestMsgSize = sizeof(struct RequestMsg);

	if (write(writeDsc, requestMsg, requestMsgSize) == -1)
		syserr("Error while sending INIT message to FIRST Worker.\n");
	
	readConfirmationSymbol(readDsc, &confirmationMsg, confirmationMsgSize);
	*mode = compute;
	prepareNextRequestMsg(*mode, 1, requestMsg);
}

void computeCeofficients(enum Token* mode, int readDsc, int writeDsc,
	int maxWorkers, struct RequestMsg* requestMsg) {
	struct ConfirmationMsg confirmationMsg;
	int confirmationMsgSize = sizeof(struct ConfirmationMsg);
	int requestMsgSize = sizeof(struct RequestMsg);

	while (requestMsg->workersLeft <= maxWorkers) {
		/* Compute next ceofficient. */
		if (write(writeDsc, requestMsg, requestMsgSize) == -1)
			syserr("Error while sending COMPUTE message to FIRST Worker.\n");

		/* Increase the number of Workers involved in computation process. */
		requestMsg->workersLeft++;
	}

	/* After sending requests wait for all processes finished their tasks. */
	readConfirmationSymbol(readDsc, &confirmationMsg, confirmationMsgSize);
	*mode = gatherResults;
	prepareNextRequestMsg(*mode, maxWorkers, requestMsg);
}

void gatherCeofficients(enum Token* mode, int readDsc, int writeDsc, 
	int maxWorkers, struct RequestMsg* requestMsg) {
	readNPrintData(readDsc, writeDsc, requestMsg);
	*mode = waitAndClose;
	prepareNextRequestMsg(*mode, maxWorkers, requestMsg);
}

void handleOperation(int readDsc, int writeDsc, enum Token* mode,
	struct RequestMsg* requestMsg, int maxWorkers) {

	switch (*mode) {
		case init:
			initProcessList(mode, readDsc, writeDsc, requestMsg);
			break;
		case compute:
			computeCeofficients(mode, readDsc, writeDsc, maxWorkers, requestMsg);
			break;
		case gatherResults:
			gatherCeofficients(mode, readDsc, writeDsc, maxWorkers, requestMsg);
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

	/* Create and initialize pipes. */
	initParentPipes(readPipe, writePipe);

	/* Set flags and constants. */
	mode = init;
	readDsc = readPipe[0];
	writeDsc = writePipe[1];

	/* Initialize first Pascals' request. */
	requestMsgSize = sizeof(struct RequestMsg);
	prepareNextRequestMsg(init, maxWorkers, &requestMsg);

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
				handleOperation(readDsc, writeDsc, &mode, &requestMsg, maxWorkers);

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

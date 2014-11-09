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

void initTriangleCeofficient(struct TriangleCeofficient* triangleCeofficient,
	unsigned long ceofficient, char isLastSymbol) {
	triangleCeofficient->ceofficient = ceofficient;
	triangleCeofficient->isLast = isLastSymbol;
}

void computeCeofficients(unsigned long* actualCeofficient, int processNumber,
	int readDsc, int writeDsc, struct RequestMsg* requestMsg)	{
	/* Used for assigning temporary ceofficient value. */
	unsigned long previousParentCeofficient;
	struct ConfirmationMsg confirmationMsg;
	int requestMsgSize = sizeof(struct RequestMsg);
	int confirmationMsgSize = sizeof(struct ConfirmationMsg);
	int maxMessagesToBeProcessed = processNumber;
	int processedMessages = 0;

	while (++processedMessages <= maxMessagesToBeProcessed) {		
		/* Save previous parent ceofficient value. */
		previousParentCeofficient = requestMsg->previousCeofficient;

		/* Prepare message for the child. */
		requestMsg->previousCeofficient = *actualCeofficient;
		
		if (--requestMsg->workersLeft > 0) {
			/* Forward modified message to the child process. */
			if (write(writeDsc, requestMsg, requestMsgSize) == -1)
				syserr("Error while forwarding COMPUTE request to the child.\n");
		}

		if (processedMessages != maxMessagesToBeProcessed)
			/* Read message from parent process. */
			if (read(STDIN_FILENO, requestMsg, requestMsgSize) == -1)
				syserr("Error while reading COMPUTE request from parent process.\n");

		/* Actualize new ceofficient value. */
		if (processedMessages > 1)
			*actualCeofficient += previousParentCeofficient;
		else
			*actualCeofficient = 1;
	}
	
	/* If we are not the last process, then wait for confirmation symbol
		 from descendant.	*/
	if (processNumber > 1) {
		readConfirmationSymbol(readDsc, &confirmationMsg, confirmationMsgSize);	
	}

	confirmationMsg.result = SUCCESS;
	/* Pass confirmation message to the parent process. */
	if (write(STDOUT_FILENO, &confirmationMsg, confirmationMsgSize) == -1)
		syserr("Error while sending confirmation symbol to the parent process.\n");
}

void gatherCeofficients(unsigned long actualCeofficient, int processNumber,
	int readDsc, int writeDsc, struct RequestMsg* requestMsg) {
	struct TriangleCeofficient triangleCeofficient;
	char isLastSymbol;
	int triangleCeofficientSize = sizeof(struct TriangleCeofficient);
	int requestMsgSize = sizeof(struct RequestMsg);
	
	/* If we are the last process in the list, then pass computed ceofficient
		 to the parent process. */
	if (processNumber == 1) {
		/* Prepare ceofficient package with `LAST_ONE` label. */
		isLastSymbol = LAST_ONE;
	} else {
		/* Since I'm not the last Worker process in the list, mark as `NOT_LAST` */
		isLastSymbol = NOT_LAST;
	}
	
	/* Init current triangle ceofficient. */
	initTriangleCeofficient(&triangleCeofficient, actualCeofficient,
		isLastSymbol);
	
	/* Send appropriately prepared package to the parent process. */
	if (write(STDOUT_FILENO, &triangleCeofficient,
		triangleCeofficientSize) == -1)
		syserr("Error while sending ceofficient to the parent process.\n");
	
	if (processNumber > 1)
		/* Forward modified message to the child process. */
		if (write(writeDsc, requestMsg, requestMsgSize) == -1)
			syserr("Error while forwarding GATHER message to the child process.\n");

	/* Handle passing messages till the package labeled `LAST_ONE` come out. */ 
	if (processNumber > 1) {
		while (1) {
			/* Read next ceofficient package */
			if (read(readDsc, &triangleCeofficient, triangleCeofficientSize) == -1)
				syserr("Error while reading next ceofficient package.\n");
			
			/* Despite `isLast` value forward the package to the parent process. */
			if (write(STDOUT_FILENO, &triangleCeofficient, triangleCeofficientSize) == -1)
				syserr("Error while forwarding computed coefficient to the parent process.\n");
		
			/* If current ceofficient happend to be the last one, stop reading. */
			if (triangleCeofficient.isLast == LAST_ONE)
				break;
		}
	}
}

int main(int argc, char* argv[]) {
	/* Declare FIRST in order to not receive unexpected warning:
			
				ISO C90 forbids mixed declarations and code.
	*/
	int processNumber;
	int readPipe[2];
	int writePipe[2];
	int readDsc;
	int writeDsc;
	struct RequestMsg requestMsg;
	int requestMsgSize;
	struct ConfirmationMsg confirmationMsg;
	int confirmationMsgSize;
	unsigned long actualCeofficient;
	char childProcessNumberStr[PROC_NUMBER_BUF_SIZE];
	int childProcessNumber;

	/* Usage info in case of invalid parameters number. */
	if (argc != 2)
		fatal("Usage: %s <process number>\n", argv[0]);
	
	/* Eliminate requests with illegal row number. */
	processNumber = atoi(argv[1]);
	if (processNumber <= 0)
		fatal("Row number should be expressed with positive value!\n");

	/* Create and initialize pipes. */
	initParentPipes(readPipe, writePipe);
	
	/* Assign valid descriptors. */
	readDsc = readPipe[0];
	writeDsc = writePipe[1];

	/* Set structures size. */
	requestMsgSize = sizeof(struct RequestMsg);
	
	confirmationMsgSize = sizeof(struct ConfirmationMsg);
	confirmationMsg.result = SUCCESS;
	
	/* Initialize current ceofficient value. */
	actualCeofficient = 0;

	/* Assign child process number. */
	childProcessNumber = processNumber - 1;
	
	while (1) {
		/* Read message from predecessor. */
		if (read(STDIN_FILENO, &requestMsg, requestMsgSize) == -1)
			syserr("Error in process: %d, while reading message.\n", processNumber);
		
		/* Process message for specified token. */
		switch (requestMsg.token) {
			case init:
				/* If not the last process to be initialized in the list.. */
				if (processNumber > 1) {
					switch (fork()) {
						case -1:
							syserr("Error in fork, process: %d\n", processNumber);
						case 0:
							/* Prepare next Worker process. */
							reassignChildDescriptor(STDOUT_FILENO, readPipe);
							reassignChildDescriptor(STDIN_FILENO, writePipe);
							
							/* Close unused pipes. */
							closeUnusedChildDescriptors(readPipe, writePipe);
							
							sprintf(childProcessNumberStr, "%d", childProcessNumber);
							execl("./worker", "worker", &childProcessNumberStr, (char*)NULL);
							syserr("Error in exec, process: %d.\n", childProcessNumber);
						default:
							/* Write message to the child. */
							if (write(writeDsc, &requestMsg, requestMsgSize) == -1)
								syserr("Error while forwarding init message to the child.\n");
							
							/* Read child response. */
							readConfirmationSymbol(readDsc, &confirmationMsg,
								confirmationMsgSize);

							/* Forward successful init message to parent process. */
							if (write(STDOUT_FILENO, &confirmationMsg, confirmationMsgSize) == -1)
								syserr("Error while forwarding message to the parent.\n");
					}
				} else {
					/* Forward successful init message to parent process. */
					if (write(STDOUT_FILENO, &confirmationMsg, confirmationMsgSize) == -1)
						syserr("Error while forwarding message to the parent.\n");
				}
				break;
			case compute:
				computeCeofficients(&actualCeofficient, processNumber, readDsc,
					writeDsc, &requestMsg);
				break;
			case gatherResults:
				gatherCeofficients(actualCeofficient, processNumber, readDsc,
					writeDsc, &requestMsg);
				break;
			case waitAndClose:
				/* Since I'm not the last worker process in the list.. */
				if (requestMsg.workersLeft > 1) {
					/* Decrease number of engaged workers on particular level. */
					requestMsg.workersLeft--;

					/* Forward closing alert to the child. */
					if (write(writeDsc, &requestMsg, requestMsgSize) == -1)
						syserr("Error while writing `close` message to the child.\n");
					
					/* Wait for the child. */
					if (wait(NULL) == -1)
						syserr("Error in wait, WORKER.\n");
					
					/* Free resources. By parent I mean current process. */
					closeParentDescriptors(readDsc, writeDsc);
				}
				closeOverridenStandardDescriptors();
				
				/* Finish work. */
				exit(EXIT_SUCCESS);
		}
	}
}

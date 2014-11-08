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

void readChildTriangleCeofficient(int readDsc, struct TriangleCeofficient*
	triangleCeofficient, int triangleCeofficientSize) {
	int readBytesCounter = read(readDsc, triangleCeofficient,
		triangleCeofficientSize);

	if (readBytesCounter == -1)
		syserr("Error while reading child triangle ceofficient.\n");
	if (readBytesCounter != triangleCeofficientSize)
		syserr("Expected exactly `triangleCeofficientSize` bytes.\n");
}

void writeTriangleCeofficientToParent(int writeDsc, struct TriangleCeofficient*
	triangleCeofficient, int triangleCeofficientSize) {
	int writeBytesCounter = write(writeDsc, triangleCeofficient,
		triangleCeofficientSize);

	if (writeBytesCounter == -1)
		syserr("Error while writing triangle ceofficient to the parent.\n");
	if (writeBytesCounter != triangleCeofficientSize)
		syserr("Expected exactly `triangleCeofficientSize` bytes.\n");
}

/* Returns 1 if given triangleCeofficient structure contains
	 the last of all descendant ceofficients and set `isLast`
	 value to NOT_LAST, otherwise function returns 0. */
int actualizeIfLast(struct TriangleCeofficient* triangleCeofficient) {
	if (triangleCeofficient->isLast == LAST_ONE) {
		triangleCeofficient->isLast = NOT_LAST;
		return 1;
	}
	return 0;
}

void initTriangleCeofficient(struct TriangleCeofficient* triangleCeofficient,
	int ceofficient) {
	triangleCeofficient->ceofficient = ceofficient;
	triangleCeofficient->isLast = LAST_ONE;
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
	struct TriangleCeofficient triangleCeofficient;
	int triangleCeofficientSize;
	int actualCeofficient;
	int previousCeofficient; /* Used for assigning temporary ceofficient value. */
	int readLastCeofficient;
	enum WorkerMode mode;
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

	triangleCeofficientSize = sizeof(struct TriangleCeofficient);
	
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
				/* Actualize process mode. */
				mode = ready;
				break;
			case compute:
				/* If I'm the last one in current row.. */
				if (mode == ready) {
					actualCeofficient = 1;
					/* Change mode. */
					mode = computing;
				} else {
					/* Save previous parent rows' ceofficient value. */
					previousCeofficient = requestMsg.previousCeofficient;
					
					/* Prepare request message for the child. */
					requestMsg.workersLeft--;
					requestMsg.previousCeofficient = actualCeofficient;
					
					/* Forward modified message to the child process. */
					if (write(writeDsc, &requestMsg, requestMsgSize) == -1)
						syserr("Error while forwarding compute message to the child.\n");
					
					/* Actualize new ceofficient value. */
					actualCeofficient += previousCeofficient;

					/* Read child response. */
					readConfirmationSymbol(readDsc, &confirmationMsg,
						confirmationMsgSize);
				}
				/* Send confirmation message. */
				if (write(STDOUT_FILENO, &confirmationMsg, confirmationMsgSize) == -1)
					syserr("Error while writing confirmation message to the parent.\n");
				break;
			case gatherResults:
				/* Since I'm not the last worker process in the list.. */
				if (requestMsg.workersLeft > 1) {
					/* Forward gathering alert to the child, */
					requestMsg.workersLeft--;
					if (write(writeDsc, &requestMsg, requestMsgSize) == -1)
						syserr("Error while passing gather message to the child.\n");
					
					/* Read descendants ceofficients in order:
						 <- [furthermost descendant], ...., [son] */
					while (1) {
						/* Read next ceofficient.. */
						readChildTriangleCeofficient(readDsc, &triangleCeofficient,
							triangleCeofficientSize);
						
						/* Based on this value, it is possible to determine
							 whether we processed the last descendants ceofficient
							 (and it's highest time to push our value) or not. */
						readLastCeofficient = actualizeIfLast(&triangleCeofficient);

						/* Anyway, we have to post the request. */
						writeTriangleCeofficientToParent(STDOUT_FILENO,
							&triangleCeofficient, triangleCeofficientSize);

						/* Break the loop if last descendant ceofficient has ben processed.*/
						if (readLastCeofficient)
							break;
					}
				}
				/* Update mode. */
				mode = gatherResults;
				
				/* After processing all descendant ceofficients, prepare 
					 your own request message and send it to your direct ancestor.
					 Remember to notify that it's actually the last one to be fetched! */
				initTriangleCeofficient(&triangleCeofficient, actualCeofficient);
	
				/* Pass actual ceofficient to father process. */
				if (write(STDOUT_FILENO, &triangleCeofficient, triangleCeofficientSize) == -1)
					syserr("Error while passing actual ceofficient to the parent.\n");
				break;
			default: /* case waitAndClose:*/
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

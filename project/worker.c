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
	if (readBytesCounter != triangleCefficientSize)
		syserr("Expected exactly `triangleCeofficientSize` bytes.\n");
}

void writeTriangleCeofficientToParent(int writeDsc, struct TriangleCeofficient*
	triangleCeofficient, int triangleCeofficientSize) {
	int writeBytesCounter = write(writeDsc, triangleCeofficient,
		triangleDeofficientSize);

	if (writeBytesCounter == -1)
		syserr("Error while writing triangle ceofficient to the parent.\n");
	if (writeBytesCounter != triangleCefficientSize)
		syserr("Expected exactly `triangleCeofficientSize` bytes.\n");
}

// Returns 1 if given triangleCeofficient structure contains
// the last of all descendant ceofficients and set `isLast`
// value to NOT_LAST, otherwise function returns 0.
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
	// Usage info in case of invalid parameters number.
	if (argc != 2)
		fatal("Usage: %s <process number>\n", argv[0]);
	
	// Eliminate requests with illegal row number.
	int processNumber = atoi(argv[1]);
	if (processNumber <= 0)
		fatal("Row number should be expressed with positive value!\n");

	// Create and initialize pipes.
	int readPipe[2];
	int writePipe[2];

	initParentPipes(&readPipe, &writePipe);
	
	int readDsc = readPipe[0];
	int writeDsc = writePipe[1];

	// Core variables.
	struct RequestMsg requestMsg;
	int requestMsgSize = sizeof(struct RequestMsg);
	
	struct ConfirmationMsg confirmationMsg;
	int confirmationMsgSize = sizeof(struct ConfirmationMsg);
	confirmationMsg.result = SUCCESS;

	struct TriangleCeofficient triangleCeofficient;
	int triangleCeofficientSize = sizeof(struct TriangleCeofficient);
	
	int actualCeofficient = 0;
	int previousCeofficient; // Used for assigning temporary ceofficient value.
	int isLastCeofficient;

	enum WorkerMode mode;
	char childResponse;
	char childProcessNumberStr[PROC_NUMBER_BUF_SIZE];
	int childProcessNumber = processNumber - 1;
	
	while (true) {
		// Read message from predecessor.
		if (read(STDIN_FILENO, &requestMsg, requestMsgSize) == -1)
			syserr("Error in process: %d, while reading message.\n", processNumber);

		// Process message for specified token.
		switch (requestMsg->token) {
			case init:
				// If not the last process to be initialized in the list..
				if (processNumber > 1) {
					switch (fork()) {
						case -1:
							syserr("Error in fork, process: %d\n", processNumber);
						case 0:
							// Prepare next Worker process.
							reassignChildDescriptor(STDOUT_FILENO, &readPipe);
							reassignChildDescriptor(STDIN_FILENO, &writePipe);
							
							// Close unused pipes.
							closeUnusedChildDescriptors(&readPipe, &writePipe);
							
							sprintf(childProcessNumberStr, "%d", childProcessNumber);
							execl("./worker", "worker", &childProcessNumberStr,
								PROC_NUMBER_BUF_SIZE);
							syserr("Error in exec, process: %d.\n", childProcessNumber);
						default:
							// Write message to the child
							if (write(writeDsc, &requestMsg, requestMsgSize) == -1)
								syserr("Error while forwarding init message to the child.\n");
							
							// Read child response.
							readConfirmationSymbol(readDsc);

							// Forward successful init message to parent process.
							if (write(STDOUT, &confirmationMsg, confirmationMsgSize) == -1)
								syserr("Error while forwarding message to the parent.\n");
					}
				} else {
					// Forward successful init message to parent process.
					if (write(STDOUT, &confirmationMsg, confirmationMsgSize) == -1)
						syserr("Error while forwarding message to the parent.\n");
				}
				// Actualize process mode.
				mode = ready;
				break;
			case compute:
				// If I'm the last one in current row..
				if (mode == ready) {
					actualCeofficient = 1;
					// Change mode.
					mode = compute;
				} else {
					// Actualize ceofficient value.
					previousCeofficient = actualCeofficient;
					actualCeofficient += requestMsg.previousCeofficient;
					
					// Prepare request message for the child.
					requestMsg.workersLeft--;
					requestMsg.previousCeofficient = previousCeofficient;
					
					// Forward modified message to the child process.
					if (write(writeDsc, &requestMsg, requestMsgSize) == -1)
						syserr("Error while forwarding compute message to the child.\n");
					
					// Read child response.
					readConfirmationSymbol(readDsc);
				}
				// Send confirmation message.
				if (write(STDOUT, &confirmationMsg, confirmationMsgSize) == -1)
					syserr("Error while writing confirmation message to the parent.\n");
				break;
			case gatherResults:
				// Since I'm not the last worker process in the list..
				if (requestMsg.workersLeft > 1) {
					// Forward gathering alert to the child,
					if (write(writeDsc, &requestMsg, requestMsgSize) == -1)
						syserr("Error while passing gather message to the child.\n");
					
					// Read descendants ceofficients in order:
					// <- [furthermost descendant], ...., [son]
					while (true) {
						// Read next ceofficient..
						readChildTriangleCeofficient(readDsc, &triangleCeofficient,
							triangleCeofficientSize);
						
						// Based on this value, it is possible to determine
						// whether we processed the last descendants ceofficient
						// (and it's highest time to push our value) or not.
						readLastCeofficient = actualizeIfLast(&triangleCeofficient);

						// Anyway, we have to post the request.
						writeTriangleCeofficientToParent(STDOUT_FILENO,
							&triangleCeofficient, triangleCeofficientSize);

						// Break the loop if last descendant ceofficient has ben processed.
						if (readLastCeofficient)
							break;
					}
				}
				// Update mode.
				mode = gatherResults;

				// After processing all descendant ceofficients, prepare 
				// your own request message and send it to your direct ancestor.
				// Remember to notify that it's actually the last one to be fetched!
				initTriangleCeofficient(&triangleCeofficient, actualCeofficient);
	
				// Pass actual ceofficient to father process.
				if (write(STDOUT, &triangleCeofficient, triangleCeofficientSize) == -1)
					syserr("Error while passing actual ceofficient to the parent.\n");
				break;
			case waitAndClose:
				// Since I'm not the last worker process in the list..
				if (requestMsg.workersLeft > 1) {
					// Forward closing alert to the child.
					if (write(writeDsc, &requestMsg, requestMsgSize) == -1)
						syserr("Error while writing `close` message to the child.\n");
					
					// Wait for the child.
					if (wait(NULL) == -1)
						syserr("Error in wait, WORKER.\n");
					
					// Free resources. By parent I mean current process.
					closeParentDescriptors(readDsc, writeDsc);
				}
				closeOverridenStandardDescriptors();
				
				// Finish work.
				exit(EXIT_SUCCESS);
		}
	}
}

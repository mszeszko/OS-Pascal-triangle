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
	confirmationMsg.result = SUCCESS;
	int confirmationMsgSize = sizeof(struct ConfirmationMsg);

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
							
							sprintf(childProcessNumber, "%d", childProcessNumber);
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
								syserr("Error while forwarding message to the parent\n");
					}
				} else {
					// Forward successful init message to parent process.
					if (write(STDOUT, &confirmationMsg, confirmationMsgSize) == -1)
						syserr("Error while forwarding message to the parent\n");
				}
				// Actualize process mode.
				mode = ready;
				break;
			case compute:
		}
	}
	exit(EXIT_SUCCESS);
}

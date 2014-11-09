/*
			Maciej Szeszko,
			id: ms335814,
			University of Warsaw
*/

#ifndef __CONSTANTS_H_
#define __CONSTANTS_H_

enum WorkerMode {
	ready = 0, /* Created and initialized, but not active yet. */
	computing = 1, /* Worker about to compute the coefficient. */
	sending = 2, /* Send processed data to the predecessor. */
	closing = 3 /* Wait for child process, free allocated resources and exit. */
};

enum Token {
	init = 0,
	compute = 1,
	gatherResults = 2,
	waitAndClose = 3
};

/* Success message: */
#define SUCCESS '#'
#define LAST_ONE 'Y'
#define NOT_LAST 'N'

struct ConfirmationMsg {
	/* After accomplishing speficic request job,
		 process is obliged(excluding some extraordinary cases like data transfer)
		 to return confirmation message to the predecessor. */
	char result;
};

/* 'Y' stands for "Yes, I'm the last one.". */
struct TriangleCeofficient {
	unsigned long int ceofficient;
	char isLast;
};

struct RequestMsg {
	enum Token token;
	int workersLeft; /* Number of workers required to complete the current row. */
	/* Appropriate ceofficient from predecessing row. */
	unsigned long int previousCeofficient;
};

/* Size of process number represented as a string. */
#define PROC_NUMBER_BUF_SIZE 17

#endif

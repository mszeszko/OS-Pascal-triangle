/*
			Maciej Szeszko,
			id: ms335814,
			University of Warsaw
*/

enum WorkerMode {
	ready = 0 // Created and initialized, but not active yet.
	compute, // 
	send, // Send processed data to the predecessor.
	closed // Wait for child process, free allocated resources and exit.
};

enum PascalMode {
	initChain = 10, // Chain of worker processes still does not exist.
	commitComputation, //
	gatherResults, //
	waitAndClose // --------||--------
};

enum Token {
	init = 20, //
	compute, // 
	gatherResults, //
	waitAndClose
};

struct PascalMsg {
	int token = 40
};

struct ProcessConfirmMsg {
	// After accomplishing request for speficic token,
	// process is obliged to return confirmation message to the predecessor.
	int msg = 50
};

struct TriangleCeofficient {
	int ceofficient,
	int isLast
};

struct ProcessMsg {
	int token; // One of tokens from Token enum.
	int workersLeft, // Number of workers required for complete current row.
};

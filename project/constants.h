/*
			Maciej Szeszko,
			id: ms335814
			University of Warsaw
*/

enum WorkerMode {
	ready = 0 // Created and initialized, but not active yet.
	compute, // 
	sent, // Sent processed data to the predecessor.
	closed // Wait for child process, free allocated resources and exit.
};

enum PascalMode {
	initChain = 10, // Chain of worker processes still does not exist.
	commitComputation, //
	gatherResults, //
	waitAndClose // --------||--------
};

enum Token {
	compute = 20, // 
	gatherResults, //
	waitAndClose
};

enum Action {
	perform = 30,
	done
};

struct PascalMsg {
	int token;
};

struct ProcessMsg {
	// Message exchanged between Workers.
	// After accomplishing request for speficic token,
	// process is obliged to return action parameter set to value of `done`.
  int action; // `done` or `perform` from `Action` enum.
	int token; // One of tokens from Token enum.
	int workersLeft; // Number of workers required for complete current row.
};

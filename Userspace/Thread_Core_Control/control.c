
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <sched.h>

#define MAX_CORES 4

char* exec(char* cmd);

/**
 * This function gets a PROCESS_ID, a CORE_NUMBER  and the PROCESS_SHARE from the SLA
 * as an input and sets the scheduling policy to be the VMS_SCHEDULER.
 * .
 **/
int main (int argc, char *argv[]) {

	if (argc != 4) {
		fprintf(stderr, "Please specify the three inputs PROCESS_ID, CORE_NUMBER and the desired PROCESS_SHARE\nFor example: \"control 12345 4 50\"\nwhereas '12345' is the process id, '4' the core (core number starts with 0) and 50 the share.");
		exit(0);
	}
 
	int pid;
	int core;
	int share;

	pid = atoi(argv[1]);

	core = atoi(argv[2]);

	share = atoi(argv[3]);

	if (core < 0 | core >= MAX_CORES) {
		perror("Please specify a CORE_NUMBER from 0 to 3!");
		exit(0);
	}

	if (pid < 0) {
		perror("Please specify a valid PROCESS_ID.");
		exit(0);
	}
		
	if (share < 0) {
		perror("Please specify a valid PROCESS_SHARE.");
		exit(0);
	}

	const struct sched_param *param;

#ifdef SCHED_VMS
	if (sched_setscheduler(pid, SCHED_VMS, param) == -1) { /* Set scheduling policy for the process . Returns -1 on error and sets errno accordingly */
		perror("[ERROR]'sched_setscheduler' returned an error");	
	}
#else
	fprintf(stderr, "Please specify the scheduling policy for the virtual machines inside the \"sched.h\".\n");
#endif
	return 0;
}

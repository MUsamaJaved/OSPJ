/*************************************************************************\
*                  Copyright (C) Michael Kerrisk, 2014.                   *
*                                                                         *
* This program is free software. You may use, modify, and redistribute it *
* under the terms of the GNU Affero General Public License as published   *
* by the Free Software Foundation, either version 3 or (at your option)   *
* any later version. This program is distributed without any warranty.    *
* See the file COPYING.agpl-v3 for details.                               *
\*************************************************************************/

/* Solution for Exercise 35-3 */

/* demo_sched_fifo.c - No! Demo sched VMS

This program demonstrates the use of realtime scheduling policies. It creates
two processes, each running under the SCHED_FIFO scheduling policy. Each
process executes a function that prints a message every quarter of a second
of CPU time. After each second of consumed CPU time, the function and calls
sched_yield() to yield the CPU to the other process. Once a process has
consumed 3 seconds of CPU time, the function terminates.

This program must be run as superuser, or (on Linux 2.6.12 and later)
with a suitable RLIMIT_RTPRIO resource limit.
*/
#define _GNU_SOURCE
#include <sched.h>
#include <sys/resource.h>
#include <sys/times.h>
#include "../lib/tlpi_hdr.h"
#include <stdbool.h>
#define CSEC_STEP 10           /* CPU centiseconds between messages */
#define CSEC_RUN 100
#define CSEC_YIELD 25

char *names[] = {
	"SCHED_OTHER",
	"SCHED_FIFO",
	"SCHED_RR",
	"SCHED_BATCH",
	"SCHED_ISO", // reserved but not implemented
	"SCHED_IDLE",
	"SCHED_VMS",
};

static void debug_scheduler(const char* msg) {
	int pid_sched = sched_getscheduler(0);

	fprintf(stdout, "Retrieved scheduler value %s: 0x%X (-%d)\n", msg, pid_sched, -pid_sched);

	if (pid_sched == -EINVAL) {
		//invalid pid
		fprintf(stderr, "Failed to retrieve scheduling policy, pid is invalid\n\n");
		return;
	} else if (pid_sched == -ESRCH || pid_sched == -1) {
		//no such process
		//-1 is a hack, found manually
		fprintf(stderr, "Failed to retrieve scheduling policy, there is no process with such pid \n\n");
		return;
	} else if (pid_sched == -EFAULT) {
		//no such process
		fprintf(stderr, "Failed to retrieve scheduling policy, EFAULT was returned\n\n");
		return;
	} 

	fprintf(stdout, "Current scheduling policy %s is %s\n", msg, names[pid_sched & 0x7]);
#ifdef SCHED_VMS
	fprintf(stdout, "VMS share is %d\n", pid_sched >> 4);
#endif
	return;
}

static void useCPU(char *msg, int id)
{
	printf("Entering useCPU\n");
	struct tms tms;
	int cpuCentisecs, prevStep, prevSec;

	prevStep = 0;
	prevSec = 0;
	for (;;) {
		if (times(&tms) == -1)
			errExit("times");
		cpuCentisecs = (tms.tms_utime + tms.tms_stime) * 100 /
						sysconf(_SC_CLK_TCK);

		if (cpuCentisecs >= prevStep + CSEC_STEP) {
			prevStep += CSEC_STEP;
			printf("%s%02d (PID %ld) cpu=%0.2f\n", msg, id, (long) getpid(),
							cpuCentisecs / 100.0);
		}

		if (cpuCentisecs > CSEC_RUN)    
			/* Terminate after 10 seconds */
			break;

		if (cpuCentisecs >= prevSec + CSEC_YIELD) {    
			/* Yield once/second */
			prevSec = cpuCentisecs;
			sched_yield();
		}
	}
}

int
main(int argc, char *argv[])
{
//	struct rlimit rlim;
	struct sched_param sp;
	cpu_set_t set;
	bool vms = false;
	setbuf(stdout, NULL);               /* Disable buffering of stdout */

	if(argc > 1 && strcmp(argv[1], "-vms") == 0)
	{
		vms = true;
	}

	printf("Running with %s scheduler\n", vms ? "VMS" : "RT");

	/* Confine all processes to a single CPU, so that the processes
		 won't run in parallel on multi-CPU systems. */

	CPU_ZERO(&set);
	CPU_SET(0, &set);

	if (sched_setaffinity(getpid(), sizeof(set), &set) == -1)
			errExit("sched_setaffinity");

	/* Establish a CPU time limit. This demonstrates how we can
	ensure that a runaway realtime process is terminated if we
	make a programming error. The resource limit is inherited
	by the child created using fork().

	An alternative technique would be to make an alarm() call in each
	process (since interval timers are not inherited across fork()). */

//	rlim.rlim_cur = rlim.rlim_max = 50;
//	if (setrlimit(RLIMIT_CPU, &rlim) == -1)
//		errExit("setrlimit");

	/* Run the two processes in the lowest SCHED_FIFO priority */

	/* each process shares 30 % of CPU bandwidth */
#ifdef SCHED_VMS
	if(vms)
	{
		/*
		* valid sched_priority is always 0 for sched vms 
		* Must assign a valid share (0-100)
		*/
		sp.sched_priority = 0;
		sp.share = 10;
	}
	else
#else
		sp.sched_priority = sched_get_priority_min(SCHED_RR);
#endif
		if (sp.sched_priority == -1)
				errExit("sched_get_priority_min");
			
	if (vms)
	{
#ifdef SCHED_VMS
		if (sched_setscheduler(0, SCHED_VMS, &sp) == -1)
			errExit("sched_setscheduler");
#else
		printf("VMS is not configured\n");
#endif 
	}
	else
	{
		if (sched_setscheduler(0, SCHED_RR, &sp) == -1)
				errExit("sched_setscheduler");
	}
	printf("calling fork");
	debug_scheduler("before fork");
	for (int i = 0; i < 4; i++) {
		switch (fork()) {
		case -1:
			errExit("fork");

		case 0:
			debug_scheduler("fork 0");
			useCPU("child", i);
			exit(EXIT_SUCCESS);

		default:
			continue;
		}
	}
	debug_scheduler("fork other");
	useCPU("parent", 0);
	exit(EXIT_SUCCESS);
}

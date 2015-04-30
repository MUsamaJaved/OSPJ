/**
 * @file   ospj_setsched.c
 * @Author Armand, Andrew
 * @date   January, 2015
 * @brief  Operating System Project set scheduler policy utility.
 *
 * Useful links:
 * - http://man7.org/linux/man-pages/man2/sched_setscheduler.2.html
 * - http://www-numi.fnal.gov/offline_software/srt_public_context/WebDocs/Errors/unix_system_errors.html
 */

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>

#include <sched.h>

char *names[] = {
	"SCHED_NORMAL",
	"SCHED_FIFO",
	"SCHED_RR",
	"SCHED_BATCH",
	"SCHED_ISO", // reserved but not implemented
	"SCHED_IDLE",
	"SCHED_VMS",
};

enum {
	SCHED_NORMAL_ID = 0,
	SCHED_FIFO_ID = 1,
	SCHED_RR_ID = 2,
	SCHED_BATCH_ID = 3,
	SCHED_ISO_ID = 4,
	SCHED_IDLE_ID = 5,
	SCHED_VMS_ID = 6,
};

const uint names_length = sizeof(names) / sizeof(names[0]);

char *pid_arg;
char *policy_arg;
char *share_arg;
char *priority_arg;
bool flag_help = false;
bool flag_version = false;
bool flag_debug = false;
bool flag_set = false;
bool flag_read = false;
bool flag_policy = false;
bool flag_share = false;
bool flag_priority = false;


void opt_version() {
	fprintf(stdout, "ospj-setsched %s\n", BUILD_VERSION);
	char *vms_support;
#ifdef SCHED_VMS
	vms_support = "ENABLED";
#else
	vms_support = "DISABLED";
#endif
	fprintf(stdout, "SCHED_VMS support %s\n", vms_support);
	exit(0);
}

void opt_help(bool error) {
	FILE *fout = stdout;
	if (error) {
		fout = stderr;
	}

	fprintf(fout, "Usage: ospj-setsched OPTIONS\n\n");
	fprintf(fout, "Option\t\t\tMeaning\n");
	fprintf(fout, "-h\t\t\tShow this message\n");
	fprintf(fout, "-v\t\t\tShow program version\n");
	fprintf(fout, "-d\t\t\tVerbose output\n");
	fprintf(fout, "-s <#pid>\t\tSet scheduling policy for the process #pid\n");
	fprintf(fout, "-r <#pid>\t\tRead scheduling policy for the process #pid\n");
	fprintf(fout, "-p <#policy> or\t\tSupply #policy (see sched.h) for the flag -s\n");
	fprintf(fout, "-p SCHED_(NORMAL|FIFO|RR|BATCH|IDLE|VMS)\n");
	fprintf(fout, "-u <#per>\t\tVMS usage share #per in range [1,100] (if enabled)\n");
	fprintf(fout, "-o <#prio>\t\tScheduling priority prio in range [1,99] (only for\n\t\t\tSCHED_FIFO and SCHED_RR)\n");

	if (error) {
		exit(1);
	}
	exit(0);
}

uint find_flag(const char *name) {
	if (name == NULL) {
		abort();
	}
	for(int i = 0; i < names_length; i++) {
		if(!strcmp (names[i], name)) {
			return i;
		}
	}

	return -EINVAL;
}

void opt_setsched() {
	uint policy;
	uint pid;
	char *endptr;
	struct sched_param sp;

	if(flag_policy == false) {
		fprintf(stderr, "Setting the scheduling policy requires a valid policy\n\n");
		opt_help(false);
	}

	//TODO check later if is a correct number
	//TODO check if a process with this PID exists
	pid = strtoul(pid_arg, &endptr, 10);

	if (isdigit(policy_arg[0])) {
		// first char of the policy is digit, let's see
		policy = strtoul(policy_arg, &endptr, 10);
		if (endptr == policy_arg) {
			// no conversion performed
			policy = -1;
		}
	} else {
		policy = find_flag(policy_arg);
	}

	if (policy < 0 || policy > names_length - 1) {
		fprintf(stderr, "The policy shall be supplied as a number or as SCHED_* value\n\n");
		opt_help(true);
	}

	if (policy == SCHED_ISO_ID) {
		fprintf(stderr, "The SCHED_ISO policy was reserved but not implemented yet\n\n");
		opt_help(true);
	}

	if (policy == SCHED_VMS_ID) {

#ifndef SCHED_VMS
		fprintf(stderr, "ospj-setsched was built without SCHED_VMS support\n");
		fprintf(stderr, "Run 'ospj-setsched -v' to verify the program version\n\n");
		opt_help(true);
#else
		// need an additional param
		if (!flag_share) {
			fprintf(stderr, "To set SCHED_VMS policy, a VMS share must be supplied\n\n");
			opt_help(true);
		}

		uint share = strtoul(share_arg, &endptr, 10);
		if (share < 1 || share > 100) {
			fprintf(stderr, "VMS usage share is out of range\n\n");
			opt_help(true);
		}

		sp.share = share;
#endif
	}

	if (flag_debug) {
		fprintf(stdout, "Setting the scheduling policy for pid %d to %s\n", pid, names[policy]);
	}

	sp.sched_priority = 0;

	if (policy == SCHED_FIFO_ID || policy == SCHED_RR_ID) {
		// needs a valid priority
		if (!flag_priority) {
			fprintf(stderr, "SCHED_FIFO and SCHED_RR require a valid priority\n\n");
			opt_help(true);
		}

		sp.sched_priority = strtoul(priority_arg, &endptr, 10);

		if (sp.sched_priority < 1 || sp.sched_priority > 99) {
			fprintf(stderr, "scheduling priority is out of range\n\n");
			opt_help(true);
		}
	}

	int retval = sched_setscheduler(pid, policy, &sp);

	if (retval != 0) {
		const char *message;
		if (errno == EPERM) {
			message = "you must be root to execute this action";
		} else if (errno == EINVAL) {
			message = "kernel discarded the input, send your core dump to the developers";
		} else {
			message = "unknown error, send your core dump to the developers";
		}

		fprintf(stderr, "Failed to set policy %s for pid %d: %s.\n", names[policy], pid, message);
		abort(); // force core dump
	} else {
		fprintf(stdout, "Policy %s was succesfully set for pid %d\n", names[policy], pid);
		exit(0);
	}
}

void opt_read() {
	uint pid;
	char *endptr;

	//TODO check later if is a correct number
	//TODO check if a process with this pid exists
	pid = strtol(pid_arg, &endptr, 10);

	int pid_sched = sched_getscheduler(pid);
	uint policy = pid_sched & 0x7;
	if(flag_debug) {
		fprintf(stdout, "Retrieved result: 0x%X (-%d)\n", pid_sched, -pid_sched);
	}

	if (pid_sched == -EINVAL) {
		//invalid pid
		fprintf(stderr, "Failed to retrieve scheduling policy, pid is invalid\n\n");
		opt_help(true);
	} else if (pid_sched == -ESRCH || pid_sched == -1) {
		//no such process
		//-1 is a hack, found manually
		fprintf(stderr, "Failed to retrieve scheduling policy, there is no process with such pid \n\n");
		opt_help(true);
	} else if (pid_sched == -EFAULT) {
		//no such process
		fprintf(stderr, "Failed to retrieve scheduling policy, EFAULT was returned\n\n");
		opt_help(true);
	} 

	fprintf(stdout, "Current scheduling policy of pid %d is %s\n", pid, names[policy]);
#ifdef SCHED_VMS
	if (policy == SCHED_VMS_ID)
		fprintf(stdout, "VMS share is %d\n", pid_sched >> 4);
#endif
	exit(0);
}

int main(int argc, char *argv[]) {
	extern char *optarg;
	extern int optind;
	extern int optopt;

	char c;

	if (argc < 2) {
		opt_help(true);
	}

	while ((c = getopt(argc, argv, "+hvds:r:p:u:o:")) != -1) {
		switch (c) {
			case 'h':
				flag_help = true;
				opt_help(false);
			case 'v':
				flag_version = true;
				break;
			case 'd':
				flag_debug = true;
				break;
			case 's':
				flag_set = true;
				pid_arg = optarg;
				break;
			case 'r':
				flag_read = true;
				pid_arg = optarg;
				break;
			case 'p':
				flag_policy = true;
				policy_arg = optarg;
				break;
			case 'u':
				flag_share = true;
				share_arg = optarg;
				break;
			case 'o':
				flag_priority = true;
				priority_arg = optarg;
				break;

			default:
				flag_help = true;
				opt_help(true);
		}
	}

	if (flag_version) {
		opt_version();
		exit(0);
	}

	if ((flag_policy || flag_share)
		&& ! (flag_set)) {
		// option flags are useless w/o action
		opt_help(true);
	}

	if (flag_set) {
		opt_setsched();
	}

	if (flag_read) {
		opt_read();
	}

	return 0;
}

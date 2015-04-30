### Build instructions

Run `make` to build the tool.

### How to run it?

From the tool help output:

```
andrew@andrew-zenbook:/tub/ospj/ospj_src/Userspace/ospj-setsched$ ./ospj-setsched 
ospj-setsched <pid> <scheduler_id> [share]
Note: <share> is to be in [1,100]
```

### How to debug it?

Before running the `ospj-setsched`, execute

```
ulimit -s unlimited
ulimit -c unlimited
```

Run the program and right after it finishes, execute the following if there was an error:

```
echo $? 
```

To get the exit code. Then look up this code in the source.

If you got a segfault, run

```
gdb ospj-setsched core
```

In the gdb prompt, run

```
bt
```

It'll give you something similar to the stacktrace you might have been used to from Java.

### Sample output

    andrew$ ./ospj-setsched -s 31792 -p SCHED_IDLE
    Policy SCHED_IDLE was succesfully set for pid 31792
    andrew$ ./ospj-setsched -s 31792 -p SCHED_VMS
    ospj-setsched was built without SCHED_VMS support
    Run 'ospj-setsched -v' to verify the program version

    Usage: ospj-setsched OPTIONS

    Option          Meaning
    -h          Show this message
    -v          Show program version
    -d          Verbose output
    -s <#pid>       Set scheduling policy for the process #pid
    -r <#pid>       Read scheduling policy for the process #pid
    -p <#policy> or     Supply #policy (see sched.h) for the flag -s
    -p SCHED_(OTHER|FIFO|RR|BATCH|IDLE|VMS)
    -u <#per>       VMS usage share #per in range [1,100] (if enabled)
    -o <#prio>      Scheduling priority prio in range [1,99] (only for
                SCHED_FIFO and SCHED_RR)
    andrew$ ./ospj-setsched -s 31792 -p SCHED_FIFO
    SCHED_FIFO and SCHED_RR require a valid priority

    Usage: ospj-setsched OPTIONS

    Option          Meaning
    -h          Show this message
    -v          Show program version
    -d          Verbose output
    -s <#pid>       Set scheduling policy for the process #pid
    -r <#pid>       Read scheduling policy for the process #pid
    -p <#policy> or     Supply #policy (see sched.h) for the flag -s
    -p SCHED_(OTHER|FIFO|RR|BATCH|IDLE|VMS)
    -u <#per>       VMS usage share #per in range [1,100] (if enabled)
    -o <#prio>      Scheduling priority prio in range [1,99] (only for
                SCHED_FIFO and SCHED_RR)
    andrew$ ./ospj-setsched -s 31792 -p SCHED_FIFO -o 80
    Failed to set policy SCHED_FIFO for pid 31792: you must be root to execute this action.
    Aborted (core dumped)

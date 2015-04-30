#!/bin/bash

freq() {
        if [ -z "$1" ]
        then
                grep 'cpu MHz' /proc/cpuinfo
        else
                sudo cpupower -c 0 frequency-set -f $(($1*1000))
        fi
}

time=${3-600}
host=${2-deneb}
LOGFILE="measurementResults-$(date +%y%m%d-%H%M).log"

declare -A shares=( [800]=50 [1800]=25 [2300]=20 [3000]=15 )

for frequency in "${!shares[@]}"; do
	ssh $host "sudo freq $frequency"
	ssh $host "sudo ospj-setsched -s $1 -p SCHED_NORMAL"
	SHARE=${shares[$frequency]}
	echo "share=$SHARE @ ${frequency}MHz"
	ssh $host "sudo ospj-setsched -s $1 -p SCHED_VMS -u $SHARE"
	echo "switched to $frequency Hz" >> "$LOGFILE"
	for n in `seq 1 5`; do
		echo "test #$n" >> "$LOGFILE"
		./measure.sh $time >> "$LOGFILE"
	done
done

echo 'done.'

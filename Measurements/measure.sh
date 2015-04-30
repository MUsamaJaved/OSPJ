#!/bin/bash

timespan=${1-600}	#in seconds 600=10min
# ==============================================
# created by Lukas Braband
echo Time after which measurement will start:
date

timeA=`./mclient.sh 1`
sleep $timespan
timeB=`./mclient.sh 1`

diff=$(($timeB-$timeA))

echo "starting point:   " $timeA
echo "end point:        " $timeB
echo Consumption in Ws within the last $timespan seconds: $diff Ws


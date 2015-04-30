
#Measurement and Evaluation
*Written by Stephan Bauroth*

##Measurement Scripts

The scripts to measure the energy consumption on `deneb` connect to the
measurement device `Expert PDU 8001`.

The device supports measuring two channels, we have `deneb` fixed on
channel one. To actually start a measurement, the server needs to be
started on your machine and `energy`, a machine within the local network
of `deneb`, needs to be reachable from your local area network.

First, the server script is started, then energy consumption up until
now can be read with the client script. For convenience, `measure.sh` 
will measure the consumed energy over a given timespan (default 10 min.)
automatically.

###Server Script
The server script periodically queries the device for information via
SNMP on already consumed power and `echo`s the return value into
`/tmp/energy.channel`.


```{style="code" caption="energie-mess-server.sh" .bash}
case $1 in
	"1" | "2") channel=$1;;
	*)
		echo "usage: $0 <channel>"	
		exit
esac

interval=0.43
trap cleanup EXIT
cleanup()
{
	echo "Quit."
	rm -f /tmp/energy.$channel
	exit
}

wh=`snmpget -c public -v 1 energy 1.3.6.1.4.1.28507.2.1.3.1.3.$channel | awk '{ print $4 }'`
wh_old=$wh
echo -n "Calibrating..."
while sleep $interval; do
	wh=`snmpget -c public -v 1 energy 1.3.6.1.4.1.28507.2.1.3.1.3.$channel | awk '{ print $4 }'`
	if [ $wh -ne $wh_old ]; then
		wh_old=$wh
		break
	fi
	wh_old=$wh
done
echo " done."
told=`date +"%s%N"`
sumwdiff=$(($wh*3600000000000))
xdiff=""
while sleep $interval; do
	w=`snmpget -c public -v 1 energy 1.3.6.1.4.1.28507.2.1.3.1.4.$channel | awk '{ print $4 }'`
	wh=`snmpget -c public -v 1 energy 1.3.6.1.4.1.28507.2.1.3.1.3.$channel | awk '{ print $4 }'`
	t=`date +"%s%N"`
	dt=$(($t-$told))
	told=$t
	sumwdiff=$(($sumwdiff + $w*$dt))
	if [ $wh -ne $wh_old ]; then
		xdiff=" DIFF=$((($sumwdiff-$wh*3600000000000)/1000000000))"
		sumwdiff=$(($wh*3600000000000))
		wh_old=$wh
		echo "dt=$dt W=$w Ws=$(($sumwdiff/1000000000)) Ws_ref=$(($wh*3600))$xdiff"
	fi
	echo $(($sumwdiff/1000000000)) > /tmp/energy.$channel
	xdiff=""
done
```

###Client Script
The cleint script queries the file `/tmp/energy.channel` and outputs the
result to `STDOUT`.

```{style="code" caption="energie-mess-client.sh" .bash}
case $1 in
	"1" | "2") channel=$1;;
	*)
		echo "usage: $0 <channel>"	
		exit
esac
cat /tmp/energy.$channel
```

###Measurement Script
The measurement script by Lukas Braband reads the already consumed
energy at starting time, waits for a given timespan, reads again,
calculates the difference and outputs everything nicely to `STDOUT`.
The server script is supposed to be already running when calling this.

```{style="code" caption="measure.sh" .bash}
timespan=600 	#in seconds 600=10min
# ==============================================
# created by Lukas Braband
echo Time after which measurement will start:
date

timeA=`./energie-mess-client.sh 1`
sleep $timespan
timeB=`./energie-mess-client.sh 1`

diff=$(($timeB-$timeA))

echo "starting point:   " $timeA
echo "end point:        " $timeB
echo Consumption in Ws within the last $timespan seconds: $diff Ws
```

##Evaluation
For now, since the power subsystem is not stable, the only measureable
adavtage of the scheduler is the strict shares. So, a CPU-intense task
(`yes > /dev/null`) is measured while run in QEMU. Setup A runs QEMU
scheduled by CFS, Setup B is scheduling the QEMU process with VMS and a
share of 30\%. As expected, the power consu,ption is reduced by the
built-in routines of Linux because the processor is idle.

**Table. Benchmark on limiting a tasks share (Duration for all experiments: 10 min).**

---------------------------
Setup    Energy consumption    
-------  ------------------
A        57051

B        47989
---------------------------



case $1 in
	"1" | "2") channel=$1;;
	*)
		echo "usage: $0 <channel> [<host>]"	
		exit
esac

host=${2:-energy}

interval=0.43
trap cleanup EXIT
cleanup()
{
	echo "Quit."
	rm -f /tmp/energy.$channel
	exit
}

#TODO define non-expanded variable for the command or function
wh=`snmpget -c public -v 1 $host 1.3.6.1.4.1.28507.2.1.3.1.3.$channel | awk '{ print $4 }'`
wh_old=$wh
echo -n "Calibrating..."
while sleep $interval; do
	echo -n "."
	wh=`snmpget -c public -v 1 $host 1.3.6.1.4.1.28507.2.1.3.1.3.$channel | awk '{ print $4 }'`
	if [ $wh -ne $wh_old ]; then
		wh_old=$wh
		break
	fi
	wh_old=$wh
done
echo " done."
told=`date +"%s%N"`
#TODO define normal date format
sumwdiff=$(($wh*3600000000000))
xdiff=""
while sleep $interval; do
	w=`snmpget -c public -v 1 $host 1.3.6.1.4.1.28507.2.1.3.1.4.$channel | awk '{ print $4 }'`
	wh=`snmpget -c public -v 1 $host 1.3.6.1.4.1.28507.2.1.3.1.3.$channel | awk '{ print $4 }'`
	t=`date +"%s%N"`
	dt=$(($t-$told))
	told=$t
#	dt=$(($dt/1000000))
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

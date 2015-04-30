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

wh=`snmpget -c public -v 1 energy.kbs.local 1.3.6.1.4.1.28507.2.1.3.1.3.$channel | cut -b54-`
wh_old=$wh
echo -n "Calibrating..."
while sleep $interval; do
	wh=`snmpget -c public -v 1 energy.kbs.local 1.3.6.1.4.1.28507.2.1.3.1.3.$channel | cut -b54-`
	if [ $wh -ne $wh_old ]; then
		wh_old=$wh
		break
	fi
	wh_old=$wh
done

echo " done."
told=`date +"%s%N"`	# seconds since 1970-01-01-00:00 UTC in nanoseconds
#sumwdiff=$(($wh*3600000000000))
sumwdiff=$((expr $wh * 3600000000000))
xdiff=""
while sleep $interval; do
	w=`snmpget -c public -v 1 energy.kbs.local 1.3.6.1.4.1.28507.2.1.3.1.4.$channel | cut -b54-`
	wh=`snmpget -c public -v 1 energy.kbs.local 1.3.6.1.4.1.28507.2.1.3.1.3.$channel | cut -b54-`
	t=`date +"%s%N"`
	dt=$(($t-$told))
	told=$t
#	dt=$(($dt/1000000))
	#sumwdiff = $((expr $sumwdiff + $w * $dt ))
	echo das hier ist w: $w
	sumwdiff = $((expr $sumwdiff + $w * $dt))

	if [ $wh -ne $wh_old ]; then
		xdiff=" DIFF=$((($sumwdiff-$wh*3600000000000)/1000000000))"
		sumwdiff=$(($wh*3600000000000))
		wh_old=$wh
		echo "dt=$dt W=$w Ws=$(($sumwdiff/1000000000)) Ws_ref=$(($wh*3600))$xdiff"
	fi

	echo $(($sumwdiff/1000000000)) > /tmp/energy.$channel
	xdiff=""

done

clear
$1 &
freq=$2
proc_id=$!
# cd /proc/$proc_id/
touch frequency
echo "Frequency : $freq " > frequency
echo "PID : $proc_id" >> frequency
exit 0

case $1 in
	"1" | "2") channel=$1;;
	*)
		echo "usage: $0 <channel>"	
		exit
esac
cat /tmp/energy.$channel

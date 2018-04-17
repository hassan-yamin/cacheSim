#now=$(date +"%T")
#echo "Current time : $now"

#./gcc 166.in -o 166.s

now=$(date +"%T")
echo "Current time : $now"

/home/hassan/Downloads/pin-3.6-97554-g31f0a167d-gcc-linux/pin -t obj-intel64/cacheSim.so -- ./gcc 166.in -o 166.s

now=$(date +"%T")
echo "Current time : $now"

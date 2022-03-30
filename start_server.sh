if [ $# -ne 5 ]
then
    echo "Error: Usage ./start_server PORT LOGFILE SENDING_RATE DURATION LOGPATH"
    exit
fi

port=$1
logfile="$5/$2"
rate=$3
duration=$4

# compile the server source if not exist
mkdir -p build && cd build && cmake .. && make && cd ..
kill $(ps aux | grep custom_udp_server | awk '{print $2}') || true
./bin/custom_udp_server ${port} ${logfile} ${rate} ${duration} > /dev/null 2>&1 &
echo "Success"

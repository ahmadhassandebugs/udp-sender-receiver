if [ $# -ne 5 ]
then
    echo "Error: Usage ./start_server PORT LOGFILE SENDING_RATE DURATION PROJECTFOLER"
    exit
fi

port=$1
logfile="$5/logs/$2"
rate=$3
duration=$4
projectfolder=$5

# compile the server source if not exist
cd ${projectfolder} && mkdir -p build && cd build && cmake .. && make && cd ..
kill $(lsof -t -i:${port}) || true
${projectfolder}/bin/custom_udp_server ${port} ${logfile} ${rate} ${duration} > /dev/null 2>&1 &
echo "Success"

#!/bin/bash

##########################################################
## Run multiple parallel instances of netmashup servers ##
##########################################################

if [[ "$#" -ne 5 ]];
then
    echo "Usage: $0 NUM_SERVERS RUN_NUMBER SENDING_RATE DURATION DIRECTION"
    exit 1
fi

# Assumes the port numbers used by the servers start at 5001
# and increase e.g. 5201, 5202, 5203, ...
base_port=5200

# Command line input: number of servers e.g. 5
num_servers=$1
shift

# Command line input: run_number for this experiment
run_number=$1
shift

# Command line input: sending rate
rate=$1
shift

# Command line input: duration is secs
duration=$1
shift

# Command line input: direction e.g. UP/DOWN
direction=$1
shift

## move to script directory
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
echo "Script Dir: $SCRIPT_DIR"

# compile the server source if not exist
printf "\n"
cd $SCRIPT_DIR && mkdir -p build && cd build && cmake .. && make && cd ..

# Run netmashup server multiple times
for i in `seq 1 $num_servers`; do

    printf "\n"

	# Set server port
	server_port=$(($base_port+$i));
    pid=$(lsof -t -i:${server_port})
    if [ -z "$pid" ]
    then
        echo "Nothing is running on port: $server_port"
    else
        echo "Stopping server on port: $server_port"
        kill $pid
    fi 
	
    # log filename includes server port and run_number
    logfile="$SCRIPT_DIR/logs/$run_number-$server_port.csv"

	# Run netmashup
    echo "starting server on port: $server_port"
	${SCRIPT_DIR}/bin/custom_udp_server ${server_port} ${logfile} ${rate} ${duration} ${direction} > /dev/null 2>&1 &

done

printf "\nAll servers are running... \n"
#!/bin/bash

# Assumes the port numbers used by the servers start at 5001
# and increase e.g. 5201, 5202, 5203, ...
base_port=5200
num_servers=5

# Run netmashup server multiple times
for i in `seq 1 $num_servers`; do

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
done

echo "All servers are stopped../"

# Commands

```bash
sudo apt-get update && sudo apt-get install build-essential cmake
```

```bash
mkdir build
cd build && cmake .. && make && cd ..
```

```bash
./custom_udp_server PORT LOG_FILE SENDING_RATE DURATION DOWN/UP
```

```bash
./custom_udp_client IP PORT LOG_FILE SENDING_RATE DURATION DOWN/UP
```

```bash
./custom_udp_server 4000 server.csv 1.0 10000
./custom_udp_client 127.0.0.1 4000 client.csv
```

Logs format on server:

- is_ack : if packet is an ack or not
- sequence_number : sequence # of ack (client's)
- send_timestamp : when client sent ack **client's clock**
- ack_sequence_number : packet sent by server, for which we received this ack
- ack_send_timestamp : time when packet was sent by the server **server's clock**
- ack_recv_timestamp : time when packet was received on client **client's clock**
- pkt_recv_timestamp: time when this ack was received **server's clock**
- ack_payload_length : payload of the packet

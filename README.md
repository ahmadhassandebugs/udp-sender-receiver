```bash
cd build && cmake .. && make && cd ..
```

```bash
./server PORT LOG_FILE SENDING_RATE DURATION
```

```bash
./client IP PORT LOG_FILE
```

```bash
./server 4000 server.csv 1.0 10000
./client 127.0.0.1 4000 client.csv
```
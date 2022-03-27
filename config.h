#ifndef UDP_CONFIG_H
#define UDP_CONFIG_H

const uint64_t PKT_PAYLOAD_LEN = 1424; // in bytes
const uint64_t RECV_BUFFER_LEN = 65536; // in bytes
const uint16_t SERVER_RECV_MSG_TIMEOUT = 2; // in secs
const double BITS_PER_BYTE = 8.0;
const double KILO = 1024.0;
const double MEGA = KILO * KILO;
const double DEF_PKT_INTERVAL_MS = ((double)PKT_PAYLOAD_LEN * BITS_PER_BYTE) / MEGA;

#endif //UDP_CONFIG_H
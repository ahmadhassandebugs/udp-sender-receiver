#ifndef UDP_CONFIG_H
#define UDP_CONFIG_H

const uint64_t PKT_PAYLOAD_LEN = 1200; // in bytes
const uint64_t RECV_BUFFER_LEN = 65536; // in bytes
const uint16_t SERVER_RECV_MSG_TIMEOUT = 2; // in secs
const double BITS_PER_BYTE = 8.0;
const double KILO = 1024.0;
const double MEGA = KILO * KILO;
const double SENDING_RATE_CONST = (MEGA / BITS_PER_BYTE) / ((double)PKT_PAYLOAD_LEN * 1000.0);  // pkts per ms

#endif //UDP_CONFIG_H
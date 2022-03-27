#ifndef UDP_PACKET_H
#define UDP_PACKET_H

#include <cstdint>
#include <string>
#include <stdexcept>
#include <sys/socket.h>
#include <pthread.h>

#include "utils.h"
#include "timestamp.h"
#include "config.h"

struct received_datagram {
    struct sockaddr_in source_address;
    uint64_t timestamp;
    std::string payload;
};

struct Packet
{
    struct Header {
        uint64_t sequence_number;
        uint64_t send_timestamp;

        uint64_t ack_sequence_number;
        uint64_t ack_send_timestamp;
        uint64_t ack_recv_timestamp;
        uint64_t ack_payload_length;

        /* Header for new message */
        explicit Header(uint64_t s_sequence_number);

        /* Parse header from wire */
        explicit Header(const std::string &str);

        /* Make wire representation of header */
        std::string to_string() const;
    } header;

    std::string payload;

    /* New Packet */
    Packet(uint64_t s_sequence_number,
           std::string s_payload);

    /* Parse incoming datagram from wire */
    Packet(const std::string &str);

    /* Fill in the send_timestamp for outgoing datagram */
    void set_send_timestamp();

    /* Make wire representation  of datagram */
    std::string to_string() const;

    /* Make human-readable representation of header */
    std::string get_string() const;

    /* Transform into an ack of the datagram */
    void transform_into_ack(uint64_t sequence_number,
                            uint64_t recv_timestamp);

    /* Is this message an ack? */
    bool is_ack() const;
};

void send_packet(const int socket_fd, const struct sockaddr *peer, socklen_t len, const std::string payload);
int receive_bytes(const int socket_fd, const struct sockaddr *peer, char *recv_buffer, size_t read_size);
received_datagram recv_packet(const int socket_fd);
std::string create_packet(uint64_t seq_num);

#endif //UDP_PACKET_H
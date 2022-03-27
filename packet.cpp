#include "packet.h"

#include <utility>

using namespace std;

/* helper to get the nth uint64_t field (in network byte order) */
uint64_t get_header_field(const size_t n, const string &str)
{
    if (str.size() < (n+1) * sizeof(uint64_t)) {
        throw runtime_error("packet too small to contain header");
    }

    const uint64_t* const data_ptr = reinterpret_cast<const uint64_t*>(str.data()) + n;

    return be64toh(*data_ptr);
}

/* Helper to put an uint64_t field (in network byte order) */
string put_header_filed(const uint64_t n)
{
    const uint64_t network_order = htobe64(n);
    return {reinterpret_cast<const char*>(&network_order),
            sizeof(network_order)};
}

/* Parse header from wire */
Packet::Header::Header(const std::string &str)
    : sequence_number(get_header_field(0, str)),
    send_timestamp(get_header_field(1, str)),
    ack_sequence_number(get_header_field(2, str)),
    ack_send_timestamp(get_header_field(3, str)),
    ack_recv_timestamp(get_header_field(4, str)),
    ack_payload_length(get_header_field(5, str))
{}

/* Parse incoming packet from wire */
Packet::Packet(const std::string &str)
    : header(str), payload(str.begin() + sizeof(header), str.end())
{}

/* Fill in the send_timestamp for an outgoing packet */
void Packet::set_send_timestamp()
{
    header.send_timestamp = timestamp_ms();
}

/* Make wire representation of header */
string Packet::Header::to_string() const
{
    return put_header_filed(sequence_number) + put_header_filed(send_timestamp) +
            put_header_filed(ack_sequence_number) + put_header_filed(ack_send_timestamp) +
            put_header_filed(ack_recv_timestamp) + put_header_filed(ack_payload_length);
}

/* Make human-readable representation of header */
string Packet::get_string() const
{   
    // IS_ACK, PKT_SEQ_NO, PKT_SEND_TIME, ACK_NO, ACK_SEND_TIME, PKT_RECV_TIME, PKT_LEN
    return string_format("%d, %d, %d, %d, %d, %d, %d", 
                         is_ack(), 
                         header.sequence_number, 
                         header.send_timestamp, 
                         header.ack_sequence_number, 
                         header.ack_send_timestamp, 
                         header.ack_recv_timestamp, 
                         header.ack_payload_length);
}

/* Make wire representation of packet */
string Packet::to_string() const
{
    return header.to_string() + payload;
}

/* Transform into an ack of packet */
void Packet::transform_into_ack(const uint64_t sequence_number, const uint64_t recv_timestamp)
{
    /* ack the old sequence number */
    header.ack_sequence_number = header.sequence_number;

    /* now assign new sequence number for outgoing ack */
    header.sequence_number = sequence_number;

    /* ack the other details */
    header.ack_send_timestamp = header.send_timestamp;
    header.ack_recv_timestamp = recv_timestamp;
    header.ack_payload_length = payload.length();

    /* delete the payload */
    payload.clear();
}

/* New Packet */
Packet::Packet(const uint64_t s_sequence_number, std::string s_payload)
    : header(s_sequence_number), payload(std::move(s_payload))
{}

/* Header for new packet */
Packet::Header::Header(const uint64_t s_sequence_number)
    : sequence_number(s_sequence_number),
    send_timestamp(-1),
    ack_sequence_number(-1),
    ack_send_timestamp(-1),
    ack_recv_timestamp(-1),
    ack_payload_length(-1)
{}

/* Is this packet ack? */
bool Packet::is_ack() const
{
    return header.ack_sequence_number != uint64_t(-1);
}

std::string create_packet(uint64_t seq_num) 
{
    /* all messages use the same dummy payload */
    static const std::string dummy_payload(PKT_PAYLOAD_LEN, 'x');
    Packet packet(seq_num, dummy_payload);
    packet.set_send_timestamp();  // send immediately
    return packet.to_string();
}


void send_packet(const int socket_fd, const struct sockaddr *peer, socklen_t len, const std::string payload)
{
    const ssize_t bytes_sent = sendto(socket_fd, payload.data(), payload.size(), 0, peer, len);
    if(size_t(bytes_sent) != payload.size()) {
        Error("Could not send packets; Error code: %d", bytes_sent);
        pthread_exit(NULL);
    }
}

int receive_bytes(const int socket_fd, const struct sockaddr *peer, 
                   char *recv_buffer, size_t read_size)
{
    socklen_t peer_len = sizeof(peer);
    ssize_t bytes_read = recvfrom(socket_fd, recv_buffer, read_size, 0, (struct sockaddr *) &peer, &peer_len);
    if(bytes_read <= 0) {
        Error("Read error or socket closed!!");
        pthread_exit(NULL);
    }
    return bytes_read;
}

/* receive datagram and where it came from */
received_datagram recv_packet(const int socket_fd)
{
    /* receive source address, timestamp and payload */
     struct sockaddr_in datagram_source_address;
    msghdr header{}; zero(header);
    iovec msg_iovec{}; zero(msg_iovec);

    char msg_payload[RECV_BUFFER_LEN];
    char msg_control[RECV_BUFFER_LEN];

    /* prepare to get the source address */
    header.msg_name = &datagram_source_address;
    header.msg_namelen = sizeof(datagram_source_address);

    /* prepare to get the payload */
    msg_iovec.iov_base = msg_payload;
    msg_iovec.iov_len = sizeof(msg_payload);
    header.msg_iov = &msg_iovec;
    header.msg_iovlen = 1;

    /* prepare to get the timestamp */
    header.msg_control = msg_control;
    header.msg_controllen = sizeof(msg_control);

    /* call recvmsg */
    // ssize_t recv_len = SystemCall("recvmsg", recvmsg(socket_fd, &header, 0));
    ssize_t recv_len = recvmsg(socket_fd, &header, 0);
    if (recv_len == -1 and errno == EAGAIN) {
        std::cerr << "recvmsg timeout\n";
        pthread_exit(NULL);
    }

    /* make sure we got the whole datagram */
    if (header.msg_flags & MSG_TRUNC) {
        Error("recvmsg (oversized datagram)");
    } else if (header.msg_flags) {
        Error("recvmsg (unhandled flag)");
    }

    uint64_t timestamp = -1;

    /* find the timestamp header (if there is one) */
    cmsghdr *ts_hdr = CMSG_FIRSTHDR(&header);
    while(ts_hdr) {
        if(ts_hdr->cmsg_level == SOL_SOCKET and ts_hdr->cmsg_type == SO_TIMESTAMPNS) {
            const timespec* const kernel_time = reinterpret_cast<timespec*>(CMSG_DATA(ts_hdr));
            timestamp = timestamp_ms(*kernel_time);
            // timestamp = timestamp_ms_raw(*kernel_time);
        }
        ts_hdr = CMSG_NXTHDR(&header, ts_hdr);
    }

    received_datagram ret = {datagram_source_address,
                             timestamp,
                             std::string(msg_payload, recv_len)};
    return ret;
}
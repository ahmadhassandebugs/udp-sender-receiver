#include <fstream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <csignal>
#include <thread>
#include <chrono>
#include <cmath>

#include "packet.h"
#include "config.h"
#include "timestamp.h"

int SERVER_RUNNING = true;
int listen_fd;
struct sockaddr_in server_addr, client_addr;
int sender_thread, receiver_thread;
void *send_udp_packets(void*);
void *recv_udp_packets(void*);
std::ofstream log_file_handler;
uint64_t milliseconds_to_sleep;
uint64_t pkts_to_send;
uint64_t duration;

bool DEBUG = true;

void signalHandler(int signum) {
    shutdown(listen_fd, SHUT_RDWR);
    log_file_handler.close();
    exit(signum);
}

int main(int argc, char **argv) {
    int LISTEN_PORT = 4000;
    const char* log_file_name = "server.csv";
    double sending_rate_mbps = 0.0108 * 100; // in Mbps
    duration = 2000;  // in ms

    // calculate packet interval for given sending rate
    double pkts_interval_sec = DEF_PKT_INTERVAL_MS / sending_rate_mbps;
    double pkt_interval_ms = pkts_interval_sec / 1000.0;
    if (pkt_interval_ms <= 0.001) { // if sending multiple packets each millisecond
        milliseconds_to_sleep = 1;
        pkts_to_send = std::round(0.001 / pkt_interval_ms);
    }
    else { // if sending one packet only a few milliseconds
        milliseconds_to_sleep = std::max(1, int(pkt_interval_ms / 0.001));
        pkts_to_send = 1;
    }
    Log("pkts interval ms %f sleep time %d ; pkts to send %d", pkt_interval_ms, milliseconds_to_sleep, pkts_to_send);

    // initialize signal handler and open log file
    signal(SIGINT, signalHandler);
    log_file_handler.open(log_file_name);

    // create server address
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(LISTEN_PORT);

    // create UDP socket
    listen_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_fd < 0) {
        Error("Cannot create socket to listen on!!!");
    }
    set_timestamps(listen_fd);

    // bind to a port
    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        Error("Cannot bind to port %d!!", LISTEN_PORT);
    }

    // initialize peer address struct
    memset(&client_addr, '\0', sizeof(struct sockaddr_in));
    socklen_t client_addr_len = sizeof(client_addr);

    // wait for client to send the first message
    if (DEBUG)
        Log("Waiting for the client...");
    char* recv_data = (char*)malloc(RECV_BUFFER_LEN);
    recvfrom(listen_fd, recv_data, RECV_BUFFER_LEN, 0, (struct sockaddr*) &client_addr, &client_addr_len);
    sendto(listen_fd, "Test1_ACK\n", strlen("Test1_ACK\n"), 0, (struct sockaddr *) &client_addr, client_addr_len);
    sendto(listen_fd, "Test2_ACK\n", strlen("Test2_ACK\n"), 0, (struct sockaddr *) &client_addr, client_addr_len);
    recvfrom(listen_fd, recv_data, RECV_BUFFER_LEN, 0, (struct sockaddr *) &client_addr, &client_addr_len);

    // print client address
    char address_str[INET_ADDRSTRLEN];
    if (DEBUG)
        Log("Communication established with client: %s", get_ip_str((struct sockaddr *) &client_addr, address_str, INET_ADDRSTRLEN));

    // connect socket to th4e client address
    connect_socket_to_address(listen_fd, (struct sockaddr *) &client_addr, client_addr_len);

    // create two threads: one for sending packets and one for receiving
    pthread_t send_thread, recv_thread;
    pthread_create(&send_thread, NULL, send_udp_packets, (void*) &listen_fd);
    pthread_create(&recv_thread, NULL, recv_udp_packets, (void*) &listen_fd);

    pthread_join(send_thread, NULL);
    Log("sender thread returned");
    pthread_join(recv_thread, NULL);
    Log("Recv thread returned");

    shutdown(listen_fd, SHUT_RDWR);
    log_file_handler.close();

    return 1;
}

/* use this function to send packets over a socket. */
void *send_udp_packets(void* fd_ptr)
{
    int server_seq_no = 1;
    int socket_fd = *((int*) fd_ptr);

    int count = 0;
    while (timestamp_ms() <= duration and SERVER_RUNNING) {
        while (count++ < pkts_to_send) {
            // send packets
            std::string message = create_packet(server_seq_no++);
            send_packet(socket_fd, (struct sockaddr *) &client_addr, sizeof(client_addr), message);
            if (DEBUG)
                Log("Custom message sent");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds_to_sleep));
        count = 0;
    }
    std::string message = create_packet(0);
    send_packet(socket_fd, (struct sockaddr *) &client_addr, sizeof(client_addr), message);
    send_packet(socket_fd, (struct sockaddr *) &client_addr, sizeof(client_addr), message);  // send two times just in case
    if (DEBUG)
        Log("Last Custom message sent");
    SERVER_RUNNING = false;
}

/* use this function to receive packets over a socket.
   the socket must already be connected to an address.*/
void *recv_udp_packets(void* fd_ptr)
{
    int socket_fd = *((int*) fd_ptr);
    set_socket_timeout(socket_fd, SERVER_RECV_MSG_TIMEOUT);

    // recv acks and log them
    while (SERVER_RUNNING) {
        received_datagram recv_message = recv_packet(socket_fd); // this will exit the thread if timeout
        Packet packet = recv_message.payload;
        if (DEBUG) {
                Log("Custom message received ==> %d, %d, %d, %d, %d, %d, %d", 
                            packet.is_ack(), 
                            packet.header.sequence_number, 
                            packet.header.send_timestamp, 
                            packet.header.ack_sequence_number, 
                            packet.header.ack_send_timestamp, 
                            packet.header.ack_recv_timestamp, 
                            packet.header.ack_payload_length);
        }
        std::string packet_info = string_format("%d, %d, %d, %d, %d, %d, %d\n", 
                                                packet.is_ack(), 
                                                packet.header.sequence_number, 
                                                packet.header.send_timestamp, 
                                                packet.header.ack_sequence_number, 
                                                packet.header.ack_send_timestamp, 
                                                packet.header.ack_recv_timestamp, 
                                                packet.header.ack_payload_length);
        log_file_handler << packet_info;
    }
}
#include <fstream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "packet.h"

#define BUFFER_SIZE 65536

struct sockaddr_in server_addr, client_addr;
int sender_thread, receiver_thread;
void *send_udp_packets(void*);
void *recv_udp_packets(void*);
std::ofstream log_file_handler;

int main(int argc, char **argv) {
    int LISTEN_PORT = 4000;
    const char* log_file_name = "server.csv";

    log_file_handler.open(log_file_name);

    // create server address
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(LISTEN_PORT);

    // create UDP socket
    int listen_fd = socket(AF_INET, SOCK_DGRAM, 0);
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
    Log("Waiting for the client...");
    char* recv_data = (char*)malloc(BUFFER_SIZE);
    recvfrom(listen_fd, recv_data, BUFFER_SIZE, 0, (struct sockaddr*) &client_addr, &client_addr_len);
    sendto(listen_fd, "Test1_ACK\n", strlen("Test1_ACK\n"), 0, (struct sockaddr *) &client_addr, client_addr_len);
    sendto(listen_fd, "Test2_ACK\n", strlen("Test2_ACK\n"), 0, (struct sockaddr *) &client_addr, client_addr_len);
    recvfrom(listen_fd, recv_data, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &client_addr_len);

    // print client address
    char address_str[INET_ADDRSTRLEN];
    Log("Communication established with client: %s", get_ip_str((struct sockaddr *) &client_addr, address_str, INET_ADDRSTRLEN));

    // connect socket to th4e client address
    connect_socket_to_address(listen_fd, (struct sockaddr *) &client_addr, client_addr_len);

    // create two threads: one for sending packets and one for receiving
    pthread_t send_thread, recv_thread;
    pthread_create(&send_thread, NULL, send_udp_packets, (void*) &listen_fd);
    pthread_create(&recv_thread, NULL, recv_udp_packets, (void*) &listen_fd);

    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);

    shutdown(listen_fd, SHUT_RDWR);
    log_file_handler.close();

    return 1;
}

/* use this function to send packets over a socket. */
void *send_udp_packets(void* fd_ptr)
{
    int server_seq_no = 1;
    int socket_fd = *((int*) fd_ptr);

    // send packet once for now
    std::string message = create_packet(server_seq_no++ );
    send_packet(socket_fd, (struct sockaddr *) &client_addr, sizeof(client_addr), message);
    Log("Custom message sent");
}

/* use this function to receive packets over a socket.
   the socket must already be connected to an address.*/
void *recv_udp_packets(void* fd_ptr)
{
    int socket_fd = *((int*) fd_ptr);

    // send packet once for now
    received_datagram recv_message = recv_packet(socket_fd);
    Packet packet = recv_message.payload;
    std::string packet_info = string_format("%d, %d, %d, %d, %d, %d, %d\n", 
                                            packet.is_ack(), 
                                            packet.header.sequence_number, 
                                            packet.header.send_timestamp, 
                                            packet.header.ack_sequence_number, 
                                            packet.header.ack_send_timestamp, 
                                            packet.header.ack_recv_timestamp, 
                                            packet.header.ack_payload_length);
    Log("Custom message received ==> %d, %d, %d, %d, %d, %d, %d", 
                         packet.is_ack(), 
                         packet.header.sequence_number, 
                         packet.header.send_timestamp, 
                         packet.header.ack_sequence_number, 
                         packet.header.ack_send_timestamp, 
                         packet.header.ack_recv_timestamp, 
                         packet.header.ack_payload_length);
    log_file_handler << packet_info;
}
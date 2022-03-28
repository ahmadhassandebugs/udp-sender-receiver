#include <fstream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <pthread.h>
#include <csignal>


#include "packet.h"
#include "config.h"

int client_fd;
std::ofstream log_file_handler;

bool DEBUG = false;

void signalHandler(int signum) {
    shutdown(client_fd, SHUT_RDWR);
    log_file_handler.close(); 
    exit(signum);
}

int run_client(const char* server_ip, int server_port, const char* log_file_name) {

    signal(SIGINT, signalHandler);

    log_file_handler.open(log_file_name);

    // initialize server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        Log("Invalid address or address not supported");
        pthread_exit(NULL);
    }
    socklen_t server_addr_len = sizeof(server_addr);


    // initialize UDP socket
    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0) {
        Error("Socket creation error"); // replace with android logging
        pthread_exit(NULL);
    }
    set_timestamps(client_fd);

    // send first packet to server and wait for 2 packets to establish communication
    if (DEBUG)
        Log("Sending message to the server");
    char* recv_data = (char*)malloc(RECV_BUFFER_LEN);
    sendto(client_fd, "Test1\n", strlen("Test1\n"), 0, (struct sockaddr *) &server_addr, server_addr_len);
    recvfrom(client_fd, recv_data, RECV_BUFFER_LEN, 0, (struct sockaddr *) &server_addr, &server_addr_len);
    recvfrom(client_fd, recv_data, RECV_BUFFER_LEN, 0, (struct sockaddr *) &server_addr, &server_addr_len);
    sendto(client_fd, "Test2_ACK\n", strlen("Test2_ACK\n"), 0, (struct sockaddr *) &server_addr, server_addr_len);
    Log("Communication established with server...");

    int client_seq_no = 1;
    // connect socket to the server address
    connect_socket_to_address(client_fd, (struct sockaddr *) &server_addr, server_addr_len);
    // start receiving packets
    while (true) {
        received_datagram message = recv_packet(client_fd);
        Packet packet = message.payload;
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
        if (packet.header.sequence_number <= 0) {
            break;
        }
        packet.transform_into_ack(client_seq_no++, message.timestamp);
        packet.set_send_timestamp();
        send_packet(client_fd, (struct sockaddr *) &server_addr, server_addr_len, packet.to_string());
    }

    shutdown(client_fd, SHUT_RDWR);
    log_file_handler.close();

    return 1;
}

int main(int argc, char** argv) {
    if (argc == 4) {
        char* server_ip = argv[1];
        int server_port = std::atoi(argv[2]);
        char* log_file_name = argv[3];
        return run_client(server_ip, server_port, log_file_name);
    }
    else {
        return 0;
    }
}

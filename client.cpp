#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <pthread.h>

#include "packet.h"

#define BUFFER_SIZE 65536

int client_fd;

int main(int argc, char **argv) {
    int SERVER_PORT = 4000;
    const char* server_ip = "141.212.108.160";

    // initialize server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
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

    // send first packet to server and wait for 2 packets to establish communication
    Log("Sending message to the server");
    char* recv_data = (char*)malloc(BUFFER_SIZE);
    sendto(client_fd, "Test1\n", strlen("Test1\n"), 0, (struct sockaddr *) &server_addr, server_addr_len);
    recvfrom(client_fd, recv_data, BUFFER_SIZE, 0, (struct sockaddr *) &server_addr, &server_addr_len);
    recvfrom(client_fd, recv_data, BUFFER_SIZE, 0, (struct sockaddr *) &server_addr, &server_addr_len);
    sendto(client_fd, "Test2_ACK\n", strlen("Test2_ACK\n"), 0, (struct sockaddr *) &server_addr, server_addr_len);
    Log("Communication established with server...");

    // receive custom messages
    connect_socket_to_address(client_fd, (struct sockaddr *) &server_addr, server_addr_len);
    received_datagram message = recv_packet(client_fd);
    Log("Custom message received");

}
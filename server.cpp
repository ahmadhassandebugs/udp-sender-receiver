#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "utils.cpp"

#define BUFFER_SIZE 50000

int main(int argc, char **argv) {
    int LISTEN_PORT = 4000;

    // create server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(LISTEN_PORT);

    // create UDP socket
    int listen_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_fd < 0) {
        Error("Cannot create socket to listen on!!!");
    }

    // bind to a port
    if (bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) != 0) {
        Error("Cannot bind to port %d!!", LISTEN_PORT);
    }

    // initialize peer address struct
    struct sockaddr_in client_addr;
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
    // inet_ntop(AF_INET, &(client_addr.sin_addr), address_str, INET_ADDRSTRLEN);
    Log("Communication established with client: %s", get_ip_str((struct sockaddr *) &client_addr, address_str, INET_ADDRSTRLEN));

}
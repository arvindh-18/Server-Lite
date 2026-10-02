#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9000
#define BUFFER_SIZE 1024

int main() {
    // 1. Create socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) return 1;

    // 2. Bind
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(listen_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) return 1;

    // 3. Listen
    listen(listen_fd, 1);
    std::cout << "Listening on 127.0.0.1: " << PORT;

    // 4. Accept
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(listen_fd, (sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) return 1;

    close(listen_fd); // Close listener - As the request is stored in cilent_fd and this is a single client server

    // 5. Echo Loop
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE)) > 0) {
        write(client_fd, buffer, bytes_read);
    }

    // 6. Cleanup 
    close(client_fd);
    return 0;
}
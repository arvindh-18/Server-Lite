#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

#define PORT 8080
#define BUFFER_SIZE 4096

// Preparing a simple HTTP response
void sendTextResponse(int& client_fd, int statusCode, const string& statusText, const string& message) {
    ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n"<< "Content-Length: " << message.size() << "\r\n"<< "Connection: close\r\n\r\n"<< message;
    string text = response.str();
    write(client_fd, text.data(), text.size());
}

int main() {
    // 1. Create socket
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) return 1;

    // Reuse por
    int option = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    // 2. Bind to 127.0.0.1:8080
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(listen_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Bind failed\n";
        close(listen_fd);
        return 1;
    }

    // 3. Listen
    listen(listen_fd, 1);
    cout << "Server listening on http://127.0.0.1:" << PORT << "\n";

    // 4. Accept one client
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    int client_fd = accept(listen_fd, (sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
        close(listen_fd);
        return 1;
    }

    close(listen_fd); // Serve this one client only

    // 5. Read the request
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        close(client_fd);
        return 1;
    }
    buffer[bytes_read] = '\0';

    // 6. Parse method, target, and version
    string raw(buffer);
    istringstream stream(raw);
    string method, target, version;

    // Check and returning 0 if the request is not properly given in the expected format
    if (!(stream >> method >> target >> version)) {
        sendTextResponse(client_fd, 400, "Bad Request", "400 Bad Request\n");
        close(client_fd);
        return 0;
    }

    // Not a GET request so we return 405
    if (method != "GET") {
        sendTextResponse(client_fd, 405, "Method Not Allowed", "405 Method Not Allowed\n");
        close(client_fd);
        return 0;
    }

    // Currently only "/" returns OK, anything else is 404
    if (target == "/") {
        sendTextResponse(client_fd, 200, "OK", "Welcome to Light Server Machi..\n");
    } else {
        sendTextResponse(client_fd, 404, "Not Found", "404 Not Found\n");
    }

    // 7. Cleanup
    close(client_fd);
    cout << "Request finished.\n";
    return 0;
}
#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <cctype>
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <thread>

#define PORT 8080
#define MAX_HEADER_SIZE 8192
#define MAX_BODY_SIZE   1048576

#include "logger.h"

using namespace std; 

// Mapping HTTPS status codes with their corresponding messages
unordered_map<int, string> STATUS_MESSAGES = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {413, "Content Too Large"},
    {501, "Not Implemented"}
};

struct HttpRequest {
    string method,target,version;
    unordered_map<string, string> headers;
    string body;
    size_t content_length = 0;
};

bool sendAll(int,const char*,size_t);
void sendResponse(int,int,string,const string&);
void sendErrorResponse(int,int);
string trim(const string&);
string toLower(string);
int parseRequest(const string&, HttpRequest&);
void handleClient(int);
void handleSigint(int);

volatile sig_atomic_t is_running = 1;


// Main server
int main() {

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    struct sigaction sa{};
    sa.sa_handler = handleSigint;
    sigaction(SIGINT, &sa, nullptr);

    if (listen_fd < 0) {
        logMessage(LogLevel::ERROR, "Failed to create socket");
        return 1;
    }

    int option = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(listen_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        logMessage(LogLevel::ERROR, "Socket bind failed");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 10) < 0) {
        logMessage(LogLevel::ERROR, "Socket listen failed");
        close(listen_fd);
        return 1;
    }

    logMessage(LogLevel::INFO, "Server listening on http://127.0.0.1:" + to_string(PORT));

    while (is_running) {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(listen_fd, (sockaddr*)&client_addr, &addr_len);

        if (client_fd < 0) {
            if(!is_running) break; // Exit loop if server is shutting down
            if (errno == EINTR) continue;
            logMessage(LogLevel::WARN, "Accept failed");
            continue;
        }

        std::thread(handleClient, client_fd).detach(); // Making an independent thread for each client 
    }

    close(listen_fd);
    logMessage(LogLevel::INFO, "Server shutting down gracefully.");
    return 0;
}

// This function ensures that all data is sent over the socket and handles partial sends if interruped.
bool sendAll(int socket_fd, const char* data, size_t length) {
    size_t total_sent = 0;
    while (total_sent < length) {
        ssize_t sent = send(socket_fd, data + total_sent, length - total_sent, 0);
        if (sent < 0) {
            if (errno == EINTR) continue; // Checks if someother OS interrupt occurred, and if so, it continues the loop to retry sending the data.
            return false;        
        }
        if (sent == 0) return false;     
        total_sent += sent;
    }
    return true;
}

//This function will contruct the HTTP reponase and send it to the client by calling sendAll funciton.
void sendResponse(int client_fd, int statusCode, string statusText, const string& body) {
    ostringstream response;
    response << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Content-Type: text/plain\r\n"
             << "Connection: close\r\n\r\n"
             << body;

    string text = response.str();
    sendAll(client_fd, text.data(), text.size());
}

//This funcation will send the status code and the corresponding message to the client by calling sendResponse function.
void sendErrorResponse(int client_fd, int statusCode) {
    auto it = STATUS_MESSAGES.find(statusCode);
    string statusText;
    if(it == STATUS_MESSAGES.end()) statusText = "Error";
    statusText = it->second;
    sendResponse(client_fd, statusCode,statusText, to_string(statusCode) + " " + statusText + "\r\n");
}

//This function will trim the leading and trailing whitespaces
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

//This function will supports case-insesitive header 
string toLower(string s) {
    for (char &c : s) c = tolower(static_cast<unsigned char>(c));
    return s;
}


//This function will parse the HTTP request and return the status code.
int parseRequest(const string& raw_buffer, HttpRequest& req) {
    size_t header_end = raw_buffer.find("\r\n\r\n");
    if (header_end == string::npos) return 0; // Keep reading

    istringstream stream(raw_buffer.substr(0, header_end));
    string line;

    if (!(stream >> req.method >> req.target >> req.version)) return 400;
    if (req.method != "GET") return 501;
    if (req.target.find("..") != string::npos) return 403;

    getline(stream, line);
    while (getline(stream, line)) {
        size_t colon = line.find(':');
        if (colon == string::npos) continue;

        string key = toLower(trim(line.substr(0, colon)));
        string val = trim(line.substr(colon + 1));
        req.headers[key] = val;
    }

    if (req.version == "HTTP/1.1" && req.headers.find("host") == req.headers.end()) {
        return 400;
    }

    return 200;
}


void handleClient(int client_fd) {
    string raw_buffer;
    char chunk[1024];
    HttpRequest req;
    int status = 0;

    // Read until headers are complete (\r\n\r\n found)
    while (status == 0) {
        ssize_t bytes = recv(client_fd, chunk, sizeof(chunk), 0);
        if (bytes <= 0) {
            if (bytes < 0 && errno == EINTR) continue;
            close(client_fd);
            return;
        }

        raw_buffer.append(chunk, bytes);
        status = parseRequest(raw_buffer, req);
    }

    if (status != 200) {
        logMessage(LogLevel::WARN, "Rejected request with status " + to_string(status));
        sendErrorResponse(client_fd, status);
    } else if (req.target == "/") {
        logMessage(LogLevel::INFO, req.method + " " + req.target + " - 200 OK");
        sendResponse(client_fd, 200, "OK", "Welcome to Light Server!\r\n");
    } else {
        logMessage(LogLevel::INFO, req.method + " " + req.target + " - 404 Not Found");
        sendErrorResponse(client_fd, 404);
    }
}

void handleSigint(int signum) {
    is_running = 0;
    logMessage(LogLevel::INFO, "SIGINT received, shutting down server...");
}
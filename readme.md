Light Web Server
A small HTTP/1.1 web server written in C++ .

I building this mainly as a learning project to understand what actually happens when a browser or curl connects to a server — starting from a TCP connection, reading the request, parsing it, finding a file, and sending back an HTTP response.

LOGS:
1. Learnt to operate with files only using open,read,close and write.
2. Built a simple TCP server that handles one client and succesfully tested it with nc.
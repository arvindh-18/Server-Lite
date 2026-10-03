Light Web Server
A small HTTP/1.1 web server written in C++ .

I building this mainly as a learning project to understand what actually happens when a browser or curl connects to a server — starting from a TCP connection, reading the request, parsing it, finding a file, and sending back an HTTP response.

LOGS:
1. Learnt to operate with files only using open,read,close and write.
2. Built a simple TCP echo server that handles one client and succesfully tested it with nc.
![Alternative text description](images/demo.png)
3. Now it parses the request line to verify it's a valid `GET` call, replies with plain text (`200 OK` for `/`, `404` for anything else, `405` for other methods, or `400` if the request is not formatted properly), and closes the connection. I also added `SO_REUSEADDR` with the correct byte size so the port frees up immediately on restart.
![Alternative text description](images/demo2.png)
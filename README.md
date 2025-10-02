# Redis-Lite

Jai shree ram

Socket Programming-
Sockets are endpoints for communication between two machines or processes.
Socket programming allows programs to send and receive data over a network.

In our code it’s used to implement client-server architectures

| Type          | Description                                        |
| ------------- | -------------------------------------------------- |
| `SOCK_STREAM` | TCP socket (reliable, connection-oriented)         |
| `SOCK_DGRAM`  | UDP socket (connectionless, faster but unreliable) |

TCP socket workflow-
Server side:-
Create a socket.
Bind it to an IP and port.
Listen for incoming connections.
Accept a connection.
Send/receive data.
Close the connection.

Client side:-
Create a socket.
Connect to server’s IP and port.
Send/receive data.
Close the connection.

RedisServer::run() follows the classic TCP server workflow:

| Step | Function       | Purpose                                                |
| ---- | -------------- | ------------------------------------------------------ |
| 1    | `socket()`     | Create a socket (endpoint for communication)           |
| 2    | `setsockopt()` | Set options like `SO_REUSEADDR` to reuse the port      |
| 3    | `sockaddr_in`  | Define server IP, port, and address family             |
| 4    | `bind()`       | Associate the socket with the server’s IP and port     |
| 5    | `listen()`     | Put the socket in **listening mode** to accept clients |



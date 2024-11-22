#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "socketutils.h"

// Maximum number of device that can queue to be accepted.
// Irrelevent on current configuration.
const int listenQueueSize = 10;

SocketData connect_to_port(char* portNumber)
{
    SocketData socketData = {-1, NULL, NULL};
    struct addrinfo* addInfoList = NULL; // Can be a linked list.
    struct addrinfo description = {0};
    description.ai_family = AF_INET; // IPv4.
    description.ai_socktype = SOCK_STREAM; // Two way connections.
    int error
            = getaddrinfo("localhost", portNumber, &description, &addInfoList);
    if (error) { // Could not create address info under given config.
        return socketData;
    }

    socketData.handle = socket(AF_INET, SOCK_STREAM, 0);
    error = connect(
            socketData.handle, addInfoList->ai_addr, sizeof(struct sockaddr));
    if (error) { // Could not connect to service at portNumber under config.
        socketData.handle = -1;
        return socketData;
    }

    // Open input/output file streams to socket and file streams and file
    // descriptor.
    socketData.get = fdopen(dup(socketData.handle), "r");
    socketData.post = fdopen(dup(socketData.handle), "w");
    return socketData;
}

int open_port(char* portNumber)
{
    struct addrinfo* addressInfoList = NULL;
    struct addrinfo description = {0};
    description.ai_family = AF_INET; // IPv4.
    description.ai_socktype = SOCK_STREAM; // Two way.
    description.ai_flags = AI_PASSIVE; // Can bind to port.

    int error = getaddrinfo(NULL, portNumber, &description, &addressInfoList);
    if (error) { // Could not created address info under given config.
        return -1;
    }

    int socketHandle = socket(AF_INET, SOCK_STREAM, 0);

    // Specifies that socket addresses can be reused.
    // Still will not bind if there is an activer listener.
    int shouldReuse = 1;
    error = setsockopt(
            socketHandle, SOL_SOCKET, SO_REUSEADDR, &shouldReuse, sizeof(int));
    if (error) {
        return -1;
    }

    // Attempt to bind a socket to an adress.
    error = bind(
            socketHandle, addressInfoList->ai_addr, sizeof(struct sockaddr));
    if (error) {
        return -1;
    }

    // Attempts to open the socket for listening.
    error = listen(socketHandle, listenQueueSize);
    if (error) {
        return -1;
    }

    // REF: getting a a port from the service
    // REF: name is based on moss week8 net4.c code.
    struct sockaddr_in addressBuffer;
    socklen_t bufferLength = sizeof(struct sockaddr_in);
    getsockname(socketHandle, (struct sockaddr*)&addressBuffer, &bufferLength);
    // ntohs converts network byte order (big-endian) to x86 byte order
    // (little-endian).
    fprintf(stderr, "%i\n", ntohs(addressBuffer.sin_port));
    fflush(stderr);

    return socketHandle;
}

SocketData block_for_connection(int socketHandle)
{
    SocketData socketData = {-1, NULL, NULL};
    struct sockaddr_in clientSockAddress;
    socklen_t clientSockAddressSize = sizeof(struct sockaddr_in);

    // The following will block, waiting for a new connection to accept.
    socketData.handle = accept(socketHandle,
            (struct sockaddr*)&clientSockAddress, &clientSockAddressSize);

    // Open file streams for input/output and return file streams as well
    // as the socket file descriptor.
    socketData.get = fdopen(dup(socketData.handle), "r");
    socketData.post = fdopen(dup(socketData.handle), "w");
    return socketData;
}

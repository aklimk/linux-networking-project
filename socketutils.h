#ifndef SOCKETUTILS_H
#define SOCKETUTILS_H

#include <stdio.h>

/* Holds a handle to an open socket and two file streams to its
   inpupt and output. The file streams are based on duplicates
   of the socket file descriptor. */
typedef struct SocketData {
    int handle;
    FILE* post;
    FILE* get;
} SocketData;

/* connect_to_port()
 * -----------------
 * Attempts to establish a connection with a port being listened too.
 *
 * portNumber: the service identifier for the port to connect to.
 *
 * return: a SocketData struct holding the file descriptor and open
 *      input/output streams for the socket. Returns a socket fd of -1 upon
 *      error.
 *
 * REF: the code to connect to an open port is structured based on
 * REF: the sample code in net2.c in the week9 examples on moss.
 */
SocketData connect_to_port(char* portNumber);

/* open_port()
 * -----------
 * Attempts to open a port for listening.
 *
 * portNumber: the service identifier for the port to listen to.
 *
 * return: socket handle for the port if successfull, otherwise -1.
 *
 * REF: code is inspired by the server-multithreaded.c example in week10
 * REF: resources on moss.
 */
int open_port(char* portNumber);

/* block_for_connection()
 * ----------------------
 * Blocks the current thread until a connection is recieved on a socket.
 *
 * socketHandle: the socket fd to wait for a connection on.
 *
 * returns: A SocketData struct containing the fd of the accepted socket as
 *      well as open input/output file streams if successfull. Otherwise a
 *      socket fd of -1 is returned.
 *
 * REF: code is inspired by the server-multithreaded.c example in week10
 * REF: resources on moss.
 */
SocketData block_for_connection(int socketHandle);

#endif // SOCKETUTILS_H

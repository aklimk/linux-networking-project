# linux-networking-project
A lightweight, educational client/server image-processing service written in C. The GitHub repo hosts the implementation for 
both the server and client instances. 1 or more clients stream an image to the server over a simple HTTP interface using TCP sockets; 
the server applies a transform (rotate/flip/scale) and returns the result as a PNG.

# Features
- Uses low-level linux and libc library calls to instantiate and communicate over TCP sockets and ports, to parse arguments, to handle multithreading and to read image data.
- Uses TCP, IPv4 and HTTP style requests as well as low-level sockets and ports for client/server communication.
- Supports image rotation, scale and flipping.
- Provides detailed error handling, including image and networking errors.
- Server is multi-threaded and allows for mutliple simultaneously connected clients. Multithreading uses libc semaphores, mutexes and flags to safely synchronize resources.
- Includes a custom command line argument parser in the client implementation.
- Prints an operating snapshot of connected clients and completed/in-progress image operations on the server recieving "SIGHUP".

# Building
The project was created in a custom remote build environment, so it is not currently buildable.

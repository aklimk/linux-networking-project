#ifndef HTTPUTILS_H
#define HTTPUTILS_H

#include <stdio.h>
#include "ioutils.h"

/* Error codes in common use throughout both client and
 * server programs */
enum HttpCode {
    HTTP_OK = 200,
    METHOD_NOT_ALLOWED = 405,
    OPERATION_NOT_IMPLEMENTED = 501,
    UNPROCESSABLE_IMAGE = 422,
    IMAGE_TOO_LARGE = 413,
    INVALID_OPERATION = 400,
    ADDRESS_NOT_FOUND = 404
};

/* Typical parameters needed to construct a http request */
typedef struct HttpRequest {
    char* type;
    char* address;
    HttpHeader** headers;
    unsigned char* bodyData;
    long unsigned int bodyLen;
} HttpRequest;

/* Typical parameters needed to construct a http response */
typedef struct HttpResponse {
    int status;
    char* statusDescription;
    HttpHeader** headers;
    unsigned char* bodyData;
    long unsigned int bodyLen;
} HttpResponse;

/* send_operations_request()
 * -------------------------
 * Sends a request throught the open socket, socketPost, to do the operations
 *      specified in args, on the image written in input.
 *
 * socketPost: a writable filestream to the server socket.
 * args: the inputs containing the drawing operations to conduct.
 * input: a file stream to read the binary image off of.
 *
 * returns: 0 if succesfull, otherwise the error code.
 */
int send_operations_request(FILE* socketPost, ClientInputs args, FILE* input);

/* write_operations_respnose()
 * ---------------------------
 * Writes the image located in response that originates in socketGet,
 * to the file stream output.
 *
 * socketGet: a readable filestream to the server socket.
 * output: the output file stream to write the retrieved image to.
 *
 * returns: 0 if successfull, otherwise the error code.
 */
int write_operations_response(FILE* socketGet, FILE* output);

/* respond_to_request()
 * --------------------
 * Recieves the http request specified in inHTTP and returns a suitable
 * HTTP response.
 *
 * inHttp: a HttpRequest struct that holds the information for the request
 * imageOps: a shared mutex to increment for each successfull image operation
 *      used for statistics tracking.
 *
 * returns: a HttpResponse containing the information associated with
 *      sending a http response over the network.
 */
HttpResponse respond_to_request(HttpRequest inHttp, Mutex* imageOps);

#endif // HTTPUTILS_H

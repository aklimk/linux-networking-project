#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <csse2310a4.h>

#include "stringutils.h"
#include "ioutils.h"
#include "argparsing.h"
#include "socketutils.h"
#include "httputils.h"

// Error status constants.
const char* const emptyImageMessage
        = "uqimageclient: no data read for input image\n";
const int emptyImageCode = 13;

const char* const noResponseMessage
        = "uqimageclient: server connection closed\n";
const int noResponseCode = 8;

const int invalidStatusCode = 9;

// Default size used in the initilization of some string and binary types.
const int bufferSize = 100;

// Default value to set an c style array buffer.
#define ARRAY_BUFFER_SIZE_DEFAULT 100

// Default rotation to use if the user does not supply any options.
const int defaultRotation = 0;

// Maximum image size that the server can accept from a client;
const unsigned int maxImageSize = 8388608;

/* construct_operations_request()
 * ------------------------------
 * Private helper function for constructing a https request that encodes
 * an image manipulation technique and the image itself.
 *
 * args: the operations to apply to the image.
 * image: a binary buffer of the image to apply the operations to.
 *
 * returns: a binary buffer which corresponds to the constructed http
 *      request, as well as its size in bytes.
 *
 * REF: Mozzila http request structure was used as a guide for the
 * REF: structuring of the html requests.
 * REF: https://developer.mozilla.org/en-US/docs/Web/HTTP/Methods
 */
BinaryData construct_operations_request(ClientInputs args, BinaryData image)
{
    char* address = (char*)calloc(bufferSize, sizeof(char));
    char temp[ARRAY_BUFFER_SIZE_DEFAULT] = {0};

    // Concatenate relevant operations into a '/' deliminated list.
    // Operation arguments are diliminated by ','.
    if (args.hasFlipAxis) {
        sprintf(temp, "/%s,%c", "flip", args.flipAxis);
        strcat(address, temp);
    }
    if (args.hasRotation) {
        sprintf(temp, "/%s,%i", "rotate", args.rotationAngle);
        strcat(address, temp);
    }
    if (args.hasScale) {
        sprintf(temp, "/%s,%i,%i", "scale", args.scaleWidth, args.scaleHeight);
        strcat(address, temp);
    }
    if (!args.hasFlipAxis && !args.hasRotation && !args.hasScale) {
        sprintf(temp, "/%s,%i", "rotate", defaultRotation);
        strcat(address, temp);
    }

    // Allocate a char array for the full request.
    char* httpRequest
            = (char*)malloc(sizeof(char) * (bufferSize + image.length));
    // Add HTTP header, address and image content into char array.
    sprintf(httpRequest, "POST %s HTTP/1.1\nContent-Length: %li\n\n", address,
            image.length);
    free(address);

    // Fill the body of the html request with binary image data.
    int httpRequestLen = strlen(httpRequest);
    for (unsigned int i = 0; i < image.length; i++) {
        httpRequest[httpRequestLen + i] = image.data[i];
    }

    // Wrap data buffer and size in a BinaryData struct so it can be returned.
    BinaryData data
            = {(unsigned char*)httpRequest, httpRequestLen + image.length};
    return data;
}

int send_operations_request(FILE* socketPost, ClientInputs args, FILE* input)
{
    // Read image from specified input stream.
    BinaryData image = read_binary_file(input);
    if (image.length == 0) {
        fprintf(stderr, emptyImageMessage);
        return emptyImageCode;
    }

    // Create http request to manipulate binary image
    // based on cmd args.
    BinaryData request = construct_operations_request(args, image);
    // Write request to server through given socket.
    fwrite(request.data, sizeof(char), request.length, socketPost);
    fflush(socketPost);
    return 0;
}

int write_operations_response(FILE* socketGet, FILE* output)
{
    // Recieve a response to the image manipulation http message.
    int httpStatus;
    char* statusDescription;
    HttpHeader** headers;
    unsigned char* bodyData;
    long unsigned int bodySize;
    int error = !get_HTTP_response(socketGet, &httpStatus, &statusDescription,
            &headers, &bodyData, &bodySize);
    if (error) { // Ill-formed HTTP request.
        fprintf(stderr, noResponseMessage);
        return noResponseCode;
    }
    if (httpStatus != HTTP_OK) { // Coult not transform image.
        fwrite(bodyData, sizeof(char), bodySize, stderr);
        return invalidStatusCode;
    }

    // Write the transformed data to the specified output stream.
    fwrite(bodyData, sizeof(char), bodySize, output);
    return 0;
}

// The following are private constructors, mostly implementing
// data specifications according to the supplied specification.
// Detailed function documentation was not included as they are
// simplisitc data aggregators.

/* Constructor for HTTP response that returns a HTML home page. */
HttpResponse create_home_post_request()
{
    HttpResponse outHttp = {0, NULL, malloc(sizeof(HttpHeader*) * 2), NULL, 0};
    FILE* homeFile
            = fopen("/local/courses/csse2310/resources/a4/home.html", "r");
    BinaryData homeBinary = read_binary_file(homeFile);
    outHttp.status = HTTP_OK; // HTTP status.
    outHttp.statusDescription = copy_string("OK"); // Status explanation.
    HttpHeader* contentType = malloc(sizeof(HttpHeader));
    contentType->name = copy_string("Content-Type"); // Header type.
    contentType->value = copy_string("text/html"); // Header value.
    outHttp.headers[0] = contentType; // Set header.
    outHttp.headers[1] = NULL; // headers is NULL terminated.
    outHttp.bodyData = homeBinary.data; // bodyDat is html binary.
    outHttp.bodyLen = homeBinary.length;
    return outHttp;
}

/* Constructor for HTTP response for when GET address is unkown.
 * Only empty (HOME) get adresses are supported. */
HttpResponse create_not_found_post_request()
{
    HttpResponse outHttp = {0, NULL, malloc(sizeof(HttpHeader*) * 2), NULL, 0};
    outHttp.status = ADDRESS_NOT_FOUND;
    outHttp.statusDescription = copy_string("Not Found");
    HttpHeader* contentType = malloc(sizeof(HttpHeader));
    contentType->name = copy_string("Content-Type");
    contentType->value = copy_string("text/plain");
    outHttp.headers[0] = contentType;
    outHttp.headers[1] = NULL;
    char* message = copy_string("Invalid address\n");
    outHttp.bodyData = (unsigned char*)message;
    outHttp.bodyLen = strlen(message);
    return outHttp;
}

/* Constructor for HTTP response for when operations inputted are invalid
 * or empty. */
HttpResponse create_invalid_op_post_request()
{
    HttpResponse outHttp = {0, NULL, malloc(sizeof(HttpHeader*) * 2), NULL, 0};
    outHttp.status = INVALID_OPERATION;
    outHttp.statusDescription = copy_string("Bad Request");
    HttpHeader* contentType = malloc(sizeof(HttpHeader));
    contentType->name = copy_string("Content-Type");
    contentType->value = copy_string("text/plain");
    outHttp.headers[0] = contentType;
    outHttp.headers[1] = NULL;
    char* msg = copy_string("Invalid operation requested\n");
    outHttp.bodyData = (unsigned char*)msg;
    outHttp.bodyLen = strlen(msg);
    return outHttp;
}

/* Constructor for HTTP response when the image size is too large. */
HttpResponse create_payload_large_post_request(long unsigned int payloadSize)
{
    HttpResponse outHttp = {0, NULL, malloc(sizeof(HttpHeader*) * 2), NULL, 0};
    outHttp.status = IMAGE_TOO_LARGE;
    outHttp.statusDescription = copy_string("Payload Too Large");
    HttpHeader* contentType = malloc(sizeof(HttpHeader));
    contentType->name = copy_string("Content-Type");
    contentType->value = copy_string("text/plain");
    outHttp.headers[0] = contentType;
    outHttp.headers[1] = NULL;
    char* message = malloc(sizeof(char) * bufferSize);
    sprintf(message, "Image received is too large: %li bytes\n", payloadSize);
    outHttp.bodyData = (unsigned char*)message;
    outHttp.bodyLen = strlen(message);
    return outHttp;
}

/* Constructor for HTTP response for when image cannot
 * be laoded into bitmap */
HttpResponse create_unprocessable_post_request()
{
    HttpResponse outHttp = {0, NULL, malloc(sizeof(HttpHeader*) * 2), NULL, 0};
    outHttp.status = UNPROCESSABLE_IMAGE;
    outHttp.statusDescription = copy_string("Unprocessable Content");
    HttpHeader* contentType = malloc(sizeof(HttpHeader));
    contentType->name = copy_string("Content-Type");
    contentType->value = copy_string("text/plain");
    outHttp.headers[0] = contentType;
    outHttp.headers[1] = NULL;
    char* msg = copy_string("Request contains invalid image\n");
    outHttp.bodyData = (unsigned char*)msg;
    outHttp.bodyLen = strlen(msg);
    return outHttp;
}

/* Constructor for HTTP response for when one or more of the image
 * manipulation operations failed. */
HttpResponse create_not_implemented_post_request(char* failCheck)
{
    HttpResponse outHttp = {0, NULL, malloc(sizeof(HttpHeader*) * 2), NULL, 0};
    outHttp.status = OPERATION_NOT_IMPLEMENTED;
    outHttp.statusDescription = copy_string("Not Implemented");
    HttpHeader* contentType = malloc(sizeof(HttpHeader));
    contentType->name = copy_string("Content-Type");
    contentType->value = copy_string("text/plain");
    outHttp.headers[0] = contentType;
    outHttp.headers[1] = NULL;
    char* message = malloc(sizeof(char) * bufferSize);
    sprintf(message, "Operation failed: %s\n", failCheck);
    outHttp.bodyData = (unsigned char*)message;
    outHttp.bodyLen = strlen(message);
    return outHttp;
}

/* Constructor for HTTP response when the HTTP method (E.G GET, POST) was
 * not supported. Only GET and POST methods are supported. */
HttpResponse create_method_disallowed_post_request()
{
    HttpResponse outHttp = {0, NULL, malloc(sizeof(HttpHeader*) * 2), NULL, 0};
    outHttp.status = METHOD_NOT_ALLOWED;
    outHttp.statusDescription = copy_string("Method Not Allowed");
    HttpHeader* contentType = malloc(sizeof(HttpHeader));
    contentType->name = copy_string("Content-Type");
    contentType->value = copy_string("text/plain");
    outHttp.headers[0] = contentType;
    outHttp.headers[1] = NULL;
    char* msg = copy_string("Invalid method on request list\n");
    outHttp.bodyData = (unsigned char*)msg;
    outHttp.bodyLen = strlen(msg);
    return outHttp;
}

/* Constructor for HTTP response for when image was succesfully manipulated
 * and needs to be returned to the client. */
HttpResponse create_image_return_post_request(FIBITMAP* bitmap)
{
    HttpResponse outHttp = {0, NULL, malloc(sizeof(HttpHeader*) * 2), NULL, 0};
    outHttp.status = HTTP_OK;
    outHttp.statusDescription = copy_string("OK");
    HttpHeader* contentType = malloc(sizeof(HttpHeader));
    contentType->name = copy_string("Content-Type");
    contentType->value = copy_string("image/png");
    outHttp.headers[0] = contentType;
    outHttp.headers[1] = NULL;

    unsigned long dataLen;
    unsigned char* data = fi_save_png_image_to_buffer(bitmap, &dataLen);
    outHttp.bodyData = data;
    outHttp.bodyLen = dataLen;
    return outHttp;
}

HttpResponse respond_to_request(HttpRequest inHttp, Mutex* imageOps)
{
    HttpResponse outHttp = {0};
    if (!strcmp(inHttp.type, "GET")) {
        // GET request with empty address indicates home page.
        if (strlen(inHttp.address) == 1 && !strcmp(inHttp.address, "/")) {
            outHttp = create_home_post_request();
        } else { // No other GET requests supported.
            outHttp = create_not_found_post_request();
        }
    } else if (!strcmp(inHttp.type, "POST")) {
        // Parse POST request address as a '/' deliminated options list.
        CommandBuffer cmdBuffer
                = create_image_processing_command_buffer(inHttp.address);

        // If no operations are specified, or a parsing error occured
        // return 400.
        if (cmdBuffer.parseError || cmdBuffer.numCmds == 0) {
            outHttp = create_invalid_op_post_request();
        } else if (inHttp.bodyLen > maxImageSize) { // Large image, return 413.
            outHttp = create_payload_large_post_request(inHttp.bodyLen);
        } else { // Operations appear valid.
            // Attempt to load binary image data into a cross-platform
            // bitmap format.
            FIBITMAP* bitmap = fi_load_image_from_buffer(
                    inHttp.bodyData, inHttp.bodyLen);
            if (!bitmap) { // Failed to load image into bitmap.
                outHttp = create_unprocessable_post_request();
            } else {
                // Bitmap was succesfully created. Attempt operations.
                char* failCheck = apply_cmd_buffer_to_image(
                        &bitmap, cmdBuffer, imageOps);
                if (failCheck) { // One or more of the operations failed.
                    outHttp = create_not_implemented_post_request(failCheck);
                } else { // All operations completed successfully.
                    outHttp = create_image_return_post_request(bitmap);
                }
            }
        }
    } else { // HTTP method was not GET or POST. No other methods supported.
        outHttp = create_method_disallowed_post_request();
    }

    // Return constructed http response.
    return outHttp;
}

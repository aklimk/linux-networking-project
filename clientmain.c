#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <csse2310a4.h>

#include "argparsing.h"
#include "ioutils.h"
#include "socketutils.h"
#include "httputils.h"

// Error status constants.
const char* const invalidCmdMessage
        = "Usage: uqimageclient portnumber [--input infile] [--out "
          "outfilename] [--scale w h | --flip dirn | --rotate angle]\n";
const int invalidCmdCode = 7;

const char* const invalidPortFormat
        = "uqimageclient: unable to establish connection to port \"%s\"\n";
const int invalidPortCode = 17;

/* Entry point for the client prgram */
int main(int argc, char** argv)
{
    // Parse and validate inputs.
    ClientInputs args = parse_client_inputs(argc, argv);
    if (args.error) {
        fprintf(stderr, invalidCmdMessage);
        return invalidCmdCode;
    }
    int error = check_client_inputs_validity(args);
    if (error) {
        return error;
    }

    // Open input and output to source if path present and valid.
    FILE* inputSource = stdin;
    FILE* outputSource = stdout;
    error = check_client_inputs_validity(args);
    if (error) {
        return error;
    }
    if (args.inputFilePath) {
        inputSource = fopen(args.inputFilePath, "r");
    }
    if (args.outputFilePath) {
        outputSource = fopen(args.outputFilePath, "w");
    }

    // Open a socket to the specified port, returning error if not possible.
    SocketData socketData = connect_to_port(args.portNumber);
    if (socketData.handle == -1) {
        fprintf(stderr, invalidPortFormat, args.portNumber);
        return invalidPortCode;
    }

    // Attempt to send a http request to apply the operations specified in
    // argv to the image specified in the input source.
    error = send_operations_request(socketData.post, args, inputSource);
    if (error) {
        return error;
    }

    // Attempt to write the server response to the previous request into
    // the file stream specified in the output source.
    error = write_operations_response(socketData.get, outputSource);
    if (error) {
        return error;
    }
    return 0;
}

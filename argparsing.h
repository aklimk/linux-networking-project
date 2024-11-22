#ifndef ARGPARSING_H
#define ARGPARSING_H

#include <stdbool.h>

/* Holds values of parsed and formatted client program inputs */
typedef struct ClientInputs {
    bool error;
    char* portNumber;
    char* inputFilePath;
    char* outputFilePath;
    int rotationAngle;
    char flipAxis;
    int scaleWidth;
    int scaleHeight;
    bool hasRotation;
    bool hasFlipAxis;
    bool hasScale;
} ClientInputs;

/* parse_client_inputs()
 * ---------------------
 * Parses a command line into formatted client input, checking for validity.
 *
 * argc: number of arguments.
 * argv: array of argument strings.
 *
 * returns: a ClientInputs struct holding the command line inputs. Error is
 *      switched if the parser encountered a comand line validity issue.
 */
ClientInputs parse_client_inputs(int argc, char** argv);

/* check_client_inputs_validity()
 * -----------------------------
 * Ensures that specfied file paths are readable or writiable.
 *
 * args: inputs to test.
 *
 * Returns: 0 upon success, otherwise the error code.
 */
int check_client_inputs_validity(ClientInputs args);

/* Holds the possible command line inputs to the server application.
 * A positive error value indicates a parsing error. */
typedef struct ServerInputs {
    bool error;
    int maxConnections;
    char* port;
} ServerInputs;

/* parse_server_inputs()
 * ---------------------
 * Parses server inputs from their argument strings into thier natural format.
 *
 * argc: number of arguments.
 * argv: lsit of argument strings.
 *
 * returns: A ServerInputs struct that holds the parsed inputs. A positive
 *      error value indicats a parsing error has occured.
 */
ServerInputs parse_server_inputs(int argc, char** argv);

/* Holds a series of commands for image manipulation */
typedef struct CommandBuffer {
    bool parseError;
    int* buffer;
    int numCmds;
} CommandBuffer;

/* Constants that describe values inside the command buffer */
/* REF: based on ed lesson's lesson on enums */
enum CommandTypes { CMD_ROTATE, CMD_FLIP, CMD_SCALE };

/* Constants that map cmd buffer values to real flip parameters */
/* REF: based on ed lesson's lesson on enums */
enum FlipTypes { FLIP_HORIZONTAL, FLIP_VERTICAL };

/* create_image_processing_command_buffer()
 * ----------------------------------------
 * Interprests a http address into a series of integer command representations.
 *
 * address: the http address to parse into a command buffer.
 *
 * Returns: a CommandBuffer struct where the buffer array holds both
 *      the commands and thier parameters in a packed manner.
 *      These values can be reintepreted as they have known parameter
 *      counts. Returns a parseError value of true if address has invalid
 *      formatting.
 */
CommandBuffer create_image_processing_command_buffer(char* address);

#endif // ARGPARSING_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "stringutils.h"
#include "argparsing.h"
#include "ioutils.h"

// Possible command line options which share a similar format.
const char* const cmdOptions[] = {"--input", "--out", "--rotate", "--flip"};
const int optionsCount = 4;

// Sentinal value for an error response in an integer function.
const int intSentinal = -1000000;

// Error status constants.
const char* const invalidInputFormat
        = "uqimageclient: unable to read from file \"%s\"\n";
const int invalidInputCode = 2;

const char* const invalidOutputFormat
        = "uqimageclient: unable to open file \"%s\" for writing\n";
const int invalidOutputCode = 15;

// Inclusive bounds for rotation value.
const int rotationMin = -359;
const int rotationMax = 359;

// Inclusive bounds for both width and height of scaling.
const int scalingMin = 1;
const int scalingMax = 10000;

// Maximum value for server --max option.
const int maxConnectionsMax = 10000;

// Standard base for integer conversion to formatted units.
const int intBase = 10;

// Default initilization size for command buffer.
const int cmdBufferDefaultSize = 100;

// Number of params featured after given option.
const int numRotateArgs = 2;
const int numFlipArgs = 2;
const int numScalingArgs = 3;

/* Private struct that holds string addresses to parsed but
 * unformatted client program inputs */
typedef struct ClientInputsRaw {
    bool error;
    char* portNumber;
    char* inputFilePath;
    char* outputFilePath;
    char* rotationAngle;
    char* flipAxis;
    char* scaleWidth;
    char* scaleHeight;
} ClientInputsRaw;

/* parse_raw_client_inputs()
 * ---------------------
 * Private helper function that parses
 * inputs to the client program, from the terminal. Also checks for
 * common validity errors in command line structure.
 *
 * argc: number of arguments.
 * argv: array of argument strings.
 *
 * Returns: a ClientInputsRaw struct that holds all potential client inputs
 *      available. Some fields may be empty.
 */
ClientInputsRaw parse_raw_client_inputs(int argc, char** argv)
{
    ClientInputsRaw argsRaw = {0};
    char** argPointers[] = {&argsRaw.inputFilePath, &argsRaw.outputFilePath,
            &argsRaw.rotationAngle, &argsRaw.flipAxis};

    // Ensure that first argument is a portNumber.
    if (argc < 2 || !strcmp(argv[1], "--") || !strcmp(argv[1], "")) {
        argsRaw.error = true;
        return argsRaw;
    }
    argsRaw.portNumber = argv[1];

    for (int i = 2; i < argc; i++) {
        // Attempt to find option within valid options.
        int indexCheck
                = get_string_array_index(cmdOptions, optionsCount, argv[i]);
        if (indexCheck == -1) {
            // Scale is formatted differently and is thus individual parsed.
            if (!strcmp(argv[i], "--scale")) {
                // Not enough arguments or scale already specified.
                if (i + 2 >= argc || argsRaw.scaleWidth) {
                    argsRaw.error = true;
                    return argsRaw;
                }
                argsRaw.scaleWidth = argv[i + 1];
                argsRaw.scaleHeight = argv[i + 2];
                i += 2;
                continue;
            }
            argsRaw.error = true;
            return argsRaw;
        }

        // Check if value already set, indicating duplicate.
        if (*argPointers[indexCheck]) {
            argsRaw.error = true;
            return argsRaw;
        }
        // Every option requires a non-empty parameter.
        if (i + 1 >= argc || !strlen(argv[i + 1])) {
            argsRaw.error = true;
            return argsRaw;
        }
        // Set value of input option and skip over next iteration, as it is
        // a parameter.
        *argPointers[indexCheck] = argv[i + 1];
        i++;
    }
    return argsRaw;
}

/* get_axis()
 * ----------
 * Private helper function that retrieves a formatted value for the
 * flip option. Returns 0 upon error.
 *
 * args: flipAxis a string that holds the axis flip paramter.
 *
 * Returns: 'v' for vertical flip, 'h' for horizontal flip, or 0 for error.
 */
char get_axis(char* flipAxis)
{
    // Expects only a singular character in the string.
    if (strlen(flipAxis) > 1) {
        return 0;
    }
    if (flipAxis[0] != 'h' && flipAxis[0] != 'v') {
        return 0;
    }
    return flipAxis[0];
}

/* get_rotation()
 * --------------
 * Private helper function that formats a rotation string into a integer.
 *
 * rotationAngle: string containing the rotation to apply to an image.
 *      rotationAngle must be between -359 and 359 inclusive.
 *
 * Returns: rotation in degrees if value is a valid integer, otherwise
 *      intSentinal.
 */
int get_rotation(char* rotationAngle)
{
    char* endPtr;
    int rotationAngleInt = strtol(rotationAngle, &endPtr, intBase);
    // endPtr being at the start indicates a invalid value.
    if (endPtr - rotationAngle == 0) {
        return intSentinal;
    }
    if (rotationAngleInt < rotationMin || rotationAngleInt > rotationMax) {
        return intSentinal;
    }
    return rotationAngleInt;
}

/* Private struct that holds a width and a height */
typedef struct Extent {
    int width;
    int height;
} Extent;

/* get_extent()
 * ------------
 * Private helper function that retrieves a width and a height from strings.
 *
 * width: string representing and integer width. Must be between 1 and 10000
 *      inclusive.
 * height: string representing and integer height. Must be between 1 and 10000
 *      inclusive.
 *
 * Return: Extent struct containing formatted width and height, or intSentinal
 *      value(s) in said struct upon error.
 */
Extent get_extent(char* width, char* height)
{
    char* endPtr;
    // Start extent with invalid, sentinal values.
    Extent extent = {intSentinal, intSentinal};
    int scaleWidth = strtol(width, &endPtr, intBase);
    // The endptr being at the start indicates a formatting error.
    if (endPtr - width == 0) {
        return extent;
    }
    extent.width = scaleWidth;

    int scaleHeight = strtol(height, &endPtr, intBase);
    if (endPtr - height == 0) {
        return extent;
    }
    extent.height = scaleHeight;

    if (extent.width > scalingMax || extent.height > scalingMin) {
        extent.width = intSentinal;
        extent.height = intSentinal;
    }
    if (extent.width < scalingMin || extent.height < scalingMin) {
        extent.width = intSentinal;
        extent.height = intSentinal;
    }
    return extent;
}

/* copy_from_raw()
 * ---------------
 * Private helper function that copies easily intepretable values from a
 *      ClientInputsRaw struct into a new ClientInputs struct.
 *
 * argsRaw: The raw arguments to copy.
 *
 * returns: A ClientInputs struct where the boolean fields, and the fields
 *      that share type with their raw counterpart, are copied over.
 */
ClientInputs copy_from_raw(ClientInputsRaw argsRaw)
{
    ClientInputs args = {0};
    args.portNumber = argsRaw.portNumber;
    args.inputFilePath = argsRaw.inputFilePath;
    args.outputFilePath = argsRaw.outputFilePath;
    args.hasRotation = argsRaw.rotationAngle;
    args.hasFlipAxis = argsRaw.flipAxis;
    args.hasScale = argsRaw.scaleWidth;
    return args;
}

ClientInputs parse_client_inputs(int argc, char** argv)
{
    ClientInputsRaw argsRaw = parse_raw_client_inputs(argc, argv);
    ClientInputs args = {0};
    if (argsRaw.error) {
        args.error = true;
        return args;
    }
    args = copy_from_raw(argsRaw);

    // Retrieve flip axis if available.
    if (args.hasFlipAxis) {
        char axis = get_axis(argsRaw.flipAxis);
        if (axis == 0) {
            args.error = true;
            return args;
        }
        args.flipAxis = axis;
    }

    // Retrieve rotation if available.
    if (args.hasRotation) {
        int rotation = get_rotation(argsRaw.rotationAngle);
        if (rotation == intSentinal) {
            args.error = true;
            return args;
        }
        args.rotationAngle = rotation;
    }

    // Retrieve scaling parameters if available.
    if (args.hasScale) {
        Extent extent = get_extent(argsRaw.scaleWidth, argsRaw.scaleHeight);
        if (extent.width == intSentinal || extent.height == intSentinal) {
            args.error = true;
            return args;
        }
        args.scaleWidth = extent.width;
        args.scaleHeight = extent.height;
    }

    // Check if total number of rotation, flip, scale set is more than 1.
    if ((int)args.hasFlipAxis + (int)args.hasRotation + (int)args.hasScale
            > 1) {
        args.error = true;
    }
    return args;
}

int check_client_inputs_validity(ClientInputs args)
{
    if (args.inputFilePath) {
        if (!file_is_valid(args.inputFilePath, "r")) {
            fprintf(stderr, invalidInputFormat, args.inputFilePath);
            return invalidInputCode;
        }
    }
    if (args.outputFilePath) {
        if (!file_is_valid(args.outputFilePath, "w")) {
            fprintf(stderr, invalidOutputFormat, args.outputFilePath);
            return invalidOutputCode;
        }
    }
    return 0;
}

ServerInputs parse_server_inputs(int argc, char** argv)
{
    ServerInputs args = {false, -1, NULL};
    for (int i = 1; i < argc; i++) {
        if (i + 1 >= argc) { // All arguments must have a parameter.
            args.error = true;
            return args;
        }
        if (!strcmp(argv[i], "--max")) {
            // Parsing error if value already set or string is empty.
            if (args.maxConnections != -1 || !strlen(argv[i + 1])) {
                args.error = true;
                return args;
            }
            char* endPtr;
            args.maxConnections = strtol(argv[i + 1], &endPtr, intBase);
            i++;
            // endPtr being at the start indicates an error.
            if (endPtr - argv[i + 1] == 0) {
                args.error = true;
                return args;
            }
        } else if (!strcmp(argv[i], "--port")) {
            // Parsing error if value already set of string is empty.
            if (args.port || !strlen(argv[i + 1])) {
                args.error = true;
                return args;
            }
            args.port = argv[i + 1];
            i++;
        } else { // Option unrecognized, must be --max or --port.
            args.error = true;
            return args;
        }
    }

    // Ensure that the --max value was within bounds.
    // Parsining error if it is out of bounds.
    if (args.maxConnections > maxConnectionsMax) {
        args.error = true;
    }

    return args;
}

/* parse_rotation_cmd()
 * ------------------
 * Private helper function that parses a rotation option and its parameter
 *      into a command buffer pointed to by cmdBuffer.
 *
 * cmdBuffer: pointer to the command buffer to populate.
 * arg: the list of arg strings to parse.
 * argSize: length of the arg list.
 *
 * returns: 0 if successfull, 1 otherwise.
 */
int parse_rotation_cmd(CommandBuffer* cmdBuffer, char** arg, int argSize)
{
    if (argSize != numRotateArgs) {
        return 1;
    }
    cmdBuffer->buffer[cmdBuffer->numCmds] = CMD_ROTATE;
    cmdBuffer->numCmds++;
    char* endPtr;
    int rotation = strtol(arg[1], &endPtr, intBase);
    // endPtr being at the start indicates an error.
    if (endPtr - arg[1] == 0) {
        return 1;
    }
    // Rotation should be between -359 and 359 inclusive.
    if (rotation < rotationMin || rotation > rotationMax) {
        return 1;
    }
    cmdBuffer->buffer[cmdBuffer->numCmds] = rotation;
    cmdBuffer->numCmds++;
    return 0;
}

/* parse_flip_cmd()
 * ------------------
 * Private helper function that parses a flip option and its parameter into
 *      a command buffer pointed to by cmdBuffer.
 *
 * cmdBuffer: pointer to the command buffer to populate.
 * arg: the list of arg strings to parse.
 * argSize: length of the arg list.
 *
 * returns: 0 if successfull, 1 otherwise.
 */
int parse_flip_cmd(CommandBuffer* cmdBuffer, char** arg, int argSize)
{
    if (argSize != numFlipArgs) {
        return 1;
    }
    // Flip paramters should be a single char.
    if (strlen(arg[1]) != 1) {
        return 1;
    }
    cmdBuffer->buffer[cmdBuffer->numCmds] = CMD_FLIP;
    cmdBuffer->numCmds++;
    if (arg[1][0] == 'h') {
        cmdBuffer->buffer[cmdBuffer->numCmds] = FLIP_HORIZONTAL;
        cmdBuffer->numCmds++;
    } else if (arg[1][0] == 'v') {
        cmdBuffer->buffer[cmdBuffer->numCmds] = FLIP_VERTICAL;
        cmdBuffer->numCmds++;
    } else { // Unrecognized flip option.
        return 1;
    }
    return 0;
}

/* parse_scaling_cmd()
 * ------------------
 * Private helper function that parses a scaling option and
 *      its two parameters into a command buffer pointed to by cmdBuffer.
 *
 * cmdBuffer: pointer to the command buffer to populate.
 * arg: the list of arg strings to parse.
 * argSize: length of the arg list.
 *
 * returns: 0 if successfull, 1 otherwise.
 */
int parse_scaling_cmd(CommandBuffer* cmdBuffer, char** arg, int argSize)
{
    if (argSize != numScalingArgs) {
        return 1;
    }
    cmdBuffer->buffer[cmdBuffer->numCmds] = CMD_SCALE;
    cmdBuffer->numCmds++;
    char* endPtr;

    // Save width to command buffer.
    int width = strtol(arg[1], &endPtr, intBase);
    if (endPtr - arg[1] == 0) {
        return 1;
    }
    if (width < scalingMin || width > scalingMax) {
        return 1;
    }
    cmdBuffer->buffer[cmdBuffer->numCmds] = width;
    cmdBuffer->numCmds++;

    // Save height to command buffer.
    int height = strtol(arg[2], &endPtr, intBase);
    if (endPtr - arg[2] == 0) {
        return 1;
    }
    if (height < scalingMin || height > scalingMax) {
        return 1;
    }
    cmdBuffer->buffer[cmdBuffer->numCmds] = height;
    cmdBuffer->numCmds++;
    return 0;
}

CommandBuffer create_image_processing_command_buffer(char* address)
{
    // Split address into list of strings based on delimiter '/'.
    char** args = split_by_char(address, '/', 0);

    CommandBuffer cmdBuffer
            = {false, malloc(sizeof(int) * cmdBufferDefaultSize), 0};
    int i = 1;
    while (args[i] != NULL) {
        // Futher split string into list of string arguments based on
        // delimiter ','.
        char** arg = split_by_char(args[i], ',', 0);
        // Calculates the length of the argument list for validity checking.
        int argSize = 0;
        while (arg[argSize] != NULL) {
            argSize++;
        }
        if (!strcmp(arg[0], "rotate")) {
            int error = parse_rotation_cmd(&cmdBuffer, arg, argSize);
            if (error) {
                cmdBuffer.parseError = true;
                return cmdBuffer;
            }
        } else if (!strcmp(arg[0], "flip")) {
            int error = parse_flip_cmd(&cmdBuffer, arg, argSize);
            if (error) {
                cmdBuffer.parseError = true;
                return cmdBuffer;
            }
        } else if (!strcmp(arg[0], "scale")) {
            int error = parse_scaling_cmd(&cmdBuffer, arg, argSize);
            if (error) {
                cmdBuffer.parseError = true;
                return cmdBuffer;
            }
        } else {
            cmdBuffer.parseError = true;
            return cmdBuffer;
        }
        i++;
    }
    return cmdBuffer;
}

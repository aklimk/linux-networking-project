#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <csse2310_freeimage.h>
#include <FreeImage.h>
#include <csse2310a4.h>

#include "stringutils.h"
#include "argparsing.h"
#include "ioutils.h"

// The amount to increase the binary buffer by in each reallocation.
const long unsigned int sizeGuess = 10000;

BinaryData read_binary_file(FILE* binaryFile)
{
    unsigned char* buffer = malloc(sizeof(unsigned char) * sizeGuess);
    long unsigned int length = 0;

    // Read from binary file until EOF.
    int checkEOF = fgetc(binaryFile);
    while (checkEOF != EOF) {
        buffer[length] = checkEOF;
        length++;
        checkEOF = fgetc(binaryFile);
        // Every sizeGuess number of iterations, reallocate buffer.
        if (length % sizeGuess == 0) {
            buffer = realloc(
                    buffer, sizeof(unsigned char) * (length + sizeGuess));
        }
    }
    BinaryData binaryData = {buffer, length};
    return binaryData;
}

bool file_is_valid(char* filePath, char* accessMode)
{
    if (filePath) {
        FILE* file = fopen(filePath, accessMode);
        if (!file) { // File could not be opened.
            return false;
        }
        fclose(file);
    }
    return true;
}

char* apply_cmd_buffer_to_image(
        FIBITMAP** bitmap, CommandBuffer cmdBuffer, Mutex* imageOps)
{
    char* failCheck = NULL;
    // Loop through command array, dispatching FreeImage operations depening
    // on the recieved value.
    for (int i = 0; i < cmdBuffer.numCmds; i++) {
        if (cmdBuffer.buffer[i] == CMD_ROTATE) {
            *bitmap = FreeImage_Rotate(
                    *bitmap, (double)cmdBuffer.buffer[i + 1], NULL);
            if (!(*bitmap)) { // Rotation operation not permitted.
                failCheck = "rotate";
                break;
            }
            // i + 1 was the parameter of rotation, so skip for next iteration.
            i++;
        } else if (cmdBuffer.buffer[i] == CMD_FLIP) {
            if (cmdBuffer.buffer[i + 1] == FLIP_HORIZONTAL) {
                int32_t error = !FreeImage_FlipHorizontal(*bitmap);
                if (error) { // Flip operation not permitted.
                    failCheck = "flip";
                    break;
                }
            } else if (cmdBuffer.buffer[i + 1] == FLIP_VERTICAL) {
                int32_t error = !FreeImage_FlipVertical(*bitmap);
                if (error) { // Flip operation not permitted.
                    failCheck = "flip";
                    break;
                }
            }
            // i + 1 was the parameter of flip axis so skip for next iteration.
            i++;
        } else if (cmdBuffer.buffer[i] == CMD_SCALE) {
            *bitmap = FreeImage_Rescale((*bitmap), cmdBuffer.buffer[i + 1],
                    cmdBuffer.buffer[i + 2], FILTER_BILINEAR);
            if (!(*bitmap)) { // Scale operation not permitted.
                failCheck = "scale";
                break;
            }
            // i + 1, i + 2 were the paramters of scaling, so skip over them
            // for the next iteration.
            i += 2;
        }
        // Record as a successfull operation, as it would have brocken out
        // of the loop if it failed.
        modify_mutex(imageOps, 1);
    }
    return failCheck;
}

void modify_mutex(Mutex* mutex, int change)
{
    sem_wait(&(mutex->lock)); // Lock mutex.
    mutex->value += change; // Change value.
    sem_post(&(mutex->lock)); // Unlock mutex.
}

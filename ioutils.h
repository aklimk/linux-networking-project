#ifndef IOUTILS_H
#define IOUTILS_H

#include <stdbool.h>
#include <stdio.h>
#include <semaphore.h>

#include <csse2310_freeimage.h>
#include <FreeImage.h>
#include <csse2310a4.h>

#include "argparsing.h"

/* Simple structure that holds a binary unsigned char array
 * and its length. */
typedef struct BinaryData {
    unsigned char* data;
    long unsigned int length;
} BinaryData;

/* read_binary_file()
 * ------------------
 * Returns the contents of a binary file stream pointed to by binaryFile.
 *
 * binaryFile: the file to read from.
 *
 * Returns: a BinaryData struct containing the binary data in a
 *      heap allocated unsigned char array, and the length of the data.
 */
BinaryData read_binary_file(FILE* binaryFile);

/* is_file_valid()
 * ---------------
 * Checks if file can be read/written from. Returns true if it can,
 *      otherwise, false.
 *
 * filePath: the file of the path to check.
 *
 * Returns: true if file can be read from, false otherwise.
 */
bool file_is_valid(char* filePath, char* accessMode);

/* Associates an integer with a semaphore that can lock and unlock
 * acess to it.
 *
 * REF: semaphore implementation based on race3.c in week8
 * REF: resources on moss
 */
typedef struct Mutex {
    int value;
    sem_t lock;
} Mutex;

/* apply_cmd_buffer_to_image()
 * ---------------------------
 * Applies the commands specified in cmdBuffer, to the image specified in
 *      bitmap. On sucess returns NULL, on fail returns the operation that
 *      resulted in an error.
 *
 * bitmap: device independent FreeImage representation of an image.
 * cmdBuffer: a CommandBuffer object, holding a sequence of commands to
 *      perform.
 *
 * Returns: NULL if all commands succeeded, the name of the failed command
 *      type otherwise.
 */
char* apply_cmd_buffer_to_image(
        FIBITMAP** bitmap, CommandBuffer cmdBuffer, Mutex* imageOps);

/* modify_mutex()
 * --------------
 * Changes the value held by the int mutex, locking and unlocking it as needed.
 *
 * mutex: a pointer to the mutex to modify.
 * change: the amount to change the held value by.
 *
 * REF: Sempahore handling based on race3.c week8 moss code example.
 */
void modify_mutex(Mutex* mutex, int change);

#endif // IOUTILS_H

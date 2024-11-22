#include <string.h>
#include <stdlib.h>

#include "stringutils.h"

int get_string_array_index(const char* const* array, int arraySize, char* elem)
{
    for (int i = 0; i < arraySize; i++) {
        if (!strcmp(array[i], elem)) {
            return i;
        }
    }
    return -1;
}

char* copy_string(const char* const string)
{
    char* copy = (char*)malloc(sizeof(char) * (strlen(string) + 1));
    strcpy(copy, string);
    return copy;
}

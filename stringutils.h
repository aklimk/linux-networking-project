#ifndef STRINGUTILS_H
#define STRINGUTILS_H

/* get_string_array_index()
 * ------------------------
 * Returns the index of the first instance of elem in array, if it exists.
 *
 * array: an array of strings.
 * arraySize: size of `array`.
 * elem: string to search for.
 *
 * Returns: index of the first instance of elem in array, otherwsie -1 if
 *      elem is not in array.
 */
int get_string_array_index(const char* const* array, int arraySize, char* elem);

/* copy_string()
 * -------------
 * Returns a heap allocated string from the input string.
 *
 * string: the string to copy.
 *
 * returns: a heap allocated string that is a copy of string.
 */
char* copy_string(const char* const string);

#endif // STRINGUTILS_H

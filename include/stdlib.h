#ifndef MPX_STDLIB_H
#define MPX_STDLIB_H

/**
 @file stdlib.h
 @brief A subset of standard C library functions.
*/

/**
 Convert an ASCII string to an integer
 @param s A NUL-terminated string
 @return The value of the string converted to an integer
*/
int atoi(const char *s);

void itoa(int num, char* str, int base);

#endif

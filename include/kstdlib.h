#pragma once

#include <stddef.h>
#include <stdint.h>

// Compares the c strings str2 and str2
// Continues comparison while the characters in comparison are equal,
// or a null byte is encountered
// returns 0 on equal, 
// <0 if the non-matching char has a lower value in str1 than str2
size_t kstrcmp(const char* ptr1, const char* ptr2);

// Compares the first num bytes of the block of memory pointed to by ptr1
// to that of ptr2. Returns zero if equal, or a non-zero value representing
// which is greater.
// **Note: unlike strcmp, this function does not stop comparing after finding
size_t kmemcmp(const void* ptr1, const void* ptr2, size_t num);

// Copies num chars from src to dest
// returns the number of chars written
size_t kmemcpy(void* dest, const void* src, size_t num);

// Copies chars from src to destination until a null character is hit
// returns the number of chars written
size_t kstrcpy(char* dest, const char* src);

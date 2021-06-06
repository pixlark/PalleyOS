//
// C standard library I/O operations, but for the kernel
//

#ifndef KSTDIO_H
#define KSTDIO_H

#include <stdarg.h>

void kvsprintf(char* result, const char* format, va_list args);
void ksprintf(char* result, const char* format, ...);
void kvprintf(const char* format, va_list args);
int kprintf(const char* format, ...);

#endif

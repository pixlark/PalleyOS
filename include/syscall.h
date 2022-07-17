#pragma once

#include <stddef.h>
#include <stdint.h>

#define SYSCALL(name) void (name)(va_list args)
#define TERMINAL_WRITE_SYSCALL 0x16

void syscalls_init();
extern void terminal_write(const char*);


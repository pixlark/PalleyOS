#pragma once

#include <stddef.h>
#include <stdint.h>

#define SYSCALL(name) void (name)(va_list args)

void syscalls_init();
extern void __attribute__((__cdecl__)) syscall(int syscall_index, ...);

#ifndef CPUID_H
#define CPUID_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void print_cpuid_vendor();
void load_cpuid();
void load_cpuid_features(intptr_t, intptr_t);

#endif

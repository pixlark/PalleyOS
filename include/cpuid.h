#ifndef CPUID_H
#define CPUID_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void cpuidPrintVendor();
void loadCpuid();
void cpuidLoadFeatures(intptr_t, intptr_t);

#endif

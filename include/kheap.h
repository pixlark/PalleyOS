#pragma once

#define KHEAP_START 0x0f00000
#define KHEAP_END   0x0ffe000

void  kheapInit();
void* kheapAlloc(size_t size);
void* kheapAlignedAlloc(size_t size, size_t alignment);
void* kheapRealloc(void* ptr, size_t size);
void  kheapFree(void* ptr);
void  kheapDump();

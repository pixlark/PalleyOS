#pragma once

#define KHEAP_START 0xf000000
#define KHEAP_END   0xfffffff

void  kheapInit();
void* kheapAlloc(size_t size);
void* kheapAlignedAlloc(size_t size, size_t alignment);
void* kheapRealloc(void* ptr, size_t size);
void  kheapFree(void* ptr);
void  kheapDump();

#pragma once

#define KHEAP_START 0xf000000
#define KHEAP_END   0xfffffff

void initialize_kheap();
void* kheap_alloc(uint32_t size);
void kheap_free(void* ptr);
void dump_heap();

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kheap.h>
#include <kstdio.h>

#define KHEAP_MAGIC 0x7ea4

typedef struct HeapNode {
    uint16_t magic_number;
    uint32_t size;
    struct HeapNode* next_node;
    bool allocated;
} __attribute__((packed)) HeapNode;

#define NODE_TO_CONTENTS(ptr) (((HeapNode*) (ptr)) + 1)
#define CONTENTS_TO_NODE(ptr) (((HeapNode*) (ptr)) - 1)

static HeapNode* root_node;

void initialize_kheap() {
    kprintf("sizeof(HeapNode): %d\n", sizeof(HeapNode));
    root_node = (HeapNode*) KHEAP_START;
    root_node->magic_number = KHEAP_MAGIC;
    root_node->size = (KHEAP_END - KHEAP_START) - sizeof(HeapNode);
    root_node->next_node = NULL;
    root_node->allocated = false;
}

void* kheap_alloc(uint32_t size) {
    HeapNode* iter = root_node;
    while (iter != NULL) {
        if (iter->magic_number != KHEAP_MAGIC) {
            kprintf("Kernel heap was corrupted!\n");
            while (true);
        }
        if (!iter->allocated && iter->size >= size) {
            // Allocate!
            uint32_t space_remaining_after_allocation
                = iter->size - size;
            if (space_remaining_after_allocation > sizeof(HeapNode)) {
                // Create a new node
                uint8_t* byte_ptr = (uint8_t*) NODE_TO_CONTENTS(iter);
                byte_ptr += size;
                HeapNode* next_node = (HeapNode*) byte_ptr;
                next_node->magic_number = KHEAP_MAGIC;
                next_node->size
                    = space_remaining_after_allocation - sizeof(HeapNode);
                next_node->next_node = iter->next_node;
                next_node->allocated = false;
                // Rewrite current node
                iter->size = size;
                iter->next_node = next_node;
                iter->allocated = true;
                return NODE_TO_CONTENTS(iter);
            }
        }
        // Otherwise
        iter = iter->next_node;
    }
    // Didn't find any space!
    kprintf("Kernel heap ran out of space!\n");
    while (true);
}

void compact_heap() {
    HeapNode* iter = root_node;
    while (iter != NULL) {
        if (iter->next_node == NULL) {
            break;
        }
        if (!iter->allocated && !iter->next_node->allocated) {
            // Combine free space
            ptrdiff_t diff = ((uint8_t*) iter->next_node) - ((uint8_t*) iter);
            iter->size = (diff - sizeof(HeapNode))
                       + (sizeof(HeapNode) + iter->next_node->size);
            iter->next_node = iter->next_node->next_node;
            continue;
        }
        iter = iter->next_node;
    }
}

void kheap_free(void* ptr) {
    HeapNode* node = CONTENTS_TO_NODE(ptr);
    node->allocated = false;
    compact_heap();
}

void kheap_dump() {
    HeapNode* iter = root_node;
    while (iter != NULL) {
        kprintf("Node (%x):\n  Magic: %x\n  Next: %x\n  Size: %d\n  Alloc: %s\n", iter, iter->magic_number, iter->next_node, iter->size, iter->allocated ? "yes" : "no");
        iter = iter->next_node;
    }
}

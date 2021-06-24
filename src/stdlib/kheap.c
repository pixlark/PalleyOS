#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kheap.h>
#include <kstdio.h>
#include <kstdlib.h>

#define KHEAP_MAGIC 0x7ea4

typedef struct HeapNode {
    uint16_t magic_number;
    size_t size;
    struct HeapNode* next_node;
    bool allocated;
} __attribute__((packed)) HeapNode;

#define NODE_TO_CONTENTS(ptr) (((HeapNode*) (ptr)) + 1)
#define CONTENTS_TO_NODE(ptr) (((HeapNode*) (ptr)) - 1)

static HeapNode* root_node;

void kheapInit() {
    kprintf("sizeof(HeapNode): %d\n", sizeof(HeapNode));
    root_node = (HeapNode*) KHEAP_START;
    root_node->magic_number = KHEAP_MAGIC;
    root_node->size = (KHEAP_END - KHEAP_START) - sizeof(HeapNode);
    root_node->next_node = NULL;
    root_node->allocated = false;
}

static HeapNode* createNode(void* place, size_t size, HeapNode* next, bool allocated) {
    HeapNode* node = (HeapNode*) place;
    node->magic_number = KHEAP_MAGIC;
    node->size = size;
    node->next_node = next;
    node->allocated = allocated;
    return node;
}

void* kheapAlloc(size_t size) {
    HeapNode* iter = root_node;
    while (iter != NULL) {
        if (iter->magic_number != KHEAP_MAGIC) {
            kprintf("Kernel heap was corrupted!\n");
            while (true);
        }
        if (!iter->allocated && iter->size >= size) {
            // Allocate!
            size_t space_remaining_after_allocation = iter->size - size;
            if (space_remaining_after_allocation > sizeof(HeapNode)) {
                // Create a new node from this extra space
                uint8_t* byte_ptr = ((uint8_t*) NODE_TO_CONTENTS(iter)) + size;
                HeapNode* next_node = createNode(
                    byte_ptr,
                    space_remaining_after_allocation - sizeof(HeapNode),
                    iter->next_node,
                    false
                );
                // Rewrite current node
                iter->size = size;
                iter->next_node = next_node;
                iter->allocated = true;
            } else {
                // Add this extra bit of dangling space to our current heap node
                iter->allocated = true;
            }
            return NODE_TO_CONTENTS(iter);
        }
        // Otherwise
        iter = iter->next_node;
    }
    // Didn't find any space!
    kprintf("Kernel heap ran out of space!\n");
    while (true);
}

static void compactHeap() {
    HeapNode* iter = root_node;
    while (iter != NULL) {
        if (iter->next_node == NULL) {
            break;
        }
        if (!iter->allocated && !iter->next_node->allocated) {
            // Combine free space
            iter->size += sizeof(HeapNode) + iter->next_node->size;
            iter->next_node = iter->next_node->next_node;
            continue;
        }
        iter = iter->next_node;
    }
}

// Performs a reallocation, but makes the operation more efficient in
// many cases.
void* kheapRealloc(void* ptr, size_t size) {
    // Case: Resize to zero is equivalent to a free
    if (size == 0) {
        kheapFree(ptr);
        return NULL;
    }
    
    HeapNode* current_node = CONTENTS_TO_NODE(ptr);
    if (current_node->magic_number != KHEAP_MAGIC) {
        kprintf("Passed bad pointer to kheapRealloc!\n");
        while (true);
    }
    if (size == current_node->size) {
        // Case: Don't do anything if you're reallocating to the same size
        return ptr;
    }
    if (size < current_node->size) {
        // Case: Resize the current node if you're reallocating to a smaller size
        size_t size_delta = current_node->size - size;
        if (size_delta >= sizeof(HeapNode) + 1) {
            // If there's enough room, fill the space with a new, unallocated node
            HeapNode* new_node = createNode(
                NODE_TO_CONTENTS(current_node) + size,
                size_delta - sizeof(HeapNode),
                current_node->next_node,
                false
            );

            current_node->next_node = new_node;
            current_node->size = size;

            compactHeap();
        }
        // Otherwise, don't even bother resizing. We can't do anything with the space it would free.
        return NODE_TO_CONTENTS(current_node);
    }
    size_t possible_size = 0;
    HeapNode* iter = current_node->next_node;
    while (iter != NULL && !iter->allocated) {
        possible_size += sizeof(HeapNode) + iter->size;
        iter = iter->next_node;
    }
    if (size <= possible_size) {
        // Case: Expand the current space if there's enough empty unallocated space afterwards
        size_t size_delta = possible_size - size;
        if (size_delta >= sizeof(HeapNode) + 1) {
            // Create a new node out of the extra space
            HeapNode* new_node = createNode(
                NODE_TO_CONTENTS(current_node) + size,
                size_delta - sizeof(HeapNode),
                iter,
                false
            );
            
            current_node->next_node = new_node;
            current_node->size = size;

            compactHeap();
            
            return NODE_TO_CONTENTS(current_node);
        } else {
            // Consume the extra space - it's not big enough to be useful
            current_node->next_node = iter;
            current_node->size = possible_size;
            return NODE_TO_CONTENTS(current_node);
        }
    }
    // Case: No optimization possible, just do the naive thing
    void* new_ptr = kheapAlloc(size);
    kmemcpy(new_ptr, ptr, current_node->size);
    kheapFree(ptr);
    return new_ptr;
}

void kheapFree(void* ptr) {
    HeapNode* node = CONTENTS_TO_NODE(ptr);
    node->allocated = false;
    compactHeap();
}

void kheapDump() {
    HeapNode* iter = root_node;
    while (iter != NULL) {
        kprintf("Node (%x):\n  Magic: %x\n  Next: %x\n  Size: %d\n  Alloc: %s\n", iter, iter->magic_number, iter->next_node, iter->size, iter->allocated ? "yes" : "no");
        iter = iter->next_node;
    }
}

#include <stddef.h>

// Simple static allocator for CherryUSB (only used for debug/HID classes, CDC doesn't need it)
#define POOL_SIZE 1024
static char mem_pool[POOL_SIZE];
static size_t mem_offset;

void *usb_malloc(size_t size) {
    size = (size + 3) & ~3;
    if (mem_offset + size > POOL_SIZE) return 0;
    void *ptr = mem_pool + mem_offset;
    mem_offset += size;
    return ptr;
}

void usb_free(void *ptr) { (void)ptr; }

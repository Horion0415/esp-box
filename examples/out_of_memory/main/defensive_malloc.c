#include "esp_log.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "defensive_malloc.h"

// Define an array to record allocated memory blocks
MemoryBlock memory_blocks[MAX_BLOCKS];
static int num_blocks = 0;

static const char *TAG = "defensive_malloc";

// Function to record allocated memory blocks
static void record_allocated_memory(void *ptr, size_t size) {
    if (num_blocks < MAX_BLOCKS) {
        memory_blocks[num_blocks].ptr = ptr;
        memory_blocks[num_blocks].size = size;
        num_blocks++;
    }
}

// Function to record freed memory blocks
static void record_freed_memory(void *ptr) {
    for (int i = 0; i < num_blocks; ++i) {
        if (memory_blocks[i].ptr == ptr) {
            // Output log
            ESP_LOGI(TAG, "Freed memory block at address %p, size %zu", memory_blocks[i].ptr, memory_blocks[i].size);
            // Mark the freed memory block as invalid
            memory_blocks[i].ptr = NULL;
            memory_blocks[i].size = 0;
            break;
        }
    }
}

// Override malloc function to record allocated memory blocks and add memory markers
void* defensive_malloc(size_t size, uint32_t caps) {
    size_t total_size = size + 2 * MEMORY_MARKER_SIZE;
    void *ptr = heap_caps_malloc(total_size, caps);
#if REMAINING_MEMORY_LOG
    if(caps == MALLOC_CAP_SPIRAM) {
        size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        ESP_LOGD(TAG, "Free internal memory: %d", free_internal);
    } else if (caps == MALLOC_CAP_INTERNAL) {
        size_t free_spiram =  heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        ESP_LOGD(TAG, "Free SPIRAM memory: %d", free_spiram);
    }
#endif

    if (ptr != NULL) {
        // Record allocated memory block
        record_allocated_memory(ptr, size);
        // Add memory markers
        unsigned int *start_marker = (unsigned int *)ptr;
        *start_marker = MEMORY_MARKER_VALUE;
        unsigned int *end_marker = (unsigned int *)((char *)ptr + total_size - MEMORY_MARKER_SIZE);
        *end_marker = MEMORY_MARKER_VALUE;
        // Output log
        ESP_LOGI(TAG, "Allocated memory block at address %p, size %zu", ptr, size);
        // Return pointer to the actual data
        return (void *)((char *)ptr + MEMORY_MARKER_SIZE);
    } else {
        ESP_LOGE(TAG, "Failed to allocate memory of size %zu", size);
        return NULL;
    }
}

// Override free function to record freed memory blocks and check memory markers
void defensive_free(void *ptr) {
    if (ptr != NULL) {
        // Return to the start of the actual memory block
        void *start_ptr = (char *)ptr - MEMORY_MARKER_SIZE;
        // Check the start marker
        unsigned int *start_marker = (unsigned int *)start_ptr;
        if (*start_marker != MEMORY_MARKER_VALUE) {
            ESP_LOGE(TAG, "Memory block at address %p has been corrupted!", start_ptr);
        }
        // Check the end marker
        void *end_ptr = (char *)ptr + memory_blocks[num_blocks - 1].size;
        unsigned int *end_marker = (unsigned int *)end_ptr;
        if (*end_marker != MEMORY_MARKER_VALUE) {
            ESP_LOGE(TAG, "Memory block at address %p has been corrupted!", start_ptr);
        }
        // Record freed memory block
        record_freed_memory(start_ptr);
        // Free memory
        heap_caps_free(start_ptr);

#if REMAINING_MEMORY_LOG
        size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        ESP_LOGD(TAG, "Free internal memory: %d", free_internal);
        size_t free_spiram =  heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        ESP_LOGD(TAG, "Free SPIRAM memory: %d", free_spiram);
#endif
    }
}

/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef DEFENSIVE_MALLOC_H
#define DEFENSIVE_MALLOC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"

#define MEMORY_MARKER_SIZE  4
#define MEMORY_MARKER_VALUE 0xDEADBEEF
#define MAX_BLOCKS          500

#define REMAINING_MEMORY_LOG (0) 

// Define a structure to represent allocated memory blocks
typedef struct {
    void *ptr;
    size_t size;
} MemoryBlock;

// Override malloc function to record allocated memory blocks and add memory markers
void* defensive_malloc(size_t size, uint32_t caps);

// Override free function to record freed memory blocks and check memory markers
void defensive_free(void *ptr);

#endif
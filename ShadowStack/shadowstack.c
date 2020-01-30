#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define STACK_SIZE 1024
static uint64_t *stack;

void shadow_stack_init() {
    stack = (uint64_t*) malloc(STACK_SIZE * sizeof(uint64_t));
    memset(stack, 0, STACK_SIZE * sizeof(uint64_t));
}

void shadow_stack_push(uint64_t** retAddr) {
    *stack = **retAddr;
    stack++;
}

void shadow_stack_pop(uint64_t** retAddr) {
    --stack;
    if (*stack != **retAddr) {
        fprintf(stderr, "Stack smashing attack detected!\n");
        exit(1);
    }
}

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdint.h>

typedef struct {
  volatile uint64_t *tohost;
  volatile uint64_t *fromhost;
} HTIF_Type;


// __attribute__((used)) volatile uint64_t tohost __attribute__ ((section (".htif")));
// __attribute__((used)) volatile uint64_t fromhost __attribute__ ((section (".htif")));

volatile uint64_t tohost __attribute__ ((section (".htif")));
volatile uint64_t fromhost __attribute__ ((section (".htif")));

void prevent_removal(void) {
    *(volatile uint64_t *)&tohost = 0;
    *(volatile uint64_t *)&fromhost = 0;
}


HTIF_Type htif_handler = {
  .tohost = &tohost,
  .fromhost = &fromhost,
};

void exit_spike(int code) {
    tohost = (uint64_t)((code << 1) | 1); // HTIF Exit Command
    while (1);  // Halt execution
}

#define HTIF_DEV_CHAR 1   // Device 1: Blocking Character Device
#define HTIF_CMD_READ  0  // Command 0: Read a Character
#define HTIF_CMD_WRITE 1  // Command 1: Write a Character

// HTIF Memory-Mapped Registers
volatile uint64_t tohost __attribute__((section(".htif")));
volatile uint64_t fromhost __attribute__((section(".htif")));

char htif_getchar(void) {
    // Send a read request
    tohost = ((uint64_t)HTIF_DEV_CHAR << 56) | ((uint64_t)HTIF_CMD_READ << 48);

    // Wait for FESVR to send a response
    while (fromhost == 0);

    // Read received character
    char c = (char)(fromhost & 0xFF);
    
    // Clear fromhost after reading
    fromhost = 0;
    
    return c;
}

void htif_putchar(char c) {
    tohost = ((uint64_t)HTIF_DEV_CHAR << 56) |
             ((uint64_t)HTIF_CMD_WRITE << 48) |
             (uint64_t)c;

    while (tohost != 0);
}

void htif_print(const char *s) {
    while (*s) {
        htif_putchar(*s++);
    }
}

int main() {
    htif_print("HTIF Input Test: Type a character...\n");

    while (1) {
        char c = htif_getchar();  // Wait for input
        htif_putchar(c);          // Echo the character back
    }

    return 0;
}
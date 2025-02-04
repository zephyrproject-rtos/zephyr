#ifndef ZEPHYR_DRIVERS_HTIF_H
#define ZEPHYR_DRIVERS_HTIF_H

#include <stdint.h>

#define HTIF_DEV_CHAR 1   // Device 1: Blocking Character Device
#define HTIF_CMD_READ  0  // Command 0: Read a Character
#define HTIF_CMD_WRITE 1  // Command 1: Write a Character

// Expose tohost/fromhost for all drivers
extern volatile uint64_t tohost;
extern volatile uint64_t fromhost;

#endif // ZEPHYR_DRIVERS_HTIF_H

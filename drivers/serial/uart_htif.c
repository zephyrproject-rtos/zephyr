#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <stdint.h>

#define HTIF_DEV_CHAR 1   // Device 1: Blocking Character Device
#define HTIF_CMD_READ  0  // Command 0: Read a Character
#define HTIF_CMD_WRITE 1  // Command 1: Write a Character

// HTIF Memory-Mapped Registers
volatile uint64_t tohost __attribute__((section(".htif")));
volatile uint64_t fromhost __attribute__((section(".htif")));

// Function to transmit a character
static void uart_htif_poll_out(const struct device *dev, unsigned char out_char) {
    tohost = ((uint64_t)HTIF_DEV_CHAR << 56) |
             ((uint64_t)HTIF_CMD_WRITE << 48) |
             (uint64_t)out_char;

    while (tohost != 0);  // Wait for FESVR to process the write
}

// Function to receive a character (blocking)
static int uart_htif_poll_in(const struct device *dev, unsigned char *p_char) {
    // Send a read request
    tohost = ((uint64_t)HTIF_DEV_CHAR << 56) | ((uint64_t)HTIF_CMD_READ << 48);

    // Wait until FESVR sends a response
    while (fromhost == 0);

    *p_char = (char)(fromhost & 0xFF);  // Extract received character
    fromhost = 0;  // Clear fromhost

    return 0;  // Success
}

// UART driver initialization
static int uart_htif_init(const struct device *dev) {
    return 0;  // No initialization required
}

// Zephyr UART driver API structure
static const struct uart_driver_api uart_htif_driver_api = {
    .poll_in  = uart_htif_poll_in,
    .poll_out = uart_htif_poll_out,
};

// Register the UART device with Zephyr
DEVICE_DT_DEFINE(DT_NODELABEL(htif), uart_htif_init,
                 NULL, NULL, NULL,
                 PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
                 &uart_htif_driver_api);

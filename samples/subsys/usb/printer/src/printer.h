#ifndef USB_PRINTER_H
#define USB_PRINTER_H

#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_printer.h>

/* Common printer commands */
#define PCL_ESC     0x1B    /* ESC character */
#define PCL_FF      0x0C    /* Form Feed */
#define PCL_LF      0x0A    /* Line Feed */
#define PCL_CR      0x0D    /* Carriage Return */

/* Test page dimensions */
#define TEST_PAGE_WIDTH  80
#define TEST_PAGE_HEIGHT 60

/* Print job states */
enum job_state {
    JOB_STATE_PENDING = 0,
    JOB_STATE_PRINTING,
    JOB_STATE_COMPLETED,
    JOB_STATE_CANCELLED,
    JOB_STATE_FAILED
};

/* Print job structure */
struct print_job {
    uint32_t job_id;
    char name[32];
    enum job_state state;
    uint32_t error_code;
    uint32_t pages_printed;
    uint32_t start_time;
    struct print_job *next;
};

#endif /* USB_PRINTER_H */
/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PRINTER_PCL_H_
#define PRINTER_PCL_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>

/*
 * PCL Command Reference
 * ====================
 * 
 * Reset:
 * ESC E - Printer reset, clear margins, select default font
 * 
 * Page Control:
 * ESC&l0O - Select portrait orientation
 * ESC&l1O - Select landscape orientation
 * ESC&l1H - Select paper from Tray 1
 * ESC&l2H - Select manual paper feed
 * FF     - Form feed (0x0C)
 * CR     - Carriage return (0x0D)
 * LF     - Line feed (0x0A)
 * 
 * Font Selection:
 * ESC(s#P - Set pitch (# = chars per inch)
 * ESC(s#H - Set height (points)
 * ESC(s#V - Set style (0=upright, 1=italic)
 * ESC(s#S - Set stroke weight (0=normal, 3=bold)
 */

/* PCL command characters */
#define PCL_ESC             0x1B    /* ESC character */
#define PCL_FF              0x0C    /* Form Feed */
#define PCL_CR              0x0D    /* Carriage Return */
#define PCL_LF              0x0A    /* Line Feed */

/* PCL command sequences */
#define PCL_RESET           "\x1BE"
#define PCL_PORTRAIT        "\x1B&l0O"
#define PCL_LANDSCAPE       "\x1B&l1O"
#define PCL_FONT_COURIER    "\x1B(s0p12h0s0b4099T"
#define PCL_FONT_LINE       "\x1B(s1p12v0s0b4099T"
#define PCL_MEDIA_TRAY1     "\x1B&l1H"
#define PCL_MEDIA_MANUAL    "\x1B&l2H"

/* Print job information */
struct print_job {
    uint32_t job_id;
    uint32_t pages_total;
    uint32_t pages_printed;
    bool landscape;
    bool manual_feed;
    uint32_t start_time;
    char name[32];           /* Job name from PJL */
};

/* PCL parser state */
struct pcl_parser_state {
    bool in_escape;
    uint8_t cmd_buf[32];
    uint8_t cmd_pos;
    bool landscape;
    bool manual_feed;
};

/* Test page configuration */
#if defined(CONFIG_USB_PRINTER_SAMPLE_TEST_PAGE)
#define TEST_PAGE_WIDTH     80
#define TEST_PAGE_HEIGHT    60
#endif

/**
 * @brief Initialize PCL parser state
 *
 * @param parser Pointer to parser state structure
 */
void pcl_parser_init(struct pcl_parser_state *parser);

/**
 * @brief Process PCL commands in received data
 *
 * Processes PCL escape sequences and updates printer state.
 * If CONFIG_USB_PRINTER_SAMPLE_PCL_SUPPORT is disabled,
 * treats all data as raw text.
 *
 * @param parser PCL parser state
 * @param job Current print job info
 * @param data Received data buffer
 * @param len Length of received data
 */
void pcl_process_command(struct pcl_parser_state *parser,
                        struct print_job *job,
                        const uint8_t *data,
                        size_t len);

#if defined(CONFIG_USB_PRINTER_SAMPLE_TEST_PAGE)
/**
 * @brief Generate test page content
 *
 * @param buffer Buffer to store test page data
 * @param len Pointer to store length of generated data
 */
void pcl_generate_test_page(uint8_t *buffer, size_t *len);
#endif

/**
 * @brief Start a new print job
 *
 * @return Pointer to new print job structure, or NULL if allocation failed
 */
struct print_job *pcl_start_print_job(void);

#endif /* PRINTER_PCL_H_ */
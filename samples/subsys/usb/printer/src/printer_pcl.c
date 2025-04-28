/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include "printer_pcl.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(printer_pcl, CONFIG_USB_DEVICE_LOG_LEVEL);

static uint32_t next_job_id;

void pcl_parser_init(struct pcl_parser_state *parser)
{
    memset(parser, 0, sizeof(*parser));
}

#if defined(CONFIG_USB_PRINTER_SAMPLE_PCL_SUPPORT)
static void process_escape_sequence(struct pcl_parser_state *parser,
                                  struct print_job *job)
{
    /* Skip ESC character */
    if (parser->cmd_pos < 2) {
        return;
    }

    switch (parser->cmd_buf[1]) {
    case 'E':  /* Printer reset */
        LOG_INF("PCL: Printer reset");
        parser->landscape = false;
        parser->manual_feed = false;
        break;

    case '&':  /* PCL parameterized command */
        if (parser->cmd_pos < 4) {
            return;
        }
        if (parser->cmd_buf[2] == 'l') {  /* Logical page */
            switch (parser->cmd_buf[3]) {
            case '0':  /* Portrait orientation */
                if (parser->cmd_buf[4] == 'O') {
                    LOG_INF("PCL: Portrait orientation");
                    parser->landscape = false;
                    if (job) {
                        job->landscape = false;
                    }
                }
                break;
            case '1':  /* Landscape orientation */
                if (parser->cmd_buf[4] == 'O') {
                    LOG_INF("PCL: Landscape orientation");
                    parser->landscape = true;
                    if (job) {
                        job->landscape = true;
                    }
                }
                break;
            case '1':  /* Media source - Tray 1 */
                if (parser->cmd_buf[4] == 'H') {
                    LOG_INF("PCL: Paper from Tray 1");
                    parser->manual_feed = false;
                    if (job) {
                        job->manual_feed = false;
                    }
                }
                break;
            case '2':  /* Media source - Manual */
                if (parser->cmd_buf[4] == 'H') {
                    LOG_INF("PCL: Manual paper feed");
                    parser->manual_feed = true;
                    if (job) {
                        job->manual_feed = true;
                    }
                }
                break;
            }
        }
        break;

    case '(':  /* Font selection */
        LOG_INF("PCL: Font selection command");
        break;
    }
}
#endif /* CONFIG_USB_PRINTER_SAMPLE_PCL_SUPPORT */

void pcl_process_command(struct pcl_parser_state *parser,
                        struct print_job *job,
                        const uint8_t *data,
                        size_t len)
{
    for (size_t i = 0; i < len; i++) {
        uint8_t ch = data[i];

#if defined(CONFIG_USB_PRINTER_SAMPLE_PCL_SUPPORT)
        if (parser->in_escape) {
            if (parser->cmd_pos < sizeof(parser->cmd_buf)) {
                parser->cmd_buf[parser->cmd_pos++] = ch;
            }
            /* Look for command terminator */
            if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
                process_escape_sequence(parser, job);
                parser->in_escape = false;
                parser->cmd_pos = 0;
            }
        } else {
            if (ch == PCL_ESC) {
                parser->in_escape = true;
                parser->cmd_pos = 0;
                parser->cmd_buf[parser->cmd_pos++] = ch;
            } else if (ch == PCL_FF) {
                /* Form feed - page complete */
                if (job) {
                    job->pages_printed++;
                    LOG_INF("Page %d printed", job->pages_printed);
                }
            }
        }
#else
        /* Without PCL support, just count pages on form feeds */
        if (ch == PCL_FF && job) {
            job->pages_printed++;
            LOG_INF("Page %d printed", job->pages_printed);
        }
#endif /* CONFIG_USB_PRINTER_SAMPLE_PCL_SUPPORT */
    }
}

#if defined(CONFIG_USB_PRINTER_SAMPLE_TEST_PAGE)
void pcl_generate_test_page(uint8_t *buffer, size_t *len)
{
    char *pos = buffer;
    int remaining = TEST_PAGE_WIDTH * TEST_PAGE_HEIGHT;

#if defined(CONFIG_USB_PRINTER_SAMPLE_PCL_SUPPORT)
    /* Reset printer and set format with PCL */
    pos += snprintf(pos, remaining, "%s%s%s",
                   PCL_RESET, PCL_PORTRAIT, PCL_FONT_COURIER);
#endif

    /* Header */
    pos += snprintf(pos, remaining,
                   "\r\n\r\n"
                   "    USB Printer Test Page\r\n"
                   "    ===================\r\n\r\n");

    /* Printer information */
    pos += snprintf(pos, remaining,
                   "Model: Virtual PCL Printer\r\n"
                   "Firmware: Zephyr RTOS USB Printer Sample\r\n"
                   "Status: Ready\r\n\r\n");

    /* Print capabilities */
    pos += snprintf(pos, remaining,
                   "Supported Features:\r\n"
#if defined(CONFIG_USB_PRINTER_SAMPLE_PCL_SUPPORT)
                   "- PCL command processing\r\n"
                   "- Portrait/Landscape modes\r\n"
                   "- Multiple paper sources\r\n"
#else
                   "- Raw text printing\r\n"
#endif
                   "- Job tracking\r\n"
                   "- Status reporting\r\n\r\n");

    /* Test patterns */
    pos += snprintf(pos, remaining,
                   "Print Test Pattern:\r\n"
                   "==================\r\n\r\n");

    /* Character test */
    for (char c = 33; c <= 126; c++) {
        *pos++ = c;
        if ((c - 32) % TEST_PAGE_WIDTH == 0) {
            *pos++ = '\r';
            *pos++ = '\n';
        }
    }

    /* Form feed at end of page */
    *pos++ = PCL_FF;

    *len = pos - (char *)buffer;
}
#endif /* CONFIG_USB_PRINTER_SAMPLE_TEST_PAGE */

struct print_job *pcl_start_print_job(void)
{
    struct print_job *job = k_malloc(sizeof(struct print_job));
    
    if (job) {
        memset(job, 0, sizeof(*job));
        job->job_id = ++next_job_id;
        job->start_time = k_uptime_get_32();
    }

    return job;
}
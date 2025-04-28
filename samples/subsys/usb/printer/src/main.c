/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_printer.h>
#include <zephyr/sys/ring_buffer.h>
#include <errno.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>
#include "printer_pcl.h"
#include "printer_pjl.h"
#include "printer_queue.h"

LOG_MODULE_REGISTER(main);

/* Ring buffer for received data */
RING_BUF_DECLARE(printer_rx_buf, CONFIG_USB_PRINTER_SAMPLE_RING_BUF_SIZE);
K_MUTEX_DEFINE(printer_rx_mutex);

/* Semaphore to signal data reception */
K_SEM_DEFINE(printer_rx_sem, 0, 1);

/* Printer state tracking */
static struct pcl_parser_state pcl_parser;
static struct print_job *current_job;
static bool printer_paper_present = true;
static enum printer_state {
    PRINTER_READY,
    PRINTER_BUSY,
    PRINTER_ERROR,
    PRINTER_NO_PAPER
} current_state = PRINTER_READY;

/* PJL response buffer */
static char pjl_response[CONFIG_USB_PRINTER_SAMPLE_PJL_BUFFER_SIZE];

#if defined(CONFIG_USB_PRINTER_SAMPLE_TEST_PAGE)
/* Test page buffer */
static uint8_t test_page[TEST_PAGE_WIDTH * TEST_PAGE_HEIGHT];
static size_t test_page_len;
static bool test_page_pending;
#endif

static void update_printer_status(void)
{
    uint8_t status = USB_PRINTER_STATUS_SELECTED;

    switch (current_state) {
    case PRINTER_ERROR:
        status |= USB_PRINTER_STATUS_ERROR;
        break;
    case PRINTER_NO_PAPER:
        status |= USB_PRINTER_STATUS_ERROR | USB_PRINTER_STATUS_PAPER;
        break;
    default:
        break;
    }

    usb_printer_update_status(status);

    /* Send PJL status update if job active */
    if (current_job) {
        int len = pjl_generate_response(pjl_response, sizeof(pjl_response),
                                      PJL_USTATUS " DEVICE", current_job);
        if (len > 0) {
            usb_printer_send_data(pjl_response, len);
        }
    }
}

/* Job management */
static void start_print_job(void)
{
    if (current_job == NULL) {
        current_job = print_queue_get_next();
        if (current_job) {
            LOG_INF("Processing job %d (%s)", 
                    current_job->job_id, current_job->name);
            update_printer_status();
        }
    }
}

static void finish_current_job(bool success)
{
    if (current_job) {
        uint32_t duration = (k_uptime_get_32() - current_job->start_time) / 1000;
        
        if (success) {
            LOG_INF("Completed job %d (%s): %d pages in %d seconds",
                    current_job->job_id, current_job->name,
                    current_job->pages_printed, duration);
            print_queue_update_state(current_job->job_id,
                                   JOB_STATE_COMPLETED, 0);
        } else {
            LOG_WRN("Failed job %d (%s) after %d pages",
                    current_job->job_id, current_job->name,
                    current_job->pages_printed);
            print_queue_update_state(current_job->job_id,
                                   JOB_STATE_FAILED, current_state);
        }

        /* Send final job status */
        int len = pjl_generate_response(pjl_response, sizeof(pjl_response),
                                      PJL_USTATUS " JOB", current_job);
        if (len > 0) {
            usb_printer_send_data(pjl_response, len);
        }

        current_job = NULL;
        
        /* Clean up completed jobs */
        print_queue_cleanup();
        
        /* Start next job if available */
        start_print_job();
    }
}

#if defined(CONFIG_USB_PRINTER_SAMPLE_SIMULATED_ERRORS)
static void toggle_paper_sensor(void)
{
    printer_paper_present = !printer_paper_present;
    
    if (!printer_paper_present) {
        current_state = PRINTER_NO_PAPER;
        LOG_INF("Paper out");
        
        /* Fail current job if active */
        if (current_job) {
            finish_current_job(false);
        }
    } else {
        current_state = PRINTER_READY;
        LOG_INF("Paper loaded");
        
        /* Start next job if available */
        start_print_job();
    }
    
    update_printer_status();
}
#endif

static void printer_status_changed(bool configured)
{
    if (configured) {
        LOG_INF("USB printer device configured");
        pcl_parser_init(&pcl_parser);
        print_queue_init();
        update_printer_status();
    } else {
        LOG_INF("USB printer device disconnected");
        if (current_job) {
            finish_current_job(false);
        }
        k_mutex_lock(&printer_rx_mutex, K_FOREVER);
        ring_buf_reset(&printer_rx_buf);
        k_mutex_unlock(&printer_rx_mutex);
    }
}

static int printer_class_handle_request(struct usb_setup_packet *setup,
                                      int32_t *len, uint8_t **data)
{
#if defined(CONFIG_USB_PRINTER_SAMPLE_TEST_PAGE)
    /* Check if this is a test page request (vendor-specific) */
    if (setup->bRequest == 0xFF && setup->wValue == 0x00) {
        if (!test_page_pending) {
            struct print_job *job = pcl_start_print_job();
            if (job) {
                strncpy(job->name, "Test Page", sizeof(job->name));
                if (print_queue_add_job(job)) {
                    pcl_generate_test_page(test_page, &test_page_len);
                    test_page_pending = true;
                    start_print_job();
                } else {
                    k_free(job);
                    return -EBUSY;
                }
            }
        }
        return 0;
    }
#endif
    return -ENOTSUP;
}

/* Process received data */
static void process_received_data(const uint8_t *data, size_t len)
{
#if defined(CONFIG_USB_PRINTER_SAMPLE_PJL_SUPPORT)
    /* Check for PJL commands first */
    if (pjl_process_command(current_job, data, len)) {
        /* Generate PJL response if needed */
        int resp_len = pjl_generate_response(pjl_response,
                                           sizeof(pjl_response),
                                           (const char *)data,
                                           current_job);
        if (resp_len > 0) {
            usb_printer_send_data(pjl_response, resp_len);
        }
        return;
    }
#endif

#if defined(CONFIG_USB_PRINTER_SAMPLE_PCL_SUPPORT)
    /* Process PCL if not PJL */
    pcl_process_command(&pcl_parser, current_job, data, len);
#else
    /* Raw text mode - just count form feeds */
    for (size_t i = 0; i < len; i++) {
        if (data[i] == PCL_FF && current_job) {
            current_job->pages_printed++;
            LOG_INF("Page %d printed", current_job->pages_printed);
        }
    }
#endif
}

/* Data received callback */
static void printer_data_received(const uint8_t *data, size_t len)
{
    if (!data || !len) {
        return;
    }

    /* Reject data if we're in an error state */
    if (current_state == PRINTER_ERROR || current_state == PRINTER_NO_PAPER) {
        LOG_WRN("Printer not ready, data rejected");
        return;
    }

    /* Start a new print job if needed */
    if (!current_job) {
        struct print_job *job = pcl_start_print_job();
        if (job) {
            snprintf(job->name, sizeof(job->name),
                    "Job-%d", job->job_id);
            if (print_queue_add_job(job)) {
                start_print_job();
            } else {
                k_free(job);
                return;
            }
        }
    }

    k_mutex_lock(&printer_rx_mutex, K_FOREVER);
    
    uint32_t written = ring_buf_put(&printer_rx_buf, data, len);
    if (written < len) {
        LOG_WRN("Buffer full, dropped %d bytes", len - written);
    }
    
    k_mutex_unlock(&printer_rx_mutex);
    k_sem_give(&printer_rx_sem);
}

/* USB Printer configuration */
static const struct usb_printer_config printer_config = {
    .status_cb = printer_status_changed,
    .class_handler = printer_class_handle_request,
    .device_id = CONFIG_USB_PRINTER_SAMPLE_DEVICE_ID,
    .data_received = printer_data_received,
};

#if defined(CONFIG_USB_PRINTER_SAMPLE_TEST_PAGE)
/* Process test page if pending */
static void process_test_page(void)
{
    if (test_page_pending && current_job) {
        int ret = usb_printer_send_data(test_page, test_page_len);
        if (ret > 0) {
            LOG_INF("Test page sent");
            current_job->pages_printed++;
            finish_current_job(true);
        }
        test_page_pending = false;
    }
}
#endif

/* Print data processing task */
static void printer_task(void)
{
    uint8_t buf[64];
    uint32_t bytes_read;

    while (1) {
        /* Wait for data */
        k_sem_take(&printer_rx_sem, K_FOREVER);

#if defined(CONFIG_USB_PRINTER_SAMPLE_TEST_PAGE)
        /* Process test page if pending */
        process_test_page();
#endif

        k_mutex_lock(&printer_rx_mutex, K_FOREVER);
        bytes_read = ring_buf_get(&printer_rx_buf, buf, sizeof(buf));
        k_mutex_unlock(&printer_rx_mutex);

        if (bytes_read > 0) {
            /* Process received print data */
            LOG_HEXDUMP_DBG(buf, bytes_read, "Received print data:");
            
            /* Simulate busy state during processing */
            current_state = PRINTER_BUSY;
            update_printer_status();
            
            /* Process received data */
            process_received_data(buf, bytes_read);
            
            /* Check if job complete */
            if (current_job && current_job->pages_printed > 0 &&
                ring_buf_is_empty(&printer_rx_buf)) {
                finish_current_job(true);
            }
            
            /* Return to ready if no errors occurred */
            if (current_state == PRINTER_BUSY) {
                current_state = PRINTER_READY;
                update_printer_status();
            }
            
#if defined(CONFIG_USB_PRINTER_SAMPLE_SIMULATED_ERRORS)
            /* Simulate paper out every 10 pages */
            if (current_job && (current_job->pages_printed % 10) == 0) {
                toggle_paper_sensor();
            }
#endif
            
            /* Simulate processing time */
            k_sleep(K_MSEC(100));
        }
    }
}

K_THREAD_DEFINE(printer_task_id, 2048, printer_task, NULL, NULL, NULL,
                K_PRIO_PREEMPT(8), 0, 0);

void main(void)
{
    int ret;

    LOG_INF("Starting USB printer sample");

    ret = usb_printer_init(&printer_config);
    if (ret < 0) {
        LOG_ERR("Failed to initialize USB printer: %d", ret);
        return;
    }

    LOG_INF("USB printer initialized");
}
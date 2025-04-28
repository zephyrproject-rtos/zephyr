/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include "printer_queue.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(printer_queue, CONFIG_USB_DEVICE_LOG_LEVEL);

/* Queue of print jobs */
static struct print_job_info job_queue[CONFIG_USB_PRINTER_SAMPLE_JOB_QUEUE_SIZE];
static uint32_t queue_head;  /* Index of first job */
static uint32_t queue_tail;  /* Index for adding new jobs */
static uint32_t queue_count; /* Number of jobs in queue */

/* Mutex to protect queue access */
K_MUTEX_DEFINE(queue_mutex);

void print_queue_init(void)
{
    k_mutex_lock(&queue_mutex, K_FOREVER);
    memset(job_queue, 0, sizeof(job_queue));
    queue_head = 0;
    queue_tail = 0;
    queue_count = 0;
    k_mutex_unlock(&queue_mutex);
}

bool print_queue_add_job(struct print_job *job)
{
    bool added = false;

    k_mutex_lock(&queue_mutex, K_FOREVER);

    if (queue_count < CONFIG_USB_PRINTER_SAMPLE_JOB_QUEUE_SIZE) {
        /* Copy job data and initialize extended info */
        memcpy(&job_queue[queue_tail].job, job, sizeof(struct print_job));
        job_queue[queue_tail].state = JOB_STATE_PENDING;
        job_queue[queue_tail].error_code = 0;
        
        /* Update tail and count */
        queue_tail = (queue_tail + 1) % CONFIG_USB_PRINTER_SAMPLE_JOB_QUEUE_SIZE;
        queue_count++;
        
        LOG_INF("Added job %d to queue, %d jobs total",
                job->job_id, queue_count);
        added = true;
    } else {
        LOG_WRN("Job queue full, cannot add job %d", job->job_id);
    }

    k_mutex_unlock(&queue_mutex);
    return added;
}

struct print_job *print_queue_get_next(void)
{
    struct print_job *next_job = NULL;

    k_mutex_lock(&queue_mutex, K_FOREVER);

    if (queue_count > 0) {
        /* Get first pending job */
        for (uint32_t i = 0; i < CONFIG_USB_PRINTER_SAMPLE_JOB_QUEUE_SIZE; i++) {
            uint32_t idx = (queue_head + i) % CONFIG_USB_PRINTER_SAMPLE_JOB_QUEUE_SIZE;
            if (job_queue[idx].state == JOB_STATE_PENDING) {
                job_queue[idx].state = JOB_STATE_ACTIVE;
                next_job = &job_queue[idx].job;
                LOG_INF("Starting job %d", next_job->job_id);
                break;
            }
        }
    }

    k_mutex_unlock(&queue_mutex);
    return next_job;
}

bool print_queue_update_state(uint32_t job_id,
                            enum print_job_state state,
                            uint32_t error_code)
{
    bool updated = false;

    k_mutex_lock(&queue_mutex, K_FOREVER);

    /* Find job in queue */
    for (uint32_t i = 0; i < CONFIG_USB_PRINTER_SAMPLE_JOB_QUEUE_SIZE; i++) {
        if (job_queue[i].job.job_id == job_id) {
            job_queue[i].state = state;
            job_queue[i].error_code = error_code;
            
            LOG_INF("Job %d state changed to %d", job_id, state);
            updated = true;
            break;
        }
    }

    k_mutex_unlock(&queue_mutex);
    return updated;
}

const struct print_job_info *print_queue_get_info(uint32_t job_id)
{
    const struct print_job_info *info = NULL;

    k_mutex_lock(&queue_mutex, K_FOREVER);

    /* Find job in queue */
    for (uint32_t i = 0; i < CONFIG_USB_PRINTER_SAMPLE_JOB_QUEUE_SIZE; i++) {
        if (job_queue[i].job.job_id == job_id) {
            info = &job_queue[i];
            break;
        }
    }

    k_mutex_unlock(&queue_mutex);
    return info;
}

bool print_queue_cancel_job(uint32_t job_id)
{
    bool cancelled = false;

    k_mutex_lock(&queue_mutex, K_FOREVER);

    /* Find job in queue */
    for (uint32_t i = 0; i < CONFIG_USB_PRINTER_SAMPLE_JOB_QUEUE_SIZE; i++) {
        if (job_queue[i].job.job_id == job_id &&
            (job_queue[i].state == JOB_STATE_PENDING ||
             job_queue[i].state == JOB_STATE_ACTIVE)) {
            
            job_queue[i].state = JOB_STATE_CANCELLED;
            LOG_INF("Job %d cancelled", job_id);
            cancelled = true;
            break;
        }
    }

    k_mutex_unlock(&queue_mutex);
    return cancelled;
}

uint32_t print_queue_get_count(void)
{
    k_mutex_lock(&queue_mutex, K_FOREVER);
    uint32_t count = queue_count;
    k_mutex_unlock(&queue_mutex);
    return count;
}

void print_queue_cleanup(void)
{
    k_mutex_lock(&queue_mutex, K_FOREVER);

    /* Remove completed/failed/cancelled jobs */
    while (queue_count > 0 &&
           (job_queue[queue_head].state == JOB_STATE_COMPLETED ||
            job_queue[queue_head].state == JOB_STATE_FAILED ||
            job_queue[queue_head].state == JOB_STATE_CANCELLED)) {
        
        LOG_INF("Removing job %d from queue",
                job_queue[queue_head].job.job_id);
        
        /* Clear job data */
        memset(&job_queue[queue_head], 0, sizeof(struct print_job_info));
        
        /* Update head and count */
        queue_head = (queue_head + 1) % CONFIG_USB_PRINTER_SAMPLE_JOB_QUEUE_SIZE;
        queue_count--;
    }

    k_mutex_unlock(&queue_mutex);
}
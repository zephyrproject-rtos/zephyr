/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PRINTER_QUEUE_H_
#define PRINTER_QUEUE_H_

#include <stdint.h>
#include <stdbool.h>
#include "printer_pcl.h"

/* Print job states */
enum print_job_state {
    JOB_STATE_PENDING,   /* Job waiting to be processed */
    JOB_STATE_ACTIVE,    /* Job currently printing */
    JOB_STATE_COMPLETED, /* Job finished successfully */
    JOB_STATE_FAILED,    /* Job failed (error occurred) */
    JOB_STATE_CANCELLED  /* Job was cancelled */
};

/* Extended job information */
struct print_job_info {
    struct print_job job;         /* Base job information */
    enum print_job_state state;   /* Current job state */
    uint32_t error_code;         /* Error code if failed */
    void *next;                  /* Next job in queue */
};

/**
 * @brief Initialize the print job queue
 */
void print_queue_init(void);

/**
 * @brief Add a job to the print queue
 *
 * @param job Job to add to queue
 * @return true if job was queued, false if queue is full
 */
bool print_queue_add_job(struct print_job *job);

/**
 * @brief Get the next job from the queue
 *
 * @return Pointer to next job or NULL if queue empty
 */
struct print_job *print_queue_get_next(void);

/**
 * @brief Update job state
 *
 * @param job_id ID of job to update
 * @param state New job state
 * @param error_code Error code if job failed
 * @return true if job was found and updated
 */
bool print_queue_update_state(uint32_t job_id,
                            enum print_job_state state,
                            uint32_t error_code);

/**
 * @brief Get information about a specific job
 *
 * @param job_id ID of job to query
 * @return Pointer to job info or NULL if not found
 */
const struct print_job_info *print_queue_get_info(uint32_t job_id);

/**
 * @brief Cancel a pending or active job
 *
 * @param job_id ID of job to cancel
 * @return true if job was found and cancelled
 */
bool print_queue_cancel_job(uint32_t job_id);

/**
 * @brief Get number of jobs in queue
 *
 * @return Number of jobs currently in queue
 */
uint32_t print_queue_get_count(void);

/**
 * @brief Clean up completed/failed jobs
 *
 * Removes jobs that have been completed or failed from the queue
 */
void print_queue_cleanup(void);

#endif /* PRINTER_QUEUE_H_ */
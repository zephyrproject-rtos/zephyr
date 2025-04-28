/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PRINTER_PJL_H_
#define PRINTER_PJL_H_

#include <stdint.h>
#include <stdbool.h>
#include "printer_pcl.h"

/* PJL command markers */
#define PJL_PREFIX      "\x1B%-12345X"  /* Universal Exit Language */
#define PJL_ENTER      "@PJL"           /* Enter PJL mode */
#define PJL_EOL        "\r\n"           /* PJL line ending */

/* PJL commands */
#define PJL_ECHO       "ECHO"
#define PJL_INFO       "INFO"
#define PJL_USTATUS    "USTATUS"
#define PJL_JOB        "JOB"
#define PJL_EOJ        "EOJ"

/* PJL status variables */
#define PJL_STATUS_DEVICE      "DEVICE"
#define PJL_STATUS_JOB        "JOB"
#define PJL_STATUS_PAGE       "PAGE"
#define PJL_STATUS_TIMED      "TIMED"

/* PJL job information */
struct pjl_job_info {
    char name[32];        /* Job name */
    char user[32];        /* User name */
    uint32_t start_time;  /* Job start time */
    uint32_t pages;       /* Pages in job */
};

/**
 * @brief Process PJL command sequence
 *
 * @param job Current print job info
 * @param data Command data
 * @param len Command length 
 * @return true if command was a PJL command
 */
bool pjl_process_command(struct print_job *job,
                        const uint8_t *data,
                        size_t len);

/**
 * @brief Generate PJL response
 *
 * @param buffer Buffer to store response
 * @param size Size of buffer
 * @param command PJL command that was received
 * @param job Current print job
 * @return Length of response or negative error
 */
int pjl_generate_response(char *buffer,
                         size_t size,
                         const char *command,
                         const struct print_job *job);

#endif /* PRINTER_PJL_H_ */
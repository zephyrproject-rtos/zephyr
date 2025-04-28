/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include "printer_pjl.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(printer_pjl, CONFIG_USB_DEVICE_LOG_LEVEL);

/* Command buffer for parsing */
static char cmd_buffer[128];

/* Parse PJL command string */
static const char *pjl_parse_command(const uint8_t *data, size_t len)
{
    /* Look for PJL prefix */
    if (len < strlen(PJL_PREFIX)) {
        return NULL;
    }

    if (memcmp(data, PJL_PREFIX, strlen(PJL_PREFIX)) != 0) {
        return NULL;
    }

    /* Look for @PJL marker */
    const char *cmd = strstr((const char *)data, PJL_ENTER);
    if (!cmd) {
        return NULL;
    }

    /* Skip @PJL and any whitespace */
    cmd += strlen(PJL_ENTER);
    while (*cmd == ' ') {
        cmd++;
    }

    /* Copy command to buffer for parsing */
    strncpy(cmd_buffer, cmd, sizeof(cmd_buffer) - 1);
    cmd_buffer[sizeof(cmd_buffer) - 1] = '\0';

    /* Strip EOL */
    char *eol = strstr(cmd_buffer, PJL_EOL);
    if (eol) {
        *eol = '\0';
    }

    return cmd_buffer;
}

bool pjl_process_command(struct print_job *job,
                        const uint8_t *data,
                        size_t len)
{
    const char *cmd = pjl_parse_command(data, len);
    if (!cmd) {
        return false;
    }

    LOG_DBG("PJL command: %s", cmd);

    if (strncmp(cmd, PJL_JOB, strlen(PJL_JOB)) == 0) {
        /* Job start command */
        if (job) {
            /* Extract job name if present */
            const char *name = strstr(cmd, "NAME=");
            if (name) {
                name += 5; /* Skip "NAME=" */
                if (*name == '"') {
                    name++; /* Skip quote */
                    strncpy(job->name, name, sizeof(job->name) - 1);
                    /* Strip end quote if present */
                    char *quote = strchr(job->name, '"');
                    if (quote) {
                        *quote = '\0';
                    }
                }
            }
            LOG_INF("PJL job start: %s", job->name);
        }
        return true;
    }

    if (strncmp(cmd, PJL_EOJ, strlen(PJL_EOJ)) == 0) {
        /* Job end command */
        if (job) {
            LOG_INF("PJL job end: %s, %d pages", 
                   job->name, job->pages_printed);
        }
        return true;
    }

    if (strncmp(cmd, PJL_ECHO, strlen(PJL_ECHO)) == 0) {
        /* Echo command - will be handled by response generator */
        return true;
    }

    if (strncmp(cmd, PJL_INFO, strlen(PJL_INFO)) == 0) {
        /* Information request - will be handled by response generator */
        return true;
    }

    if (strncmp(cmd, PJL_USTATUS, strlen(PJL_USTATUS)) == 0) {
        /* Status update request - will be handled by response generator */
        return true;
    }

    return false;
}

int pjl_generate_response(char *buffer,
                         size_t size,
                         const char *command,
                         const struct print_job *job)
{
    int len = 0;

    /* Add PJL prefix */
    len += snprintf(buffer + len, size - len, "%s%s", PJL_PREFIX, PJL_EOL);

    if (strncmp(command, PJL_ECHO, strlen(PJL_ECHO)) == 0) {
        /* Echo the command back */
        len += snprintf(buffer + len, size - len, "%s%s", command, PJL_EOL);
    }
    else if (strncmp(command, PJL_INFO, strlen(PJL_INFO)) == 0) {
        /* Return printer information */
        len += snprintf(buffer + len, size - len,
                       "@PJL INFO ID%s"
                       "\"Zephyr USB Printer Sample v1.0\"%s",
                       PJL_EOL, PJL_EOL);
    }
    else if (strncmp(command, PJL_USTATUS, strlen(PJL_USTATUS)) == 0) {
        /* Return printer status */
        if (job) {
            len += snprintf(buffer + len, size - len,
                          "@PJL USTATUS DEVICE%s"
                          "CODE=10001%s"  /* Printer ready */
                          "DISPLAY=\"Ready\"%s"
                          "ONLINE=TRUE%s",
                          PJL_EOL, PJL_EOL, PJL_EOL, PJL_EOL);
        } else {
            len += snprintf(buffer + len, size - len,
                          "@PJL USTATUS DEVICE%s"
                          "CODE=10000%s"  /* Printer idle */
                          "DISPLAY=\"Idle\"%s"
                          "ONLINE=TRUE%s",
                          PJL_EOL, PJL_EOL, PJL_EOL, PJL_EOL);
        }
    }

    /* Add universal exit language */
    len += snprintf(buffer + len, size - len, "%s", PJL_PREFIX);

    return len;
}
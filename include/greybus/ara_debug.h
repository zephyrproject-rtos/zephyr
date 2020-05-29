/*
 * Copyright (c) 2014, 2015 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/****************************************************************************
 * configs/endo/svc/src/up_debug.h
 * SVC support for debug print messages:
 * - allows to control the dump of the data, per component and with
 *   a debug level,
 * - dumps data out to an externally exposed interface (e.g. UART).
 *
 ****************************************************************************/
#ifndef __INCLUDE_ARA_DEBUG_H
#define __INCLUDE_ARA_DEBUG_H

#include <stdio.h>

#if !defined(DBG_COMP)
#define DBG_COMP ARADBG_DBG
#endif

#define DBG_RESCUE_SIZE 64

/* Debug helper macros */
#define dbg_insane(fmt, ...)    dbg_pr(ARADBG_INSANE, fmt, ##__VA_ARGS__)
#define dbg_verbose(fmt, ...)   dbg_pr(ARADBG_VERBOSE, fmt, ##__VA_ARGS__)
#define dbg_info(fmt, ...)      dbg_pr(ARADBG_INFO, fmt, ##__VA_ARGS__)
#define dbg_warn(fmt, ...)      dbg_pr(ARADBG_WARN, fmt, ##__VA_ARGS__)
#define dbg_error(fmt, ...)     dbg_pr(ARADBG_ERROR, fmt, ##__VA_ARGS__)
#define dbg_ui(fmt, ...)        dbg_pr(ARADBG_UI, fmt, ##__VA_ARGS__)

/* Macro value stringify magic */
#define xstr(s) str(s)
#define str(s)  #s

/*
 * Debug dump function, to be used by components code.
 * Every caller has to define its own DBG_COMP macro.
 */
#define dbg_pr(level, fmt, ...)                                     \
    do {                                                            \
        if ((dbg_ctrl.comp & DBG_COMP) &&                           \
            (level >= dbg_ctrl.lvl)) {                              \
            lowsyslog("[" xstr(DBG_COMP) "]: " fmt, ##__VA_ARGS__); \
        }                                                           \
   } while (0)

/*
 * Debug levels, from the low level HW access up to application messages.
 * The dbg_pr function dumps messages of the configured debug level and
 * also the messages of higher levels.
 */
enum {
    ARADBG_INSANE = 0,     /* Insanely verbose */
    ARADBG_VERBOSE,        /* Low level: registers access etc. */
    ARADBG_INFO,           /* Informational */
    ARADBG_WARN,           /* Warning level */
    ARADBG_ERROR,          /* Critical level */
    ARADBG_UI,             /* High level (user interaction with the shell etc.) */
    ARADBG_MAX = ARADBG_UI    /* Always enabled */
};

/*
 * Components to enable debug for, encoded as a bitmask.
 * Up to 32 components using a uint32_t in the comp field.
 */
enum {
    ARADBG_COMM            = (1 << 0),
    ARADBG_DBG             = (1 << 1),
    ARADBG_EPM             = (1 << 2),
    ARADBG_GPIO            = (1 << 3),
    ARADBG_GREYBUS         = (1 << 4),
    ARADBG_HOTPLUG         = (1 << 5),
    ARADBG_I2C             = (1 << 6),
    ARADBG_IOEXP           = (1 << 7),
    ARADBG_NETWORK         = (1 << 8),
    ARADBG_POWER           = (1 << 9),
    ARADBG_SVC             = (1 << 10),
    ARADBG_SWITCH          = (1 << 11),
    ARADBG_AUDIO           = (1 << 12),
    ARADBG_ALL             = 0xFFFFFFFF,
};

/* Debug control internal struct */
typedef struct {
    uint32_t comp;      /* Bitmask for components to enable */
    uint32_t lvl;       /* debug level */
} dbg_ctrl_t;

extern dbg_ctrl_t dbg_ctrl;

/* Pretty print of an uint8_t buffer */
static inline void dbg_print_buf(uint32_t level, uint8_t *buf, uint32_t size)
{
    uint32_t i, idx;
    int ret;
    uint8_t *p;
    char msg[64];

    /* If level not active, do nothing */
    if (level < dbg_ctrl.lvl)
        return;

    /* Print 16-byte lines */
    for (i = 0; i < (size & 0xfffffff0); i += 16) {
        p = buf + i;
        dbg_pr(level, "%04x: %02x %02x %02x %02x %02x %02x %02x %02x | "
                      "%02x %02x %02x %02x %02x %02x %02x %02x\n",
               i, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
               p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
    }

    if (i >= size)
        return;

    /* Print the remaining of the buffer (< 16 bytes) */
    idx = 0;
    /*  fill the output buffer with the index */
    ret = sprintf(msg, "%04x: ", i);
    if (ret <= 0)
        return;
    else
        idx += ret;
    /*  fill the output buffer with the remaining bytes */
    for (; i < size; i++) {
        /*  fill the output buffer with a separator before the 8th uint8_t */
        if ((i & 0xf) == 8) {
            ret = sprintf(msg + idx, "| ");
            if (ret <= 0)
                return;
            else
                idx += ret;
        }
        /*  fill the output buffer with the next uint8_t */
        p = buf + i;
        ret = sprintf(msg + idx, "%02x ", *p);
        if (ret <= 0)
            return;
        else
            idx += ret;
    }

    /*  fill the output buffer with the final return char */
    ret = sprintf(msg + idx, "\n");
    if (ret <= 0)
        return;
    else
        idx += ret;

    /* Output the string */
    dbg_pr(level, "%s", msg);

    return;
}

extern void dbg_get_config(uint32_t *comp, uint32_t *level);
extern int dbg_set_config(uint32_t comp, uint32_t level);

#endif /* __INCLUDE_ARA_DEBUG_H */

/*
 * Copyright 2023 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INSTRUMENTATION_RINGBUFFER_H_
#define ZEPHYR_INCLUDE_INSTRUMENTATION_RINGBUFFER_H_

#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>

#include <zephyr/instrumentation/instrumentation.h>

#define DEBUG 0

void instr_rb_init(void);

struct instr_record *instr_rb_put_item_claim(enum instr_event_types type);
void instr_rb_put_item_finish(enum instr_event_types type);
int instr_rb_put_item_wrapping(struct instr_record *record);

struct instr_record *instr_rb_get_item_claim(void);
void instr_rb_get_item_finish(struct instr_record *record);
struct instr_record *instr_rb_get_item(struct instr_record *record);
int instr_rb_get_item_wrapping(struct instr_record *record);

/* TODO(gromero): get_item_size() probably not for external use. */
size_t instr_rb_get_item_size(enum instr_event_types type);

#endif

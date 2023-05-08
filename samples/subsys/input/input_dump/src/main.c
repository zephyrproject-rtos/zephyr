/*
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/input/input.h>

static void input_cb(struct input_event *evt)
{
	printf("input event: dev=%-16s %3s type=%2x code=%3d value=%d\n",
	       evt->dev ? evt->dev->name : "NULL",
	       evt->sync ? "SYN" : "",
	       evt->type,
	       evt->code,
	       evt->value);
}
INPUT_LISTENER_CB_DEFINE(NULL, input_cb);

int main(void)
{
	printf("Input sample started\n");
	return 0;
}

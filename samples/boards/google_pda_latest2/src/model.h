#ifndef _MODEL_H_
#define _MODEL_H_

#include <zephyr/kernel.h>

int model_init(const struct device *dev);

void set_readline(uint8_t line_mask);

void reset_sniffer();

void set_role(uint8_t role_mask);

void start_snooper(bool s);

void select_pull_up(uint8_t pull_mask);

#endif

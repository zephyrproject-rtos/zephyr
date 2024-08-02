/* board.h - Board-specific hooks */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(NODE_ADDR)
#define NODE_ADDR 0x0b0c
#endif

void board_button_1_pressed(void);
uint16_t board_set_target(void);
void board_play(const char *str);

#if defined(CONFIG_BOARD_BBC_MICROBIT)
int board_init(uint16_t *addr);
void board_play_tune(const char *str);
void board_heartbeat(uint8_t hops, uint16_t feat);
void board_other_dev_pressed(uint16_t addr);
void board_attention(bool attention);
#else
static inline int board_init(uint16_t *addr)
{
	*addr = NODE_ADDR;
	return 0;
}

static inline void board_play_tune(const char *str)
{
}

void board_heartbeat(uint8_t hops, uint16_t feat)
{
}

void board_other_dev_pressed(uint16_t addr)
{
}

void board_attention(bool attention)
{
}
#endif

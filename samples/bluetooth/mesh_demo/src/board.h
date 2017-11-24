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
u16_t board_set_target(void);
void board_play(const char *str);

#if defined(CONFIG_BOARD_BBC_MICROBIT)
void board_init(u16_t *addr, u32_t *seq);
void board_seq_update(uint32_t seq);
void board_play_tune(const char *str);
void board_heartbeat(u8_t hops, u16_t feat);
void board_other_dev_pressed(u16_t addr);
void board_attention(bool attention);
#else
static inline void board_init(u16_t *addr, u32_t *seq)
{
	*addr = NODE_ADDR;
	*seq = 0;
}

static inline void board_seq_update(uint32_t seq)
{
}

static inline void board_play_tune(const char *str)
{
}

void board_heartbeat(u8_t hops, u16_t feat)
{
}

void board_other_dev_pressed(u16_t addr)
{
}

void board_attention(bool attention)
{
}
#endif

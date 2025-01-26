/*
 * Copyright (c) 2024-2025 Freedom Veiculos Eletricos
 * Copyright (c) 2024-2025 O.S. Systems Software LTDA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_morse

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>

#include <zephyr/morse/morse.h>
#include <zephyr/morse/morse_device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(morse, CONFIG_MORSE_LOG_LEVEL);

enum morse_stream_fsm {
	MORSE_CODE_STREAM_FSM_LOAD,
	MORSE_CODE_STREAM_FSM_TX,
};

enum morse_bit_fsm {
	MORSE_CODE_BIT_FSM_LOAD_SYMBOL,
	MORSE_CODE_BIT_FSM_WAIT_BLANK_PHASE,
};

enum morse_rx_bit_fsm {
	MORSE_CODE_RX_BIT_FSM_IDLE,
	MORSE_CODE_RX_BIT_FSM_PULSE_PHASE,
	MORSE_CODE_RX_BIT_FSM_BLANK_PHASE,
	MORSE_CODE_RX_BIT_FSM_DISPATCH,
};

enum morse_prosigns {
	MORSE_CODE_WORD_SPACE = 0x20,
	MORSE_CODE_LETTER_SPACE = 0x40,
	MORSE_CODE_PROSIGN_CT_START_TX,
	MORSE_CODE_PROSIGN_AR_END_TX,
	MORSE_CODE_WORD_LOWERCASE = 0x60,
	MORSE_CODE_END_ALPHABET = 0x80,
};

enum morse_dots {
	MORSE_DOT = 0x01,
	MORSE_3DOTS = 0x03,	/* DASH length and Letter identifier */
	MORSE_DASH  = 0x07,	/* DASH mask */
	MORSE_7DOTS = 0x07,	/* Word space */
	MORSE_9DOTS = 0x09,	/* Force end reception */
	MORSE_INFINITE = 0x10,	/* Infinite */
};

struct morse_tx_callback_data {
	morse_tx_callback_handler_t *callback;
	void *ctx;
	int status;
};

struct morse_rx_callback_data {
	morse_rx_callback_handler_t *callback;
	void *ctx;
};

struct morse_tx_data {
	struct counter_top_cfg dot_tick;
	const uint8_t *data;
	size_t data_idx;
	size_t data_size;
	enum morse_stream_fsm code_fsm;
	enum morse_bit_fsm bit_fsm;
	int code_idx;
	int code_bit;
};

struct morse_rx_data {
	struct counter_top_cfg dot_tick;
	enum morse_bit_state state;
	enum morse_rx_bit_fsm rx_fsm;
	uint32_t data;
	uint32_t ticks;
	uint32_t dot_ticks;
	uint8_t bit_count;
};

struct morse_data {
	struct k_work tx_cb_work;
	struct k_work rx_cb_work;
	struct morse_tx_callback_data tx_cb_info;
	struct morse_rx_callback_data rx_cb_info;
	struct morse_tx_data tx_data;
	struct morse_rx_data rx_data;
};

struct morse_config {
	const struct device *const tx_tmr;
	const struct device *const rx_tmr;
	const struct device *const tx_drv;
	const struct device *const rx_drv;
	uint32_t speed;
};

/*
 * The bit encoding follows the ITU-R M.1677-1 for spacing and length of the
 * signals were:
 *
 * 1: The first MSB byte store the length in bits of the symbol. This is used
 * to optimize the "search first 1 bit" in the symbol bit stream.
 *
 * 2:
 *	The '.' (dot) is encoded as 1 bit with value 1, see below 'E' morse code:
 *
 *	<encoding>, <meaning> <morse code> <bit encoding>
 *	0x01000001,     E           .       1                                  1
 *
 * 3:
 * 3.1:
 *	A dash is equal to three dots. The 3 dots are 3 consecutive bits with
 *	value 1, for instance, the 'T' morse code:
 *
 *	<encoding>, <meaning> <morse code> <bit encoding>
 *	0x03000007,     T           -       111                              111
 *
 * 3.2:
 *	The space between the signals forming the same letter is equal to one
 *	dot. The encoding is made using 1 bit with value 0. In this case the 'I'
 *	is represented as:
 *
 *	<encoding>, <meaning> <morse code> <bit encoding>
 *				   . (dot space is equal to 1 '.' over time)
 *	0x03000005,     I         . .       101                              101
 *
 * 3.3:
 *	The space between two letters is equal to three dots. This is equivalent
 *	to 3 consecutive bits with value 0. This is added automatically by the
 *	engine using the MORSE_CODE_LETTER_SPACE symbol.
 *
 *	<encoding>, <meaning> <morse code> <bit encoding>
 *				  ... (letter space is 3 '.' over time)
 *	0x03000000, letter space '   '      000                              000
 *
 *	In this case to transmit the letters 'TEE' it is necessary to add the
 *	proper spaces between symbols in the letter. The bit stream in the wire
 *	will be '11100010001'. This is used to differentiate from the symbol 'D'
 *	'1110101'.
 *
 *	T<letter space>E<letter space>E
 *
 *	This means that if the space used is equivalent to a '.' in size the
 *	system could send the symbol 'D' (-..) instead transmitting the 'TEE'
 *	word.
 *
 * 3.4:
 *	The space between two words is equal to seven dots. This is equivalent
 *	to 7 consecutive bits with value 0. This is naturally added since it is
 *	mapped in the morse_symbol table as the <SPACE> symbol.
 *
 *	<encoding>, <meaning> <morse code> <bit encoding>
 *				 ....... (letter space is 7 '.' over time)
 *	0x07000000, word space  '       '   0000000                     000 0000
 *
 *	In this case the equivalent bit stream to transmit 'zephyr is the best'
 *	will be (spaces were added to improve read)
 *
 *	Z               e     p               h           y                 r
 *	11101110101 000 1 000 10111011101 000 1010101 000 1110101110111 000 1011101
 *	--.. . .--. .... -.-- .-.
 *
 *	<word space>
 *	0000000
 *
 *	i       s
 *	101 000 10101
 *	.. ...
 *
 *	<word space>
 *	0000000
 *
 *	t       h           e
 *	111 000 1010101 000 1
 *	- .... .
 *
 *	<word space>
 *	0000000
 *
 *	b             e     s         t
 *	111010101 000 1 000 10101 000 111
 *	-... . ... -
 *
 *	Full representation:
 *	--.. . .--. .... -.-- .-. | .. ... | - .... . | -... . ... -
 *	11101110101 000 1 000 10111011101 000 1010101 000 1110101110111 000 1011101 0000000
 *	101 000 10101 0000000 111 000 1010101 000 1 0000000 111010101 000 1 000 10101 000 111
 */

static const uint32_t morse_symbols[] = {
/*      <encoding>,    <meaning> <morse code> <bit encoding>
 *
 * The <encoding> is made using a uint32_t word as follows:
 *	[31:24] - start bit [1]
 *      [23:0]  - morse code symbol's bit stream
 *
 * Reserved
 *	0x0f0075d7,    /CT        -.-.-       111010111010111                  111 0101 1101 0111
 *	0x0d00175d,    /AR        .-.-.       1011101011101                      1 0111 0101 1101
 *	0x0f0055d7,    /VA        ...-.-      101010111010111                  101 0101 1101 0111
 *	0x090001d7,     K         -.-         111010111                               1 1101 0111
 *	0x0f0075dd,     KN        -.--.       111010111011101                  111 0101 1101 1101
 *	0x0b0005d5,    wait       .-...       10111010101                           101 1101 0101
 *	0x0b00055d,    understood ...-.       10101011101                           101 0101 1101
 *	0x0f005555,    error      ........    101010101010101                  101 0101 0101 0101
 */

/*
 *      <encoding>,    <symbol> <morse code> <bit encoding>
 */
	0x07000000, /* word space [2.4]       0000000                                    000 0000 */
	0x13075d77, /*     !     -.-.--       1110101110101110111         111 0101 1101 0111 0111 */
	0x0f005d5d, /*     "     .-..-.       101110101011101                  101 1101 0101 1101 */
	0x0f005555, /*     #     error								  */
	0x11015757, /*     $     ...-..-      10101011101010111             1 0101 0111 0101 0111 */
	0x0f005555, /*     %     error								  */
	0x0b0005d5, /*     &     .-...        10111010101                           101 1101 0101 */
	0x1305dddd, /*     '     .----.       1011101110111011101         101 1101 1101 1101 1101 */
	0x0f0075dd, /*     (     -.--.        111010111011101                  111 0101 1101 1101 */
	0x13075dd7, /*     )     -.--.-       1110101110111010111         111 0101 1101 1101 0111 */
	0x0b000757, /*     *     -..-         11101010111                           111 0101 0111 */
	0x0d00175d, /*     +     .-.-.        1011101011101                      1 0111 0101 1101 */
	0x13077577, /*     ,     --..--       1110111010101110111         111 0111 0101 0111 0111 */
	0x0f007557, /*     -     -....-       111010101010111                  111 0101 0101 0111 */
	0x110175d7, /*     .     .-.-.-       10111010111010111             1 0111 0101 1101 0111 */
	0x0d001d5d, /*     /     -..-.        1110101011101                      1 1101 0101 1101 */
	0x13077777, /*     0     -----        1110111011101110111         111 0111 0111 0111 0111 */
	0x11017777, /*     1     .----        10111011101110111             1 0111 0111 0111 0111 */
	0x0f005777, /*     2     ..---        101011101110111                  101 0111 0111 0111 */
	0x0d001577, /*     3     ...--        1010101110111                      1 0101 0111 0111 */
	0x0b000557, /*     4     ....-        10101010111                           101 0101 0111 */
	0x09000155, /*     5     .....        101010101                               1 0101 0101 */
	0x0b000755, /*     6     -....        11101010101                           111 0101 0101 */
	0x0d001dd5, /*     7     --...        1110111010101                      1 1101 1101 0101 */
	0x0f007775, /*     8     ---..        111011101110101                  111 0111 0111 0101 */
	0x1101dddd, /*     9     ----.        11101110111011101             1 1101 1101 1101 1101 */
	0x1101ddd5, /*     :     ---...       11101110111010101             1 1101 1101 1101 0101 */
	0x1101d75d, /*     ;     -.-.-.       11101011101011101             1 1101 0111 0101 1101 */
	0x0f005555, /*     <     error								  */
	0x0d001d57, /*     =     -...-        1110101010111                      1 1101 0101 0111 */
	0x0f005555, /*     >     error								  */
	0x0f005775, /*     ?     ..--..       101011101110101                  101 0111 0111 0101 */
	0x1101775d, /*     @     .--.-.       10111011101011101             1 0111 0111 0101 1101 */
	0x05000017, /*     A     .-           10111                                        1 0111 */
	0x090001d5, /*     B     -...         111010101                               1 1101 0101 */
	0x0b00075d, /*     C     -.-.         11101011101                           111 0101 1101 */
	0x07000075, /*     D     -..          1110101                                    111 0101 */
	0x01000001, /*     E     .            1                                                 1 */
	0x0900015d, /*     F     ..-.         101011101                               1 0101 1101 */
	0x090001dd, /*     G     --.          111011101                               1 1101 1101 */
	0x07000055, /*     H     ....         1010101                                    101 0101 */
	0x03000005, /*     I     ..           101                                             101 */
	0x0d001777, /*     J     .---         1011101110111                      1 0111 0111 0111 */
	0x090001d7, /*     K     -.-          111010111                               1 1101 0111 */
	0x09000175, /*     L     .-..         101110101                               1 0111 0101 */
	0x07000077, /*     M     --           1110111                                    111 0111 */
	0x0500001d, /*     N     -.           11101                                        1 1101 */
	0x0b000777, /*     O     ---          11101110111                           111 0111 0111 */
	0x0b0005dd, /*     P     .--.         10111011101                           101 1101 1101 */
	0x0d001dd7, /*     Q     --.-         1110111010111                      1 1101 1101 0111 */
	0x0700005d, /*     R     .-.          1011101                                    101 1101 */
	0x05000015, /*     S     ...          10101                                        1 0101 */
	0x03000007, /*     T     -            111                                             111 */
	0x07000057, /*     U     ..-          1010111                                    101 0111 */
	0x09000157, /*     V     ...-         101010111                               1 0101 0111 */
	0x09000177, /*     W     .--          101110111                               1 0111 0111 */
	0x0b000757, /*     X     -..-         11101010111                           111 0101 0111 */
	0x0d001d77, /*     Y     -.--         1110101110111                      1 1101 0111 0111 */
	0x0b000775, /*     Z     --..         11101110101                           111 0111 0101 */
	0x0f005555, /*     [     error								  */
	0x0f005555, /*     \     error								  */
	0x0f005555, /*     ]     error								  */
	0x0f005555, /*     ^     error								  */
	0x11015dd7, /*     _     ..--.-       10101110111010111             1 0101 1101 1101 0111 */
		    /* PROSIGN TABLE								  */
	0x03000000, /* letter space [2.3]     000                                             000 */
	0x0f0075d7, /* /CT        -.-.-       111010111010111                  111 0101 1101 0111 */
	0x0d00175d, /* /AR        .-.-.       1011101011101                      1 0111 0101 1101 */
};

static void morse_tx_cb_handler(struct k_work *item)
{
	struct morse_data *ctx = CONTAINER_OF(item, struct morse_data, tx_cb_work);
	struct morse_tx_callback_data *cb_info;

	if (ctx == NULL) {
		return;
	}

	cb_info = &ctx->tx_cb_info;

	if (cb_info->callback) {
		(cb_info->callback)(cb_info->ctx, cb_info->status);
	}
}

static void morse_rx_cb_handler(struct k_work *item)
{
	struct morse_data *ctx = CONTAINER_OF(item, struct morse_data, rx_cb_work);
	struct morse_rx_callback_data *cb_info = &ctx->rx_cb_info;
	enum morse_rx_state state = MORSE_RX_STATE_END_LETTER;
	int dots = ctx->rx_data.ticks / ctx->rx_data.dot_ticks;

	LOG_DBG("FSM: %d, bit: %d, ticks: %d, c: %d, t: %d",
		ctx->rx_data.rx_fsm,
		ctx->rx_data.state, dots,
		ctx->rx_data.ticks, ctx->rx_data.dot_ticks);

	/*
	 * process input level to compute:
	 * time on/off
	 * space
	 * decode a character
	 * when find a new character invoke the callback
	 */
	if (ctx->rx_data.rx_fsm == MORSE_CODE_RX_BIT_FSM_IDLE) {
		if (ctx->rx_data.state != MORSE_BIT_STATE_ON) {
			return;
		}

		ctx->rx_data.rx_fsm = MORSE_CODE_RX_BIT_FSM_PULSE_PHASE;
		ctx->rx_data.data = 0;
		ctx->rx_data.bit_count = 0;
		return;
	}

	if (ctx->rx_data.rx_fsm == MORSE_CODE_RX_BIT_FSM_PULSE_PHASE) {
		/* get pulse length
		 * compute tolerance
		 */
		uint32_t tmp = ctx->rx_data.data;

		if (dots < MORSE_3DOTS) {
			tmp <<= MORSE_DOT;
			tmp |= MORSE_DOT;
			ctx->rx_data.bit_count += MORSE_DOT;
		} else {
			tmp <<= MORSE_3DOTS;
			tmp |= MORSE_DASH;
			ctx->rx_data.bit_count += MORSE_3DOTS;
		}

		ctx->rx_data.data = tmp;
		ctx->rx_data.rx_fsm = MORSE_CODE_RX_BIT_FSM_BLANK_PHASE;

		LOG_DBG("data: 0x%08x", tmp);
		return;
	}

	if (ctx->rx_data.rx_fsm == MORSE_CODE_RX_BIT_FSM_BLANK_PHASE) {
		/* get pulse length
		 * compute tolerance
		 */
		uint32_t tmp = ctx->rx_data.data;

		if (dots < MORSE_3DOTS) {
			tmp <<= MORSE_DOT;
			ctx->rx_data.data = tmp;
			ctx->rx_data.bit_count += MORSE_DOT;
			ctx->rx_data.rx_fsm = MORSE_CODE_RX_BIT_FSM_PULSE_PHASE;

			LOG_DBG("data: 0x%08x", tmp);
			return;
		} else if (dots < MORSE_7DOTS) {
			/* Finish letter */
			state = MORSE_RX_STATE_END_LETTER;
		} else if (dots < MORSE_9DOTS) {
			/* Finish word */
			state = MORSE_RX_STATE_END_WORD;
		} else {
			/* Finish Transmission */
			state = MORSE_RX_STATE_END_TRANSMISSION;
		}

		tmp |= (ctx->rx_data.bit_count & 0xff) << 24;
		ctx->rx_data.data = tmp;
	}

	LOG_DBG("FSM: %d, data: 0x%08x", ctx->rx_data.rx_fsm, ctx->rx_data.data);

	if (cb_info->callback) {
		uint32_t data = 0x00;

		for (int i = 0; i < ARRAY_SIZE(morse_symbols); ++i) {
			if (morse_symbols[i] == ctx->rx_data.data) {
				data = i + MORSE_CODE_WORD_SPACE;
				break;
			}
		}
		(cb_info->callback)(cb_info->ctx, state, data);
	}

	if (dots < MORSE_9DOTS) {
		ctx->rx_data.data = 0;
		ctx->rx_data.bit_count = 0;
		ctx->rx_data.rx_fsm = MORSE_CODE_RX_BIT_FSM_PULSE_PHASE;
	} else {
		ctx->rx_data.rx_fsm = MORSE_CODE_RX_BIT_FSM_IDLE;
	}
}

/*
 * This executes in the rx driver interrupt context each time a new input edge.
 */
static int morse_device_bit_state_handler(const struct device *dev,
					  enum morse_bit_state state,
					  const struct device *morse)
{
	const struct morse_config *const cfg = morse->config;
	struct morse_data *ctx = morse->data;

	ARG_UNUSED(dev);

	ctx->rx_data.state = state;
	counter_get_value(cfg->rx_tmr, &ctx->rx_data.ticks);
	counter_start(cfg->rx_tmr);
	k_work_submit(&ctx->rx_cb_work);

	return 0;
}

static void morse_word_blank_handler(const struct device *dev, void *user_data)
{
	const struct device *mdev = user_data;
	const struct morse_config *const cfg = mdev->config;
	struct morse_data *ctx = mdev->data;

	counter_stop(cfg->rx_tmr);
	ctx->rx_data.ticks = counter_get_top_value(cfg->rx_tmr);
	ctx->rx_data.rx_fsm = MORSE_CODE_RX_BIT_FSM_BLANK_PHASE;
	ctx->rx_data.state = MORSE_BIT_STATE_OFF;
	k_work_submit(&ctx->rx_cb_work);

	LOG_DBG("RX Blank ticks: %d", ctx->rx_data.ticks);
}

static int morse_get_tx_bit_state(struct morse_data *ctx)
{
	uint32_t bit = morse_symbols[ctx->tx_data.code_idx]
		     & BIT(--ctx->tx_data.code_bit);

	if (ctx->tx_data.code_bit == 0) {
		ctx->tx_data.code_fsm = MORSE_CODE_STREAM_FSM_LOAD;
		if (ctx->tx_data.bit_fsm == MORSE_CODE_BIT_FSM_LOAD_SYMBOL) {
			++ctx->tx_data.data_idx;
		}
	}

	return bit ? 1 : 0;
}

static int morse_load(struct morse_data *ctx)
{
	uint8_t symbol;

	if (ctx->tx_data.bit_fsm == MORSE_CODE_BIT_FSM_LOAD_SYMBOL) {
		symbol = ctx->tx_data.data[ctx->tx_data.data_idx];
		LOG_DBG("Loading %c idx: %d/%d",
			symbol, ctx->tx_data.data_idx, ctx->tx_data.data_size - 1);

		if (ctx->tx_data.data_idx + 1 < ctx->tx_data.data_size &&
		    ctx->tx_data.data[ctx->tx_data.data_idx]     != MORSE_CODE_WORD_SPACE &&
		    ctx->tx_data.data[ctx->tx_data.data_idx + 1] != MORSE_CODE_WORD_SPACE) {
			ctx->tx_data.bit_fsm = MORSE_CODE_BIT_FSM_WAIT_BLANK_PHASE;
		}

		/* Sanity Check: Out of Bounds */
		if (symbol <  MORSE_CODE_WORD_SPACE ||
		    symbol >= MORSE_CODE_END_ALPHABET) {
			LOG_ERR("Character invalid.");

			return -EINVAL;
		}

		symbol -= (symbol > MORSE_CODE_WORD_LOWERCASE)
			? MORSE_CODE_LETTER_SPACE
			: MORSE_CODE_WORD_SPACE;
	} else {
		ctx->tx_data.bit_fsm = MORSE_CODE_BIT_FSM_LOAD_SYMBOL;
		symbol = MORSE_CODE_LETTER_SPACE;
	}

	ctx->tx_data.code_idx = symbol;
	ctx->tx_data.code_bit = (morse_symbols[symbol] >> 24) & 0x1f;

	ctx->tx_data.code_fsm = MORSE_CODE_STREAM_FSM_TX;

	LOG_DBG("TX: 0x%02x, bits: 0x%08x", symbol, morse_symbols[symbol]);

	return 0;
}

static void morse_dot_tick_handler(const struct device *dev, void *user_data)
{
	const struct device *mdev = user_data;
	const struct morse_config *const cfg = mdev->config;
	struct morse_data *ctx = mdev->data;
	enum morse_bit_state bit_state;

	if (ctx->tx_data.data_idx == ctx->tx_data.data_size) {
		LOG_DBG("Finish transmission");
		counter_stop(dev);
		morse_set_tx_bit_state(cfg->tx_drv, MORSE_BIT_STATE_OFF);
		ctx->tx_cb_info.status = 0;
		k_work_submit(&ctx->tx_cb_work);
		return;
	}

	if (ctx->tx_data.code_fsm == MORSE_CODE_STREAM_FSM_LOAD) {
		if (morse_load(ctx)) {
			counter_stop(dev);
			ctx->tx_cb_info.status = -EINVAL;
			k_work_submit(&ctx->tx_cb_work);
			return;
		}
	}

	bit_state = morse_get_tx_bit_state(ctx);
	morse_set_tx_bit_state(cfg->tx_drv, bit_state);
}

int morse_send(const struct device *dev, const uint8_t *data, const size_t size)
{
	if (dev == NULL || (data == NULL && size != 0)) {
		LOG_ERR("Device or Data is invalid");
		return -EINVAL;
	}

	const struct morse_config *const cfg = dev->config;
	struct morse_data *ctx = dev->data;

	if (cfg->tx_drv == NULL) {
		LOG_WRN("No TX device configured");
		return -EINVAL;
	}

	if (size == 0 || ctx->tx_data.data_idx != ctx->tx_data.data_size) {
		return (ctx->tx_data.data_idx != ctx->tx_data.data_size)
		       ? -EBUSY
		       : 0;
	}

	LOG_HEXDUMP_DBG(data, size, "data:");

	ctx->tx_data.data = data;
	ctx->tx_data.data_size = size;
	ctx->tx_data.data_idx = 0;

	ctx->tx_data.code_fsm = MORSE_CODE_STREAM_FSM_LOAD;
	ctx->tx_data.bit_fsm = MORSE_CODE_BIT_FSM_LOAD_SYMBOL;

	if (morse_load(ctx)) {
		LOG_ERR("No morse code entry");
		return -ENOENT;
	}

	counter_start(cfg->tx_tmr);

	return 0;
}

int morse_manage_callbacks(const struct device *dev,
			   morse_tx_callback_handler_t *tx_cb,
			   morse_rx_callback_handler_t *rx_cb,
			   void *user_ctx)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	struct morse_data *drv_ctx = dev->data;

	drv_ctx->tx_cb_info.callback = tx_cb;
	drv_ctx->tx_cb_info.ctx = tx_cb ? user_ctx : NULL;
	drv_ctx->tx_cb_info.status = 0;

	drv_ctx->rx_cb_info.callback = rx_cb;
	drv_ctx->rx_cb_info.ctx = rx_cb ? user_ctx : NULL;
	drv_ctx->rx_data.data = 0;

	return 0;
}

int morse_set_config(const struct device *dev, uint16_t speed)
{
	const struct morse_config *cfg = dev->config;
	struct morse_data *ctx = dev->data;
	uint32_t dot_time;
	uint32_t dot_tol;
	uint32_t ticks_tol;
	uint32_t rx_dot_ticks;
	int ret;

	ret = morse_send(dev, NULL, 0);
	if (ret || ctx->rx_data.rx_fsm != MORSE_CODE_RX_BIT_FSM_IDLE) {
		return ret;
	}

	if (speed == 0) {
		LOG_ERR("Speed should be greater then zero");
		return -EINVAL;
	}

	/* 60s/50wps * speed */
	dot_time = 60000000U / (50U * speed);
	/* 0.5% error accepted */
	dot_tol = dot_time / 200;

	ctx->tx_data.dot_tick.ticks = counter_us_to_ticks(cfg->tx_tmr, dot_time);
	ticks_tol = counter_us_to_ticks(cfg->rx_tmr, dot_tol);
	rx_dot_ticks = counter_us_to_ticks(cfg->rx_tmr, dot_time);
	ctx->rx_data.dot_tick.ticks = (rx_dot_ticks + ticks_tol) * MORSE_9DOTS;
	ctx->rx_data.dot_ticks = rx_dot_ticks - ticks_tol;

	ret = counter_set_top_value(cfg->tx_tmr, &ctx->tx_data.dot_tick);
	if (ret) {
		LOG_ERR("Error to set TX dot value %d", ret);
		return ret;
	}
	ret = counter_set_top_value(cfg->rx_tmr, &ctx->rx_data.dot_tick);
	if (ret) {
		LOG_ERR("Error to set RX word blank value %d", ret);
		return ret;
	}

	LOG_DBG("Device %s ready. Tick: %d", dev->name, dot_time);

	return 0;
}

static int morse_init(const struct device *dev)
{
	const struct morse_config *const cfg = dev->config;
	struct morse_data *ctx = dev->data;

	if (cfg->tx_tmr != NULL) {
		LOG_DBG("TX Timer");
		if (!device_is_ready(cfg->tx_tmr)) {
			LOG_ERR("Error: TX timer device %s is not ready", cfg->tx_tmr->name);
			return -ENODEV;
		}
	}

	if (cfg->rx_tmr != NULL) {
		LOG_DBG("RX Timer");
		if (!device_is_ready(cfg->rx_tmr)) {
			LOG_ERR("Error: RX timer device %s is not ready", cfg->rx_tmr->name);
			return -ENODEV;
		}
	}

	if (cfg->tx_drv != NULL) {
		LOG_DBG("TX Driver");
		if (!device_is_ready(cfg->tx_drv)) {
			LOG_ERR("Error: TX driver device %s is not ready", cfg->tx_drv->name);
			return -ENODEV;
		}
	} else {
		LOG_DBG("no TX Driver");
	}

	if (cfg->rx_drv != NULL) {
		LOG_DBG("RX Driver");
		if (!device_is_ready(cfg->rx_drv)) {
			LOG_ERR("Error: RX driver device %s is not ready", cfg->rx_drv->name);
			return -ENODEV;
		}

		if (morse_set_rx_callback(cfg->rx_drv, morse_device_bit_state_handler, dev)) {
			LOG_ERR("Error: RX callback can not be installed");
			return -ENODEV;
		}
	} else {
		LOG_DBG("no RX Driver");
	}

	k_work_init(&ctx->tx_cb_work, morse_tx_cb_handler);
	k_work_init(&ctx->rx_cb_work, morse_rx_cb_handler);

	ctx->tx_data.dot_tick.flags = 0;
	ctx->tx_data.dot_tick.callback = morse_dot_tick_handler;
	ctx->tx_data.dot_tick.user_data = (void *) dev;

	ctx->rx_data.dot_tick.flags = 0;
	ctx->rx_data.dot_tick.callback = morse_word_blank_handler;
	ctx->rx_data.dot_tick.user_data = (void *) dev;

	return morse_set_config(dev, cfg->speed);
}

#define MORSE_INIT(n)								\
	static struct morse_data morse_data_##n;				\
	static const struct morse_config morse_cfg_##n = {			\
		.tx_tmr = DEVICE_DT_GET(DT_INST_PROP(n, tx_tmr)),		\
		.rx_tmr = DEVICE_DT_GET(DT_INST_PROP(n, rx_tmr)),		\
		.tx_drv = DEVICE_DT_GET_OR_NULL(DT_INST_PROP(n, tx_drv)),	\
		.rx_drv = DEVICE_DT_GET_OR_NULL(DT_INST_PROP(n, rx_drv)),	\
		.speed = DT_INST_PROP(n, speed),				\
	};									\
	DEVICE_DT_INST_DEFINE(							\
		n, morse_init, NULL,						\
		&morse_data_##n,						\
		&morse_cfg_##n,							\
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(MORSE_INIT)

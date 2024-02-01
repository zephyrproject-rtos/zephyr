/*
 * Copyright (c) 2024 Freedom Veiculos Eletricos
 * Copyright (c) 2024 OS Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_morse_code

#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/misc/morse_code/morse_code.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(morse_code, CONFIG_MORSE_CODE_LOG_LEVEL);

#define DT_MORSE_PROP(inst, prop) DT_INST_PROP(inst, prop)

enum morse_code_stream_fsm {
	MORSE_CODE_STREAM_FSM_LOAD,
	MORSE_CODE_STREAM_FSM_TX,
};

enum morse_code_bit_fsm {
	MORSE_CODE_BIT_FSM_LOAD_SYMBOL,
	MORSE_CODE_BIT_FSM_WAIT_BLANK_PHASE,
};

enum morse_code_prosigns {
	MORSE_CODE_PROSIGN_LETTER_SPACE = 0x40,
	MORSE_CODE_PROSIGN_CT_START_TX,
	MORSE_CODE_PROSIGN_AR_END_TX,
};

struct morse_code_callback_data {
	morse_callback_handler_t *callback;
	void *ctx;
	int status;
};

struct morse_code_data {
	struct k_work cb_work;
	struct morse_code_callback_data cb_info;
	struct counter_top_cfg dot_tick;
	const uint8_t *data;
	size_t data_idx;
	size_t data_size;
	enum morse_code_stream_fsm code_fsm;
	enum morse_code_bit_fsm bit_fsm;
	int code_idx;
	int code_bit;
};

struct morse_code_config {
	const struct device *const timer;
	const struct gpio_dt_spec gpio;
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
 *	engine using the MORSE_CODE_PROSIGN_LETTER_SPACE symbol.
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
 *	mapped in the morse_code_symbol table as the <SPACE> symbol.
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

static const uint32_t morse_code_symbols[] = {
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
	0x07000000, /* word space [3.4]       0000000                                    000 0000 */
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

	0x03000000, /* letter space [3.3]     000                                             000 */
	0x0f0075d7, /* /CT        -.-.-       111010111010111                  111 0101 1101 0111 */
	0x0d00175d, /* /AR        .-.-.       1011101011101                      1 0111 0101 1101 */
};

static void morse_code_cb_handler(struct k_work *item)
{
	struct morse_code_data *ctx = CONTAINER_OF(item, struct morse_code_data, cb_work);
	struct morse_code_callback_data *cb_info;

	if (ctx == NULL) {
		return;
	}

	cb_info = &ctx->cb_info;

	if (cb_info->callback) {
		(cb_info->callback)(cb_info->ctx, cb_info->status);
	}
}

static int morse_code_peak_bit_state(struct morse_code_data *ctx)
{
	uint32_t bit = morse_code_symbols[ctx->code_idx] & BIT(--ctx->code_bit);

	if (ctx->code_bit == 0) {
		ctx->code_fsm = MORSE_CODE_STREAM_FSM_LOAD;
		if (ctx->bit_fsm == MORSE_CODE_BIT_FSM_LOAD_SYMBOL) {
			++ctx->data_idx;
		}
	}

	return bit;
}

static int morse_code_load(struct morse_code_data *ctx)
{
	uint8_t symbol;

	if (ctx->bit_fsm == MORSE_CODE_BIT_FSM_LOAD_SYMBOL) {
		symbol = ctx->data[ctx->data_idx];
		LOG_DBG("Loading %c idx: %d, size: %d", symbol, ctx->data_idx, ctx->data_size);

		if (ctx->data_idx + 1 < ctx->data_size
		&&  ctx->data[ctx->data_idx]     != 0x20
		&&  ctx->data[ctx->data_idx + 1] != 0x20) {
			ctx->bit_fsm = MORSE_CODE_BIT_FSM_WAIT_BLANK_PHASE;
		}

		/* Sanity Check: Out of Bounds */
		if (symbol < 0x20 || symbol >= 0x80) {
			LOG_ERR("Character invalid.");

			return -EINVAL;
		}

		symbol -= (symbol > 0x60) ? 0x40 : 0x20;
	} else {
		ctx->bit_fsm = MORSE_CODE_BIT_FSM_LOAD_SYMBOL;
		symbol = MORSE_CODE_PROSIGN_LETTER_SPACE;
	}

	ctx->code_idx = symbol;
	ctx->code_bit = (morse_code_symbols[symbol] >> 24) & 0x1f;

	ctx->code_fsm = MORSE_CODE_STREAM_FSM_TX;

	LOG_DBG("TX: 0x%02x, bits: 0x%08x", symbol, morse_code_symbols[symbol]);

	return 0;
}

static void morse_code_dot_tick_handler(const struct device *dev, void *user_data)
{
	const struct device *mdev = user_data;
	const struct morse_code_config *const cfg = mdev->config;
	struct morse_code_data *ctx = mdev->data;
	int bit_state;

	if (ctx->data_idx == ctx->data_size) {
		LOG_DBG("Finish transmission");
		counter_stop(dev);
		gpio_pin_configure_dt(&cfg->gpio, GPIO_OUTPUT_INACTIVE);
		ctx->cb_info.status = 0;
		k_work_submit(&ctx->cb_work);
		return;
	}

	if (ctx->code_fsm == MORSE_CODE_STREAM_FSM_LOAD) {
		if (morse_code_load(ctx)) {
			counter_stop(dev);
			ctx->cb_info.status = -EINVAL;
			k_work_submit(&ctx->cb_work);
			return;
		}
	}

	bit_state = morse_code_peak_bit_state(ctx);
	gpio_pin_configure_dt(&cfg->gpio, bit_state ? GPIO_OUTPUT_ACTIVE
						    : GPIO_OUTPUT_INACTIVE);
	LOG_DBG("%d", bit_state);
}

int morse_code_send(const struct device *dev, const uint8_t *data, const size_t size)
{
	if (dev == NULL || (data == NULL && size != 0)) {
		LOG_ERR("Device or Data is invalid");
		return -EINVAL;
	}

	const struct morse_code_config *const cfg = dev->config;
	struct morse_code_data *ctx = dev->data;

	if (size == 0 || ctx->data_idx != ctx->data_size) {
		return (ctx->data_idx != ctx->data_size) ? -EBUSY : 0;
	}

	LOG_HEXDUMP_DBG(data, size, "data:");

	ctx->data = data;
	ctx->data_size = size;
	ctx->data_idx = 0;

	ctx->code_fsm = MORSE_CODE_STREAM_FSM_LOAD;
	ctx->bit_fsm = MORSE_CODE_BIT_FSM_LOAD_SYMBOL;
	if (morse_code_load(ctx)) {
		LOG_ERR("No morse code entry");
		return -ENOENT;
	}

	counter_start(cfg->timer);

	return 0;
}

int morse_code_manage_callback(const struct device *dev, morse_callback_handler_t *cb, void *ctx)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	struct morse_code_data *drv_ctx = dev->data;

	drv_ctx->cb_info.callback = cb;
	drv_ctx->cb_info.ctx = cb ? ctx : NULL;
	drv_ctx->cb_info.status = 0;

	return 0;
}

int morse_code_set_config(const struct device *dev, uint16_t speed)
{
	const struct morse_code_config *cfg;
	struct morse_code_data *ctx;
	uint32_t dot_time;
	int ret;

	ret = morse_code_send(dev, NULL, 0);
	if (ret) {
		return ret;
	}

	if (speed == 0) {
		LOG_ERR("Speed should be greater then zero");
		return -EINVAL;
	}

	cfg = dev->config;
	ctx = dev->data;

	/* 60s/50wps * speed */
	dot_time = 60000000U / (50U * speed);

	ctx->dot_tick.ticks = counter_us_to_ticks(cfg->timer, dot_time);

	ret = counter_set_top_value(cfg->timer, &ctx->dot_tick);
	if (ret) {
		LOG_ERR("Error at counter_set_top_value %d", ret);
		return ret;
	}

	LOG_DBG("Device %s ready. Tick: %d", dev->name, dot_time);

	return 0;
}

static int morse_code_init(const struct device *dev)
{
	const struct morse_code_config *const cfg = dev->config;
	struct morse_code_data *ctx = dev->data;
	int ret;

	LOG_DBG("Timer");
	if (!device_is_ready(cfg->timer)) {
		LOG_ERR("Error: Timer device %s is not ready", cfg->timer->name);
		return -ENODEV;
	}

	LOG_DBG("GPIO");
	if (!gpio_is_ready_dt(&cfg->gpio)) {
		LOG_ERR("Error: GPIO device %s is not ready", cfg->gpio.port->name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&cfg->gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Error: GPIO device %s do not configure", cfg->gpio.port->name);
		return -EFAULT;
	}

	k_work_init(&ctx->cb_work, morse_code_cb_handler);

	ctx->dot_tick.flags = 0;
	ctx->dot_tick.callback = morse_code_dot_tick_handler;
	ctx->dot_tick.user_data = (void *) dev;

	return morse_code_set_config(dev, cfg->speed);
}

#define MORSE_CODE_DEVICE_DATA(n)						\
	static struct morse_code_data morse_code_data_##n = { 0 }

#define MORSE_CODE_DEVICE_CONFIG(n)						\
	static const struct morse_code_config morse_code_cfg_##n = {		\
		.timer = DEVICE_DT_GET(DT_MORSE_PROP(n, timer_unit)),		\
		.gpio = GPIO_DT_SPEC_GET(DT_DRV_INST(n), gpios),		\
		.speed = DT_MORSE_PROP(n, speed),				\
	}

#define MORSE_CODE_DEVICE_INIT(n)						\
	DEVICE_DT_INST_DEFINE(							\
		n, morse_code_init, NULL,					\
		&morse_code_data_##n,						\
		&morse_code_cfg_##n,						\
		POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY, NULL)

#define MORSE_CODE_INIT(inst)							\
	MORSE_CODE_DEVICE_CONFIG(inst);						\
	MORSE_CODE_DEVICE_DATA(inst);						\
	MORSE_CODE_DEVICE_INIT(inst);

DT_INST_FOREACH_STATUS_OKAY(MORSE_CODE_INIT)

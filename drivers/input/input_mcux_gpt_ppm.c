/*
 * Copyright (c) 2018-2023 NSCDg
 * Copyright (c) 2023 NXP Semiconductors
 * Copyright (c) 2023 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_gpt_ppm_input

#include <fsl_gpt.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mcux_gpt_ppm, CONFIG_INPUT_LOG_LEVEL);

/*
 * PPM decoder tuning parameters
 */
#define PPM_MIN_PULSE_WIDTH   200  /**< minimum width of a valid first pulse */
#define PPM_MAX_PULSE_WIDTH   600  /**< maximum width of a valid first pulse */
#define PPM_MIN_CHANNEL_VALUE 800  /**< shortest valid channel signal */
#define PPM_MAX_CHANNEL_VALUE 2200 /**< longest valid channel signal */
#define PPM_MIN_START         2300 /**< shortest valid start gap (only 2nd part of pulse) */

/* decoded PPM buffer */

#define PPM_MIN_CHANNELS 5
#define PPM_MAX_CHANNELS 20

#define PPM_CHANNEL_VALUE_ZERO 1200 /**< maximum width for a binary zero */
#define PPM_CHANNEL_VALUE_ONE  1800 /**< minimum width for a binary one */

#define PPM_FILTER CONFIG_INPUT_MCUX_GPT_INPUT_REPORT_FILTER

/** Number of same-sized frames required to 'lock' */

#define PPM_CHANNEL_LOCK 4 /**< should be less than the input timeout */

/* Driver config */
struct input_channel_config {
	uint32_t channel;
	uint32_t type;
	uint32_t zephyr_code;
};

struct input_mcux_gpt_ppm_config {
	/* GPT Timer base address */
	GPT_Type *base;
	/* Pinmux configuration */
	const struct pinctrl_dev_config *pcfg;
	int irq;
	int capture_channel;
	uint8_t num_channels;
	const struct input_channel_config *channel_info;
};

/* PPM decoder state machine */
struct ppm {
	uint32_t last_edge;        /* last capture time */
	uint32_t last_mark;        /* last significant edge */
	uint32_t frame_start;      /* the frame width */
	unsigned int next_channel; /* next channel index */
	enum {
		UNSYNCH = 0,
		ARM,
		ACTIVE,
		INACTIVE
	} phase;
};

struct input_mcux_gpt_ppm_data {
	struct k_thread thread;
	struct k_sem report_lock;

	gpt_status_flag_t irq_flag;

	uint16_t ppm_buffer[PPM_MAX_CHANNELS];
	uint16_t ppm_frame_length;
	unsigned int ppm_decoded_channels;

	uint32_t ppm_temp_buffer[PPM_MAX_CHANNELS];

	struct ppm ppm;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_INPUT_MCUX_GPT_PPM_THREAD_STACK_SIZE);
};

/*
 * Handle the PPM decoder state machine.
 */
static int ppm_decode(uint32_t count, struct input_mcux_gpt_ppm_data *data, uint32_t status)
{
	uint32_t width;
	uint32_t interval;
	unsigned int i;
	uint32_t ret = 0;

	/* how long since the last edge? - this handles counter wrapping implicitly. */
	width = count - data->ppm.last_edge;

	/*
	 * if this looks like a start pulse, then push the last set of values
	 * and reset the state machine
	 */
	if (width >= PPM_MIN_START) {

		/*
		 * If the number of channels changes unexpectedly, we don't want
		 * to just immediately jump on the new count as it may be a result
		 * of noise or dropped edges.  Instead, take a few frames to settle.
		 */
		if (data->ppm.next_channel != data->ppm_decoded_channels) {
			static unsigned int new_channel_count;
			static unsigned int new_channel_holdoff;

			if (new_channel_count != data->ppm.next_channel) {
				/* start the lock counter for the new channel count */
				new_channel_count = data->ppm.next_channel;
				new_channel_holdoff = PPM_CHANNEL_LOCK;

			} else if (new_channel_holdoff > 0) {
				/* this frame matched the last one, decrement the lock counter */
				new_channel_holdoff--;

			} else {
				/* seen PPM_CHANNEL_LOCK frames with the new count, accept it */
				data->ppm_decoded_channels = new_channel_count;
				new_channel_count = 0;
			}

		} else {
			/* frame channel count matches expected, let's use it */
			if (data->ppm.next_channel >= PPM_MIN_CHANNELS) {
				for (i = 0; i < data->ppm.next_channel; i++) {
					data->ppm_buffer[i] = data->ppm_temp_buffer[i];
				}
				ret = data->ppm.next_channel;
			}
		}

		/* reset for the next frame */
		data->ppm.next_channel = 0;

		/* next edge is the reference for the first channel */
		data->ppm.phase = ARM;

		data->ppm.last_edge = count;
		return ret;
	}

	switch (data->ppm.phase) {
	case UNSYNCH:
		/* we are waiting for a start pulse - nothing useful to do here */
		break;

	case ARM:

		/* we expect a pulse giving us the first mark */
		if (width < PPM_MIN_PULSE_WIDTH || width > PPM_MAX_PULSE_WIDTH) {
			goto error; /* pulse was too short or too long */
		}

		/* record the mark timing, expect an inactive edge */
		data->ppm.last_mark = data->ppm.last_edge;

		/* frame length is everything including the start gap */
		data->ppm_frame_length = (uint16_t)(data->ppm.last_edge - data->ppm.frame_start);
		data->ppm.frame_start = data->ppm.last_edge;
		data->ppm.phase = ACTIVE;
		break;

	case INACTIVE:

		/* we expect a short pulse */
		if (width < PPM_MIN_PULSE_WIDTH || width > PPM_MAX_PULSE_WIDTH) {
			goto error; /* pulse was too short or too long */
		}

		/* this edge is not interesting, but now we are ready for the next mark */
		data->ppm.phase = ACTIVE;
		break;

	case ACTIVE:
		/* determine the interval from the last mark */
		interval = count - data->ppm.last_mark;
		data->ppm.last_mark = count;

		/* if the mark-mark timing is out of bounds, abandon the frame */
		if ((interval < PPM_MIN_CHANNEL_VALUE) || (interval > PPM_MAX_CHANNEL_VALUE)) {
			goto error;
		}

		/* if we have room to store the value, do so */
		if (data->ppm.next_channel < PPM_MAX_CHANNELS) {
			data->ppm_temp_buffer[data->ppm.next_channel++] = interval;
		}

		data->ppm.phase = INACTIVE;
		break;
	}

	data->ppm.last_edge = count;
	return ret;

	/* the state machine is corrupted; reset it */

error:
	/* we don't like the state of the decoder, reset it and try again */
	data->ppm.phase = UNSYNCH;
	data->ppm_decoded_channels = 0;
	return ret;
}

/* Interrupt fires every time GPT reaches the current capture value */
void mcux_imx_gpt_ppm_isr(const struct device *dev)
{
	const struct input_mcux_gpt_ppm_config *const config = dev->config;
	struct input_mcux_gpt_ppm_data *const data = dev->data;

	uint32_t status, count;

	/* Get current timer count */
	count = GPT_GetInputCaptureValue(config->base, config->capture_channel);
	status = GPT_GetStatusFlags(config->base, data->irq_flag);

	if (status && ppm_decode(count, data, status) > 0) {
		k_sem_give(&data->report_lock);
	}

	/* Clear GPT capture interrupts */
	GPT_ClearStatusFlags(config->base, status);
}

static void input_mcux_gpt_ppm_input_report_thread(const struct device *dev, void *dummy2,
						   void *dummy3)
{
	const struct input_mcux_gpt_ppm_config *const config = dev->config;
	struct input_mcux_gpt_ppm_data *const data = dev->data;

	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	int i, channel;

	uint32_t ppm_last_reported[PPM_MAX_CHANNELS];

	for (i = 0; i < PPM_MAX_CHANNELS; i++) {
		ppm_last_reported[i] = 0;
	}

	while (true) {
		k_sem_take(&data->report_lock, K_FOREVER);
		for (i = 0; i < config->num_channels; i++) {
			channel = config->channel_info[i].channel - 1;
			if (data->ppm_buffer[channel] > (ppm_last_reported[channel] + PPM_FILTER) ||
			    data->ppm_buffer[channel] < (ppm_last_reported[channel] - PPM_FILTER)) {
				switch (config->channel_info[i].type) {
				case INPUT_EV_ABS:
				case INPUT_EV_MSC:
					input_report(dev, config->channel_info[i].type,
						     config->channel_info[i].zephyr_code,
						     data->ppm_buffer[channel], false, K_FOREVER);
					ppm_last_reported[channel] = data->ppm_buffer[channel];
					break;

				case INPUT_EV_KEY:
					if (data->ppm_buffer[channel] > PPM_CHANNEL_VALUE_ONE) {
						input_report_key(
							dev, config->channel_info[i].zephyr_code, 1,
							false, K_FOREVER);
						ppm_last_reported[channel] =
							data->ppm_buffer[channel];
					} else if (data->ppm_buffer[channel] <
						   PPM_CHANNEL_VALUE_ZERO) {
						input_report_key(
							dev, config->channel_info[i].zephyr_code, 0,
							false, K_FOREVER);
						ppm_last_reported[channel] =
							data->ppm_buffer[channel];
					}
					break;
				}
				break;
			}
		}
	}
}

/*
 * @brief Initialize system timer driver
 *
 * Enable the hw timer, setting its tick period, and setup its interrupt
 */
int input_mcux_gpt_ppm_init(const struct device *dev)
{
	const struct input_mcux_gpt_ppm_config *const config = dev->config;
	struct input_mcux_gpt_ppm_data *const data = dev->data;
	int err, i;

	data->ppm_frame_length = 0;
	data->ppm_decoded_channels = 0;

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	for (i = 0; i < config->num_channels; i++) {
		if (config->channel_info[i].channel == 0 ||
		    config->channel_info[i].channel > PPM_MAX_CHANNELS) {
			LOG_ERR("%s: invalid channel number %d (must be greater then 0 and "
				"smaller then %i)",
				dev->name, config->channel_info[i].channel, PPM_MAX_CHANNELS + 1);
			return -EINVAL;
		}
	}

	gpt_config_t gpt_config;

	GPT_GetDefaultConfig(&gpt_config);
	/* Enable GPT timer to run in SOC low power states */
	gpt_config.enableRunInStop = true;
	gpt_config.enableRunInWait = true;
	gpt_config.enableRunInDoze = true;

	gpt_config.enableMode = true;
	gpt_config.clockSource = kGPT_ClockSource_Periph;
	gpt_config.enableFreeRun = true;

	/* Initialize the GPT timer, and enable the relevant interrupts */
	GPT_Init(config->base, &gpt_config);

	GPT_SetInputOperationMode(config->base, config->capture_channel,
				  kGPT_InputOperation_BothEdge);

	/* Divide IPG clock 240Mhz by 24 for 10Mhz base clock */
	GPT_SetClockDivider(config->base, 24);

	/* Enable GPT interrupts for timer match */
	if (config->capture_channel == kGPT_InputCapture_Channel1) {
		GPT_EnableInterrupts(config->base, kGPT_InputCapture1InterruptEnable);
		data->irq_flag = kGPT_InputCapture1Flag;
	} else if (config->capture_channel == kGPT_InputCapture_Channel2) {
		GPT_EnableInterrupts(config->base, kGPT_InputCapture2InterruptEnable);
		data->irq_flag = kGPT_InputCapture2Flag;
	}

	/* Initialize semaphore used by thread to report input */
	k_sem_init(&data->report_lock, 0, 1);

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_INPUT_MCUX_GPT_PPM_THREAD_STACK_SIZE,
			(k_thread_entry_t)input_mcux_gpt_ppm_input_report_thread, (void *)dev, NULL,
			NULL, K_PRIO_COOP(4), 0, K_NO_WAIT);

	k_thread_name_set(&data->thread, "gpt-ppm");

	/* Start timer */
	GPT_StartTimer(config->base);

	return 0;
}

#define INPUT_INFO(input_channel_id)                                                               \
	{                                                                                          \
		.channel = DT_PROP(input_channel_id, channel),                                     \
		.type = DT_PROP(input_channel_id, type),                                           \
		.zephyr_code = DT_PROP(input_channel_id, zephyr_code),                             \
	},

#define INPUT_MCUX_GPT_PPM_INIT(n)                                                                 \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	static const struct input_channel_config ppm_input_##id[] = {                              \
		DT_INST_FOREACH_CHILD(n, INPUT_INFO)};                                             \
                                                                                                   \
	static struct input_mcux_gpt_ppm_data mcux_gpt_ppm_data_##n;                               \
                                                                                                   \
	static const struct input_mcux_gpt_ppm_config mcux_gpt_ppm_cfg_##n = {                     \
		.base = (void *)DT_INST_REG_ADDR(n),                                               \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.capture_channel = DT_INST_PROP(n, capture_channel) - 1,                           \
		.num_channels = ARRAY_SIZE(ppm_input_##id),                                        \
		.channel_info = ppm_input_##id,                                                    \
	};                                                                                         \
                                                                                                   \
	static int mcux_gpt_##n##_init(const struct device *dev);                                  \
	DEVICE_DT_INST_DEFINE(n, mcux_gpt_##n##_init, NULL, &mcux_gpt_ppm_data_##n,                \
			      &mcux_gpt_ppm_cfg_##n, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,      \
			      NULL);                                                               \
                                                                                                   \
	static int mcux_gpt_##n##_init(const struct device *dev)                                   \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mcux_imx_gpt_ppm_isr,       \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
		return input_mcux_gpt_ppm_init(dev);                                               \
	}

DT_INST_FOREACH_STATUS_OKAY(INPUT_MCUX_GPT_PPM_INIT)

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#define STACKSIZE 1024
#define PRIORITY 7

#define ADC_BUFFER_SIZE	4

struct sample_ctx {
	struct k_sem lock;
	struct i2c_target_config cfg;
	uint8_t buffer_idx;
	uint8_t buffer[ADC_BUFFER_SIZE];
};

struct sample_ctx data;

K_THREAD_STACK_DEFINE(adc_thread_stack_area, STACKSIZE);
static struct k_thread adc_thread_data;

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || \
	!DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) \
	ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

struct adc_dt_spec adc_channels[] = {
DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels,
		     DT_SPEC_AND_COMMA)
};

static const struct device *bus = DEVICE_DT_GET(DT_NODELABEL(i2c1));

static void adc_sample_thread(void *arg, void *arg1, void *arg2)
{
	int err;
	uint32_t buf;
	struct adc_sequence sequence = {
		.buffer = &buf,
		.buffer_size = sizeof(buf),
	};

	printk("ADC sampleing started\n");
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!adc_is_ready_dt(&adc_channels[i])) {
			printk("ADC controller device %s not ready\n", adc_channels[i].dev->name);
			return;
		}

		if (!strncmp("ads1310", adc_channels[i].dev->name, 7)) {
			 adc_channels[i].channel_cfg.differential = 1;
		}

		err = adc_channel_setup_dt(&adc_channels[i]);
		if (err < 0) {
			printk("Could not setup channel #%d (%d)\n", i, err);
			return;
		}
	}

	while (1) {
		for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
			int32_t val_mv;

			printk("- %s, channel %d: ",
			       adc_channels[i].dev->name,
			       adc_channels[i].channel_id);

			(void)adc_sequence_init_dt(&adc_channels[i], &sequence);

			err = adc_read_dt(&adc_channels[i], &sequence);
			if (err < 0) {
				printk("Could not read (%d)\n", err);
				continue;
			}

			if (adc_channels[i].channel_cfg.differential) {
				val_mv = (int32_t)((int16_t)buf);
			} else {
				val_mv = (int32_t)buf;
			}

			printk("ADC channel %d %x\n", i, val_mv);

			if (!err) {
				uint8_t *ptr = &data.buffer[i * 2];
				k_sem_take(&data.lock, K_FOREVER);
				*ptr = val_mv >> 8;
				ptr++;
				*ptr = val_mv;
				k_sem_give(&data.lock);
			} else {
				printk(" (value in mV not available)\n");
			}
		}

		k_sleep(K_MSEC(5000));
	}
}

int sample_target_write_requested_cb(struct i2c_target_config *config)
{
	return 0;
}

int sample_target_write_received_cb(struct i2c_target_config *cfg, uint8_t val)
{
	k_sem_take(&data.lock, K_FOREVER);
	data.buffer_idx =  val;
	k_sem_give(&data.lock);

	return 0;
}

int sample_target_read_requested_cb(struct i2c_target_config *cfg, uint8_t *val)
{
	k_sem_take(&data.lock, K_FOREVER);	
	*val = data.buffer[data.buffer_idx];
	data.buffer_idx++;
	k_sem_give(&data.lock);

	return 0;
}

int sample_target_read_processed_cb(struct i2c_target_config *cfg, uint8_t *val)
{
	k_sem_take(&data.lock, K_FOREVER);
	*val = data.buffer[data.buffer_idx];
	data.buffer_idx++;
	k_sem_give(&data.lock);

	return 0;
}

int sample_target_stop_cb(struct i2c_target_config *cfg)
{
	return 0;
}

static void sample_target_buf_write_received(struct i2c_target_config *cfg,
					     uint8_t *ptr, uint32_t len)
{
	return;
}

static int sample_target_buf_read_requested(struct i2c_target_config *cfg,
					    uint8_t **ptr, uint32_t *len)
{
	return 0;
}

static struct i2c_target_callbacks sample_target_callbacks = {
	.write_requested = sample_target_write_requested_cb,
	.write_received = sample_target_write_received_cb,
	.read_requested = sample_target_read_requested_cb,
	.read_processed = sample_target_read_processed_cb,
#ifdef CONFIG_I2C_TARGET_BUFFER_MODE
	.buf_write_received = sample_target_buf_write_received,
	.buf_read_requested = sample_target_buf_read_requested,
#endif
	.stop = sample_target_stop_cb,
};

int main(void)
{
	data.cfg.address = 0x60,
	data.cfg.callbacks = &sample_target_callbacks,

	k_sem_init(&data.lock, 1, 1);

	if (i2c_target_register(bus, &data.cfg) < 0) {
		printk("Failed to register target\n");
		return -1;
	}

	k_thread_create(&adc_thread_data, adc_thread_stack_area,
			K_THREAD_STACK_SIZEOF(adc_thread_stack_area),
			adc_sample_thread, NULL, NULL, NULL,
			PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&adc_thread_data, "adc_thread");

	return 0;
}

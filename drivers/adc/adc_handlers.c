/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel.h>

static inline int z_vrfy_adc_channel_setup(const struct device *dev,
					   const struct adc_channel_cfg *user_channel_cfg)
{
	struct adc_channel_cfg channel_cfg;

	K_OOPS(K_SYSCALL_DRIVER_ADC(dev, channel_setup));
	K_OOPS(k_usermode_from_copy(&channel_cfg,
				(struct adc_channel_cfg *)user_channel_cfg,
				sizeof(struct adc_channel_cfg)));

	return z_impl_adc_channel_setup((const struct device *)dev,
					&channel_cfg);
}
#include <syscalls/adc_channel_setup_mrsh.c>

static bool copy_sequence(struct adc_sequence *dst,
			  struct adc_sequence_options *options,
			  struct adc_sequence *src)
{
	if (k_usermode_from_copy(dst, src, sizeof(struct adc_sequence)) != 0) {
		printk("couldn't copy adc_sequence struct\n");
		return false;
	}

	if (dst->options) {
		if (k_usermode_from_copy(options, dst->options,
				sizeof(struct adc_sequence_options)) != 0) {
			printk("couldn't copy adc_options struct\n");
			return false;
		}
		dst->options = options;
	}

	if (K_SYSCALL_MEMORY_WRITE(dst->buffer, dst->buffer_size) != 0) {
		printk("no access to buffer memory\n");
		return false;
	}
	return true;
}

static inline int z_vrfy_adc_read(const struct device *dev,
				  const struct adc_sequence *user_sequence)

{
	struct adc_sequence sequence;
	struct adc_sequence_options options;

	K_OOPS(K_SYSCALL_DRIVER_ADC(dev, read));
	K_OOPS(K_SYSCALL_VERIFY_MSG(copy_sequence(&sequence, &options,
					(struct adc_sequence *)user_sequence),
				    "invalid ADC sequence"));
	if (sequence.options != NULL) {
		K_OOPS(K_SYSCALL_VERIFY_MSG(sequence.options->callback == NULL,
			    "ADC sequence callbacks forbidden from user mode"));
	}

	return z_impl_adc_read((const struct device *)dev, &sequence);
}
#include <syscalls/adc_read_mrsh.c>

#ifdef CONFIG_ADC_ASYNC
static inline int z_vrfy_adc_read_async(const struct device *dev,
					const struct adc_sequence *user_sequence,
					struct k_poll_signal *async)
{
	struct adc_sequence sequence;
	struct adc_sequence_options options;

	K_OOPS(K_SYSCALL_DRIVER_ADC(dev, read_async));
	K_OOPS(K_SYSCALL_VERIFY_MSG(copy_sequence(&sequence, &options,
					(struct adc_sequence *)user_sequence),
				    "invalid ADC sequence"));
	if (sequence.options != NULL) {
		K_OOPS(K_SYSCALL_VERIFY_MSG(sequence.options->callback == NULL,
			    "ADC sequence callbacks forbidden from user mode"));
	}
	K_OOPS(K_SYSCALL_OBJ(async, K_OBJ_POLL_SIGNAL));

	return z_impl_adc_read_async((const struct device *)dev, &sequence,
				     (struct k_poll_signal *)async);
}
#include <syscalls/adc_read_async_mrsh.c>
#endif /* CONFIG_ADC_ASYNC */

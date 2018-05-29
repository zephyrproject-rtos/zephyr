/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <adc.h>
#include <syscall_handler.h>
#include <string.h>

Z_SYSCALL_HANDLER(adc_enable, dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_ADC(dev, enable));
	_impl_adc_enable((struct device *)dev);
	return 0;
}

Z_SYSCALL_HANDLER(adc_disable, dev)
{
	Z_OOPS(Z_SYSCALL_DRIVER_ADC(dev, disable));
	_impl_adc_disable((struct device *)dev);
	return 0;
}

Z_SYSCALL_HANDLER(adc_read, dev, seq_table_p)
{
	struct adc_seq_entry *entry, *entries_copy;
	struct adc_seq_table *seq_table = (struct adc_seq_table *)seq_table_p;
	struct adc_seq_table seq_table_copy;
	unsigned int entries_bounds;
	int i = 0, ret;

	Z_OOPS(Z_SYSCALL_DRIVER_ADC(dev, read));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(seq_table, sizeof(struct adc_seq_table)));

	seq_table_copy = *seq_table;
	if (Z_SYSCALL_VERIFY_MSG(
			!__builtin_umul_overflow(seq_table_copy.num_entries,
						 sizeof(struct adc_seq_entry),
						 &entries_bounds),
			"num_entries too large")) {
		ret = -EINVAL;
		goto out;
	}

	Z_OOPS(Z_SYSCALL_MEMORY_READ(seq_table_copy.entries, entries_bounds));
	entries_copy = z_thread_malloc(entries_bounds);
	if (!entries_copy) {
		ret = -ENOMEM;
		goto out;
	}

	memcpy(entries_copy, seq_table_copy.entries, entries_bounds);
	seq_table_copy.entries = entries_copy;

	for (entry = seq_table_copy.entries; i < seq_table_copy.num_entries;
	     i++, entry++) {
		if (Z_SYSCALL_MEMORY_WRITE(entry->buffer,
					   entry->buffer_length)) {
			k_free(entries_copy);
			Z_OOPS(1);
		}
	}

	ret = _impl_adc_read((struct device *)dev, &seq_table_copy);
	k_free(entries_copy);
out:
	return ret;
}

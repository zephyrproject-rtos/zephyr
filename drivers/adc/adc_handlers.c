/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <adc.h>
#include <syscall_handler.h>

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
	struct adc_seq_entry *entry;
	struct adc_seq_table *seq_table = (struct adc_seq_table *)seq_table_p;
	int i;

	Z_OOPS(Z_SYSCALL_DRIVER_ADC(dev, read));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(seq_table, sizeof(struct adc_seq_table)));
	Z_OOPS(Z_SYSCALL_MEMORY_ARRAY_READ(seq_table->entries,
					   seq_table->num_entries,
					   sizeof(struct adc_seq_entry)));

	for (entry = seq_table->entries, i = 0; i < seq_table->num_entries;
	     i++, entry++) {
		Z_OOPS(Z_SYSCALL_MEMORY_WRITE(entry->buffer,
					      entry->buffer_length));
	}

	return _impl_adc_read((struct device *)dev, seq_table);
}

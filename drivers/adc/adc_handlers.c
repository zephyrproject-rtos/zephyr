/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <adc.h>
#include <syscall_handler.h>

_SYSCALL_HANDLER1_SIMPLE_VOID(adc_enable, K_OBJ_DRIVER_ADC, struct device *);
_SYSCALL_HANDLER1_SIMPLE_VOID(adc_disable, K_OBJ_DRIVER_ADC, struct device *);

_SYSCALL_HANDLER(adc_read, dev, seq_table_p)
{
	struct adc_seq_entry *entry;
	struct adc_seq_table *seq_table = (struct adc_seq_table *)seq_table_p;
	int i;

	_SYSCALL_OBJ(dev, K_OBJ_DRIVER_ADC);
	_SYSCALL_MEMORY_READ(seq_table, sizeof(struct adc_seq_table));
	_SYSCALL_MEMORY_ARRAY_READ(seq_table->entries, seq_table->num_entries,
				   sizeof(struct adc_seq_entry));

	for (entry = seq_table->entries, i = 0; i < seq_table->num_entries;
	     i++, entry++) {
		_SYSCALL_MEMORY_WRITE(entry->buffer, entry->buffer_length);
	}

	return _impl_adc_read((struct device *)dev, seq_table);
}

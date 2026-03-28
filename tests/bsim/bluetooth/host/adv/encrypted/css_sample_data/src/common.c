/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common.h"

#include "bs_cmd_line.h"

#define SAMPLE_DATA_SET_SIZE 2
static const struct test_sample_data *sample_data_set[] = {
	&sample_data_1,
	&sample_data_2,
};
BUILD_ASSERT(ARRAY_SIZE(sample_data_set) == SAMPLE_DATA_SET_SIZE);

const struct test_sample_data *sample_data;

int data_set;

void test_args_parse(int argc, char *argv[])
{
	bs_args_struct_t args_struct[] = {
		{
			.dest = &data_set,
			.type = 'i',
			.name = "{1, 2}",
			.option = "data-set",
			.descript = "Sample data set ID",
		},
	};

	bs_args_parse_all_cmd_line(argc, argv, args_struct);

	if (data_set < 1 || data_set > SAMPLE_DATA_SET_SIZE) {
		data_set = 1;
	}

	sample_data = sample_data_set[data_set - 1];
}

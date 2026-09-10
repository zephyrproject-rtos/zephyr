/*
 * Copyright 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_DRIVERS_BBRAM_GENERIC_INCLUDE_FIXTURE_H_
#define TESTS_DRIVERS_BBRAM_GENERIC_INCLUDE_FIXTURE_H_

#define BBRAM_TEST_IMPL(name, inst)                                                                \
	ZTEST(generic, test_##name##_##inst)                                                       \
	{                                                                                          \
		const struct device *dev = DEVICE_DT_GET(inst);                                    \
		run_test_##name(dev, get_and_check_emul(dev));                                     \
	}

#define BBRAM_FOR_EACH(fn)                                                                         \
	fn(DT_NODELABEL(mcp7940n)) fn(DT_NODELABEL(ite8xxx2)) fn(DT_NODELABEL(npcx))

const struct emul *get_and_check_emul(const struct device *dev);

#endif /* TESTS_DRIVERS_BBRAM_GENERIC_INCLUDE_FIXTURE_H_ */

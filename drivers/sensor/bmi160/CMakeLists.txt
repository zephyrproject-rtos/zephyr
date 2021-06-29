# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(bmi160.c)
zephyr_library_sources_ifdef(CONFIG_BMI160_TRIGGER bmi160_trigger.c)

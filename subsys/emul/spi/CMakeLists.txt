# SPDX-License-Identifier: Apache-2.0

# Once we have more than 10 devices we should consider splitting them into
# subdirectories to match the drivers/ structure.
zephyr_include_directories_ifdef(CONFIG_EMUL_BMI160 ${ZEPHYR_BASE}/drivers/sensor/bmi160)
zephyr_library_sources_ifdef(CONFIG_EMUL_BMI160		emul_bmi160.c)

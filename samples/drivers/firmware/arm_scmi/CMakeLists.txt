# Copyright 2024 EPAM Systems
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(arm_scmi_shell)

target_sources(app PRIVATE
	src/main.c
	src/shell.c
)

target_sources_ifdef(CONFIG_ARM_SCMI_RESET_HELPERS app PRIVATE
	src/shell_reset.c
)

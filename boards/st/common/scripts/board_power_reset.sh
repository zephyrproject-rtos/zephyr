#!/usr/bin/env bash
#
# Copyright (c) 2025 STMicroelectronics
#
# SPDX-License-Identifier: Apache-2.0

STM32_Programmer_CLI -c port=swd mode=UR --power off index=0 --power on index=0 > /dev/null

sleep 1

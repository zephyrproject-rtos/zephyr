#!/usr/bin/env bash
#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

dir_path=$(dirname "$0")

$dir_path/cap_unicast.sh

$dir_path/cap_broadcast.sh

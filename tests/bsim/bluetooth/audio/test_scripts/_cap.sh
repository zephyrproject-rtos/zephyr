#!/usr/bin/env bash
#
# Copyright (c) 2022-2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

dir_path=$(dirname "$0")

set -e # Exit on error

$dir_path/_cap_broadcast.sh
$dir_path/_cap_unicast.sh

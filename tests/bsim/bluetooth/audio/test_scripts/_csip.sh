#!/usr/bin/env bash
#
# Copyright (c) 2023-2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

dir_path=$(dirname "$0")

$dir_path/csip.sh

$dir_path/csip_encrypted_sirk.sh

$dir_path/csip_forced_release.sh

$dir_path/csip_new_sirk.sh

$dir_path/csip_no_lock.sh

$dir_path/csip_no_rank.sh

$dir_path/csip_no_size.sh

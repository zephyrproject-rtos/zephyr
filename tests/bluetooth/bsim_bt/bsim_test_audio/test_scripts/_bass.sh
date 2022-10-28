#!/usr/bin/env bash
#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

dir_path=$(dirname "$0")

$dir_path/bass_client_sync.sh

$dir_path/bass_server_sync_client_rem.sh

$dir_path/bass_server_sync_server_rem.sh

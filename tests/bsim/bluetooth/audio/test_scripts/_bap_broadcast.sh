#!/usr/bin/env bash
#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

dir_path=$(dirname "$0")

set -e # Exit on error

# Run all bap_broadcast_audio* tests
for file in "$dir_path"/bap_broadcast_audio*.sh; do
    if [ -f "$file" ]; then
        echo "Running $file"
        $file
    fi
done

#!/usr/bin/env bash
#
# Copyright (c) 2022-2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

dir_path=$(dirname "$0")

set -e # Exit on error

# Run all bap_bass* tests
for file in "$dir_path"/bap_bass*.sh; do
    if [ -f "$file" ]; then
        echo "Running $file"
        $file
    fi
done

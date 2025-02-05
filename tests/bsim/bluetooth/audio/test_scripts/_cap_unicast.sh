#!/usr/bin/env bash
#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

dir_path=$(dirname "$0")

set -e # Exit on error

# Run all cap_unicast* tests
for file in "$dir_path"/cap_unicast*.sh; do
    if [ -f "$file" ]; then
        echo "Running $file"
        $file
    fi
done

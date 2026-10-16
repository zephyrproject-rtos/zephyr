#!/usr/bin/env bash
# Copyright 2026 Xiaomi Corporation
# SPDX-License-Identifier: Apache-2.0

simulation_id="hogp_basic" \
    device_id="hogp_device_basic" \
    host_id="hogp_host_basic" \
    $(dirname "${BASH_SOURCE[0]}")/_run_test.sh

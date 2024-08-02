#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

simulation_id="gatt_caching_retry_reads_no_eatt" \
    client_id="gatt_client_retry_reads_no_eatt" \
    server_id="gatt_server_no_eatt" \
    $(dirname "${BASH_SOURCE[0]}")/_run_test.sh

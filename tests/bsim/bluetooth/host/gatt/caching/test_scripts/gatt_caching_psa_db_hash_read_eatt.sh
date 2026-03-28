#!/usr/bin/env bash
# Copyright 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

simulation_id="gatt_caching_psa_db_hash_read_eatt_psa" \
    client_id="gatt_client_db_hash_read_eatt" \
    server_id="gatt_server_eatt" \
    bin_suffix="_psa_overlay_conf" \
    $(dirname "${BASH_SOURCE[0]}")/_run_test.sh

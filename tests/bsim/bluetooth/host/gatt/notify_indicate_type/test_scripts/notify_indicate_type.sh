#!/usr/bin/env bash
# Copyright 2025 Andrew Leech
# SPDX-License-Identifier: Apache-2.0

simulation_id="gatt_notify_indicate_type" \
    server_id="gatt_server" \
    client_id="gatt_client" \
    $(dirname "${BASH_SOURCE[0]}")/_run_test.sh

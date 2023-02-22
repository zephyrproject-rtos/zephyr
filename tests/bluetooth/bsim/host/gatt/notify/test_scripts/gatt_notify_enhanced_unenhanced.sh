
#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

simulation_id="gatt_notify_enhanced_unenhanced" \
    server_id="gatt_server_enhanced" \
    client_id="gatt_client_unenhanced" \
    $(dirname "${BASH_SOURCE[0]}")/_run_test.sh


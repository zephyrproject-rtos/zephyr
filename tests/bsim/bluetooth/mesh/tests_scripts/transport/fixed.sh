#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test scenario:
# Sender:
# - sends 3 messages to all-relays fixed group address
# Receiver:
# - enables the Relay feature and delivers the first message;
# - disables the Relay feature, subscribes the model to the all-relays address and delivers the
#   second message to the model;
# - unsubscribes the model from the all-relayes address and doesn't deliver the third message to the
#   model;
RunTest mesh_transport_fixed transport_tx_fixed transport_rx_fixed

conf=prj_mesh1d1_conf
RunTest mesh_transport_fixed_1d1 transport_tx_fixed transport_rx_fixed

conf=prj_mesh1d1_conf
overlay=overlay_psa_conf
RunTest mesh_transport_fixed_1d1 transport_tx_fixed transport_rx_fixed

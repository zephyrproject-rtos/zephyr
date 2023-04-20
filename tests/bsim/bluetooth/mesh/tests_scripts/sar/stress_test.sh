#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# Test that a maximum length SDU can be processed with SAR,
# even with "stressed" transmitter and receiver configurations.
# Test procedure:
# 1. Initialize Client and Server instances.
# 2. Bind "dummy" vendor model to both instances.
# 3. Configure SAR transmitter and receiver states.
# 4. The Client sends a Get-message with a maximum length SDU, targeting the server.
# 5. The Server responds with a maximum length SDU Status-message.
# 6. The test passes when the Client successfully receives the Status response.
conf=prj_mesh1d1_conf
RunTest sar_test \
	sar_cli_max_len_sdu_send sar_srv_max_len_sdu_receive

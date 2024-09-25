#!/usr/bin/env bash
# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_access_extended_model_subs \
	access_tx_ext_model access_sub_ext_model

overlay=overlay_psa_conf
RunTest mesh_access_extended_model_subs_psa \
	access_tx_ext_model access_sub_ext_model

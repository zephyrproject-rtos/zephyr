#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

RunTest mesh_access_extended_model_subs_cap access_sub_capacity_ext_model

overlay=overlay_psa_conf
RunTest mesh_access_extended_model_subs_cap_psa access_sub_capacity_ext_model

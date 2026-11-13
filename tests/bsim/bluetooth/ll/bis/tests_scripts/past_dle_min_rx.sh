#!/usr/bin/env bash
# Copyright (c) 2024 Demant A/S
# Copyright (c) 2026 Kent Hauser
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

# PAST receive at the minimum data length: same scenario as past_dle_min.sh,
# but the sender is built with connected ISO (overlay-past_sender_cis.conf),
# so it sends LL_PERIODIC_SYNC_IND (35 octets) whole. The receiver is built
# without it and keeps connMaxRxOctets 27, so only its receive maximum decides
# whether it gets the PDU.
simulation_id="${BOARD_TS}_past_dle_min_rx"
verbosity_level=2

exe=bs_${BOARD_TS}_tests_bsim_bluetooth_ll_bis_prj_past_conf_overlay-past_dle_min_conf

cd ${BSIM_OUT_PATH}/bin

Execute ./${exe} \
  -v=${verbosity_level} -s=${simulation_id} -d=0 -testid=receive_past

Execute ./${exe} \
  -v=${verbosity_level} -s=${simulation_id} -d=1 -testid=broadcast_pa

Execute ./${exe}_overlay-past_sender_cis_conf \
  -v=${verbosity_level} -s=${simulation_id} -d=2 -testid=send_past

Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=${simulation_id} \
  -D=3 -sim_length=60e6 $@

wait_for_background_jobs

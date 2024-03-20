#!/usr/bin/env bash
# Copyright 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

dut_exe="bs_${BOARD}_$(guess_test_long_name)_dut_prj_conf"
tester_exe="bs_${BOARD}_$(guess_test_long_name)_tester_prj_conf"

simulation_id="misc_disconnect"
verbosity_level=2
sim_length_us=10e6

cd ${BSIM_OUT_PATH}/bin

Execute ./bs_2G4_phy_v1 \
    -v=${verbosity_level} -s="${simulation_id}" -D=2 -sim_length=${sim_length_us} $@

Execute "./$tester_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=1 -testid=tester -RealEncryption=0 -rs=100

Execute "./$dut_exe" \
    -v=${verbosity_level} -s="${simulation_id}" -d=0 -testid=dut -RealEncryption=0

# wait_for_background_jobs, but doesn't exit on error
exit_code=0
for process_id in $_process_ids; do
wait $process_id || let "exit_code=$?"
done

# Make pcaps
for j in {0..1}; do
    i=$(printf '%02i' $j)

    ${BSIM_OUT_PATH}/components/ext_2G4_phy_v1/dump_post_process/csv2pcap -o \
    ${BSIM_OUT_PATH}/results/${simulation_id}/Trace_$i.Tx.pcap \
    ${BSIM_OUT_PATH}/results/${simulation_id}/d_2G4_$i.Tx.csv

    ${BSIM_OUT_PATH}/components/ext_2G4_phy_v1/dump_post_process/csv2pcap -o \
    ${BSIM_OUT_PATH}/results/${simulation_id}/Trace_$i.Rx.pcap \
    ${BSIM_OUT_PATH}/results/${simulation_id}/d_2G4_$i.Rx.csv

    echo "${BSIM_OUT_PATH}/results/${simulation_id}/Trace_$i.Tx.pcap"
    echo "${BSIM_OUT_PATH}/results/${simulation_id}/Trace_$i.Rx.pcap"
done

exit $exit_code

#!/usr/bin/env bash
# Copyright (c) 2023 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

simulation_id="conn_stress"
process_ids=""; exit_code=0

# We don't use the `Execute` fn from `bsim/sh_common.source` as
# `wait_for_background_jobs` will terminate the script if there's an error, and
# this test will fail often. We still want to run the packet conversion scripts,
# especially if the test was not successful.
function Execute(){
  if [ ! -f $1 ]; then
    echo -e "ERR! \e[91m`pwd`/`basename $1` cannot be found (did you forget to\
 compile it?)\e[39m"
    exit 1
  fi
  timeout 60 $@ & process_ids="$process_ids $!"

  echo "Running $@"
}

test_path="bsim_bluetooth_host_misc_conn_stress"
bsim_central_exe_name="bs_${BOARD_TS}_${test_path}_central_prj_conf"
bsim_peripheral_exe_name="bs_${BOARD_TS}_${test_path}_peripheral_prj_conf"

# terminate running simulations (if any)
${BSIM_COMPONENTS_PATH}/common/stop_bsim.sh $simulation_id

cd ${BSIM_OUT_PATH}/bin

bsim_args="-RealEncryption=1 -v=2 -s=${simulation_id}"
test_args="-argstest notify_size=220 conn_interval=32"

nr_of_units=12

for device in `seq 1 $nr_of_units`; do
    let rs=$device*100

    Execute "./${bsim_peripheral_exe_name}" ${bsim_args} \
        -d=$device -rs=$rs -testid=peripheral ${test_args}
done

Execute ./bs_2G4_phy_v1 -dump -v=2 -s=${simulation_id} -D=13 -sim_length=1000e6 &

Execute "./${bsim_central_exe_name}" ${bsim_args} -d=0 -rs=001 -testid=central ${test_args}

for process_id in $process_ids; do
  wait $process_id || let "exit_code=$?"
done

for i in `seq -w 0 $nr_of_units`; do
    ${BSIM_OUT_PATH}/components/ext_2G4_phy_v1/dump_post_process/csv2pcap -o \
    ${BSIM_OUT_PATH}/results/${simulation_id}/Trace_$i.pcap \
    ${BSIM_OUT_PATH}/results/${simulation_id}/d_2G4_$i.Tx.csv

    ${BSIM_OUT_PATH}/components/ext_2G4_phy_v1/dump_post_process/csv2pcap -o \
    ${BSIM_OUT_PATH}/results/${simulation_id}/Trace_Rx_$i.pcap \
    ${BSIM_OUT_PATH}/results/${simulation_id}/d_2G4_$i.Rx.csv

    echo "${BSIM_OUT_PATH}/results/${simulation_id}/Trace_$i.pcap"
    echo "${BSIM_OUT_PATH}/results/${simulation_id}/Trace_Rx_$i.pcap"
done

exit $exit_code

#!/usr/bin/env bash
# Copyright (c) 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

# EATT test
simulation_id="conn_stress"
process_ids=""; exit_code=0

function Execute(){
  if [ ! -f $1 ]; then
    echo -e "ERR! \e[91m`pwd`/`basename $1` cannot be found (did you forget to\
 compile it?)\e[39m"
    exit 1
  fi
  # timeout 30 $@ & process_ids="$process_ids $!"
  $@ & process_ids="$process_ids $!"

  echo "Running $@"
}

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"

#Give a default value to BOARD if it does not have one yet:
BOARD="${BOARD:-nrf52_bsim}"

testing_apps_loc="${ZEPHYR_BASE}/tests/bluetooth/bsim_bt/bsim_test_conn_stress"
central_app_name="central"
peripheral_app_name="peripheral"
bsim_central_exe_name="bs_nrf52_bsim_bluetooth_central"
bsim_peripheral_exe_name="bs_nrf52_bsim_bluetooth_peripheral"

cd ${testing_apps_loc}

if [ ! -d "${central_app_name}" -o ! -d "${peripheral_app_name}" ]; then
    echo -e "ERR! \e[91mOne or more test applications couldn't be found\e[39m"
    exit 1
fi

#Remove old builds if they exist
find . -type d -name 'build' -exec rm -rf {} +

cd "${testing_apps_loc}/${central_app_name}"
west build -b ${BOARD} .
cp build/zephyr/zephyr.exe ${BSIM_OUT_PATH}/bin/${bsim_central_exe_name}

cd "${testing_apps_loc}/${peripheral_app_name}"
west build -b ${BOARD} .
cp build/zephyr/zephyr.exe ${BSIM_OUT_PATH}/bin/${bsim_peripheral_exe_name}

cd ${BSIM_OUT_PATH}/bin

if [ ! -f "${bsim_central_exe_name}" -o ! -f "${bsim_peripheral_exe_name}" ]; then
    echo -e "ERR! \e[91mOne or more test executables couldn't be found\e[39m"
    exit 1
fi

find ../ -type d -name ${simulation_id} -exec rm -rf {} +

Execute "./${bsim_central_exe_name}" -RealEncryption=1 -v=2 -s=${simulation_id} -d=0 -rs=915
Execute "./${bsim_peripheral_exe_name}" -RealEncryption=1 -v=2 -s=${simulation_id} -d=1 -rs=710
Execute "./${bsim_peripheral_exe_name}" -RealEncryption=1 -v=2 -s=${simulation_id} -d=2 -rs=175
Execute "./${bsim_peripheral_exe_name}" -RealEncryption=1 -v=2 -s=${simulation_id} -d=3 -rs=703
Execute "./${bsim_peripheral_exe_name}" -RealEncryption=1 -v=2 -s=${simulation_id} -d=4 -rs=17
Execute ./bs_2G4_phy_v1 -dump -v=2 -s=${simulation_id} -D=5 -sim_length=800e6 &

for process_id in $process_ids; do
  wait $process_id || let "exit_code=$?"
done

find . -type f -name ${bsim_central_exe_name} -delete
find . -type f -name ${bsim_peripheral_exe_name} -delete

${BSIM_OUT_PATH}/components/ext_2G4_phy_v1/dump_post_process/csv2pcap -o \
${BSIM_OUT_PATH}/results/${simulation_id}/Trace_Tx.pcap \
${BSIM_OUT_PATH}/results/${simulation_id}/d_2G4*.Tx.csv

${BSIM_OUT_PATH}/components/ext_2G4_phy_v1/dump_post_process/csv2pcap -o \
${BSIM_OUT_PATH}/results/${simulation_id}/Trace_Rx.pcap \
${BSIM_OUT_PATH}/results/${simulation_id}/d_2G4*.Rx.csv

echo "${BSIM_OUT_PATH}/results/${simulation_id}/Trace_Tx.pcap"
echo "${BSIM_OUT_PATH}/results/${simulation_id}/Trace_Rx.pcap"

exit $exit_code

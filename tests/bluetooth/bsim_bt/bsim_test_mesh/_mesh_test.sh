# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

process_ids=""; exit_code=0

function Execute(){
  if [ ! -f $1 ]; then
    echo -e "  \e[91m`pwd`/`basename $1` cannot be found (did you forget to\
 compile it?)\e[39m"
    exit 1
  fi
  timeout 20 $@ & process_ids="$process_ids $!"
}

function Skip(){
  for i in "${SKIP[@]}" ; do
    if [ "$i" == "$1" ] ; then
      return 0
    fi
  done

  return 1
}

function RunTest(){
  verbosity_level=${verbosity_level:-2}
  extra_devs=${EXTRA_DEVS:-0}

  cd ${BSIM_OUT_PATH}/bin

  idx=0
  for testid in ${@:2} ; do
    if Skip $testid; then
      echo "Skipping $testid (device #$idx)"
      let idx=idx+1
      continue
    fi

    echo "Starting $testid as device #$idx"
    conf=${conf:-prj_conf}
    Execute \
      ./bs_${BOARD}_tests_bluetooth_bsim_bt_bsim_test_mesh_${conf} \
      -v=${verbosity_level} -s=$1 -d=$idx -RealEncryption=1 \
      -testid=$testid
    let idx=idx+1
  done

  count=$(expr $# - 1 + $extra_devs)

  echo "Starting phy with $count devices"

  Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=$1 -D=$count

  for process_id in $process_ids; do
    wait $process_id || let "exit_code=$?"
  done

  if [ "$exit_code" != "0" ] ; then
    exit $exit_code #the last exit code != 0
  fi
}

: "${BSIM_OUT_PATH:?BSIM_OUT_PATH must be defined}"

#Give a default value to BOARD if it does not have one yet:
BOARD="${BOARD:-nrf52_bsim}"

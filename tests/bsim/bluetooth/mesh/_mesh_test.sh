# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

EXECUTE_TIMEOUT=300

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

  s_id=$1
  shift 1

  testids=()
  for testid in $@ ; do
    if [ "$testid" == "--" ]; then
      shift 1
      break
    fi

    testids+=( $testid )
    shift 1
  done

  test_options=$@

  for testid in ${testids[@]} ; do
    if Skip $testid; then
      echo "Skipping $testid (device #$idx)"
      let idx=idx+1
      continue
    fi

    echo "Starting $testid as device #$idx"
    conf=${conf:-prj_conf}

    if [ ${overlay} ]; then
        exe_name=./bs_${BOARD}_tests_bsim_bluetooth_mesh_${conf}_${overlay}
    else
        exe_name=./bs_${BOARD}_tests_bsim_bluetooth_mesh_${conf}
    fi

    Execute \
      ${exe_name} \
      -v=${verbosity_level} -s=$s_id -d=$idx -sync_preboot -RealEncryption=1 \
      -testid=$testid ${test_options}
    let idx=idx+1
  done

  count=$(expr $idx + $extra_devs)

  echo "Starting phy with $count devices"

  Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=$s_id -D=$count -argschannel -at=35

  wait_for_background_jobs
}

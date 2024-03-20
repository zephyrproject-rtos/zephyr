# Copyright 2021 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source ${ZEPHYR_BASE}/tests/bsim/sh_common.source

EXECUTE_TIMEOUT=600

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

  declare -A testids
  testid=""
  testid_in_order=()

  for arg in $@ ; do
    if [ "$arg" == "--" ]; then
      shift 1
      break
    fi

    if [[ "$arg" == "-"* ]]; then
        testids["${testid}"]+="$arg "
    else
        testid=$arg
        testid_in_order+=($testid)
        testids["${testid}"]=""
    fi

    shift 1
  done

  test_options=$@

  for testid in ${testid_in_order[@]}; do
    if Skip $testid; then
      echo "Skipping $testid (device #$idx)"
      let idx=idx+1
      continue
    fi

    echo "Starting $testid as device #$idx"
    conf=${conf:-prj_conf}

    if [ ${overlay} ]; then
        exe_name=./bs_${BOARD_TS}_tests_bsim_bluetooth_mesh_${conf}_${overlay}
    else
        exe_name=./bs_${BOARD_TS}_tests_bsim_bluetooth_mesh_${conf}
    fi

    Execute \
      ${exe_name} \
      -v=${verbosity_level} -s=$s_id -d=$idx -sync_preboot -RealEncryption=1 \
      -testid=$testid ${testids["${testid}"]} ${test_options}
    let idx=idx+1
  done

  count=$(expr $idx + $extra_devs)

  echo "Starting phy with $count devices"

  Execute ./bs_2G4_phy_v1 -v=${verbosity_level} -s=$s_id -D=$count -argschannel -at=35

  wait_for_background_jobs
}

function RunTestFlash(){
  s_id=$1
  ext_arg="${s_id} "
  idx=0
  shift 1

  for arg in $@ ; do
    if [ "$arg" == "--" ]; then
      ext_arg+=$@
      break
    fi

    ext_arg+="$arg "

    if [[ "$arg" != "-"* ]]; then
      ext_arg+="-flash=../results/${s_id}/${s_id}_${idx}.bin "
      let idx=idx+1
    fi

    shift 1
  done

  RunTest ${ext_arg}
}

# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

#!/bin/bash

if [[ $1 != "" ]]; then
  BOARD=$1
else
  BOARD=nucleo_f746zg
fi

PROJECT_DIR=${PWD}
OUT_DIR=${PWD}/out
WAMR_DIR=${PWD}/../../../../modules/lib/wasm-micro-runtime
APP_FRAMEWORK_DIR=${WAMR_DIR}/core/app-framework
WASM_APPS=${PWD}/wasm-apps

rm -rf ${OUT_DIR}
mkdir ${OUT_DIR}
mkdir ${OUT_DIR}/zephyr-build
mkdir ${OUT_DIR}/wasm-apps

echo "##################### build wamr sdk"
cd ${WAMR_DIR}/wamr-sdk
./build_sdk.sh -n simple -x ${PROJECT_DIR}/wamr_sdk_config.cmake -c
[ $? -eq 0 ] || exit $?
echo "##################### build wamr sdk done"

echo "##################### build simple project"
cd ${OUT_DIR}/zephyr-build
source ../../../../../zephyr-env.sh
cmake -GNinja -DBOARD=${BOARD} ../..
ninja
echo "##################### build simple project done"

echo "##################### build host-tool"
cd ${WAMR_DIR}/test-tools/host-tool
mkdir -p bin
cd bin
cmake ..
make
if [ $? != 0 ];then
    echo "BUILD_FAIL host tool exit as $?\n"
    exit 2
fi
cp host_tool ${OUT_DIR}
echo "##################### build host-tool done"

echo "##################### build wasm apps"
cd ${WASM_APPS}
for i in `ls *.c`
do
    APP_SRC="$i"
    OUT_FILE=${i%.*}.wasm
    /opt/wasi-sdk/bin/clang -O3 \
    -I${WAMR_DIR}/wamr-sdk/out/simple/app-sdk/wamr-app-framework/include \
    -L${WAMR_DIR}/wamr-sdk/out/simple/app-sdk/wamr-app-framework/lib \
    -lapp_framework \
    -z stack-size=8192 -Wl,--initial-memory=65536 \
    -Wl,--no-entry -nostdlib -Wl,--allow-undefined \
    -Wl,--export=__heap_base,--export=__data_end \
    -Wl,--export=on_init -Wl,--export=on_destroy \
    -Wl,--export=on_request -Wl,--export=on_response \
    -Wl,--export=on_sensor_event -Wl,--export=on_timer_callback \
    -Wl,--export=on_connection_data \
    -o ${OUT_DIR}/wasm-apps/${OUT_FILE} ${APP_SRC}

    if [ -f ${OUT_DIR}/wasm-apps/${OUT_FILE} ]; then
        echo "build ${OUT_FILE} success"
    else
        echo "build ${OUT_FILE} fail"
    fi
done
echo "##################### build wasm apps done"

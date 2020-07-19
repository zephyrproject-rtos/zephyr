# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

#!/bin/bash

PROJECT_DIR=${PWD}
OUT_DIR=${PWD}/out
WAMR_ROOT_DIR=${PWD}/../../../../modules/lib/wasm-micro-runtime
IWASM_ROOT=${WAMR_ROOT_DIR}/core/iwasm
APP_FRAMEWORK_DIR=${WAMR_ROOT_DIR}/core/app-framework
NATIVE_LIBS=${APP_FRAMEWORK_DIR}/app-native-shared
APP_LIB_SRC="${APP_FRAMEWORK_DIR}/base/app/*.c \
			 ${APP_FRAMEWORK_DIR}/sensor/app/*.c \
             ${APP_FRAMEWORK_DIR}/connection/app/*.c \
			 ${NATIVE_LIBS}/*.c"
WASM_APPS=${PWD}/wasm-apps

rm -rf ${OUT_DIR}
mkdir ${OUT_DIR}
mkdir ${OUT_DIR}/zephyr-build
mkdir ${OUT_DIR}/wasm-apps

echo "#####################build wamr sdk"
cd ${WAMR_ROOT_DIR}/wamr-sdk
./build_sdk.sh -n simple -x ${PROJECT_DIR}/wamr_config.cmake -c
[ $? -eq 0 ] || exit $?

echo "#####################build simple project"
cd ${OUT_DIR}/zephyr-build
source ../../../../../zephyr-env.sh
cmake -GNinja -DBOARD=nucleo_f746zg ../..
ninja
echo "#####################build simple project success"

echo "#####################build host-tool"
cd ${WAMR_ROOT_DIR}/test-tools/host-tool
mkdir -p bin
cd bin
cmake ..
make
if [ $? != 0 ];then
    echo "BUILD_FAIL host tool exit as $?\n"
    exit 2
fi
cp host_tool ${OUT_DIR}
echo "#####################build host-tool success"

echo "#####################build wasm apps"
cd ${WASM_APPS}
for i in `ls *.c`
do
    APP_SRC="$i"
    OUT_FILE=${i%.*}.wasm
    /opt/wasi-sdk/bin/clang -O3 \
    -I${WAMR_ROOT_DIR}/wamr-sdk/out/simple/app-sdk/wamr-app-framework/include \
    -L${WAMR_ROOT_DIR}/wamr-sdk/out/simple/app-sdk/wamr-app-framework/lib     \
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
echo "#####################build wasm apps done"

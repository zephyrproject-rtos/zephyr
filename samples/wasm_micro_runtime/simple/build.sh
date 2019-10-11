#!/bin/bash

OUT_DIR=${PWD}/out
WAMR_DIR=${PWD}/../../../../modules/lib/wasm-micro-runtime
IWASM_ROOT=${WAMR_DIR}/core/iwasm
APP_LIBS=${IWASM_ROOT}/lib/app-libs
NATIVE_LIBS=${IWASM_ROOT}/lib/native-interface
APP_LIB_SRC="${APP_LIBS}/base/*.c ${APP_LIBS}/extension/sensor/*.c ${NATIVE_LIBS}/*.c"
WASM_APPS=${PWD}/wasm-apps

rm -rf ${OUT_DIR}
mkdir ${OUT_DIR}
mkdir ${OUT_DIR}/zephyr-build
mkdir ${OUT_DIR}/wasm-apps

echo "#####################build simple project"
cd ${OUT_DIR}/zephyr-build
source ../../../../../zephyr-env.sh
cmake -GNinja -DBOARD=nucleo_f746zg ../..
ninja
echo "#####################build simple project success"

echo "#####################build host-tool"
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
echo "#####################build host-tool success"


echo "#####################build wasm apps"
cd ${WASM_APPS}
for i in `ls *.c`
do
        APP_SRC="$i ${APP_LIB_SRC}"
        OUT_FILE=${i%.*}.wasm
        emcc -O3 -I${APP_LIBS}/base -I${APP_LIBS}/extension/sensor -I${NATIVE_LIBS} \
                -I${APP_LIBS}/extension/connection \
                -s WASM=1 -s SIDE_MODULE=1 -s ASSERTIONS=1 -s STACK_OVERFLOW_CHECK=2 \
                -s TOTAL_MEMORY=65536 -s TOTAL_STACK=4096 \
                -s "EXPORTED_FUNCTIONS=['_on_init', '_on_destroy', '_on_request', '_on_response', \
                '_on_sensor_event', '_on_timer_callback']" \
                -o ${OUT_DIR}/wasm-apps/${OUT_FILE} ${APP_SRC}
        if [ -f ${OUT_DIR}/wasm-apps/${OUT_FILE} ]; then
                echo "build ${OUT_FILE} success"
        else
                echo "build ${OUT_FILE} fail"
        fi
done
echo "#####################build wasm apps done"

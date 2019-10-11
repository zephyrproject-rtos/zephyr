#!/bin/bash

OUT_DIR=${PWD}/out
WAMR_DIR=${PWD}/../../../../modules/lib/wasm-micro-runtime
WASM_APPS=${PWD}/wasm-apps

rm -rf ${OUT_DIR}
mkdir ${OUT_DIR}
mkdir ${OUT_DIR}/zephyr-build

echo "#####################build runtime"
cd ${OUT_DIR}/zephyr-build
source ../../../../../zephyr-env.sh
cmake -GNinja -DBOARD=nucleo_f746zg ../..
ninja
echo "#####################build runtime success"

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

echo "#####################  build wasm ui app start#####################"
cd ${WASM_APPS}
./build_wasm_app.sh
cp ui_app.wasm ${OUT_DIR}/
echo "#####################  build wasm ui app end#####################"

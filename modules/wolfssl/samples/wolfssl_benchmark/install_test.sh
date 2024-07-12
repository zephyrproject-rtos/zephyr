#!/bin/sh

WOLFSSL_SRC_DIR=../../..

if [ ! -d $WOLFSSL_SRC_DIR ]; then
    echo "Directory does not exist: $WOLFSSL_SRC_DIR"
    exit 1
fi
if [ ! -f $WOLFSSL_SRC_DIR/wolfcrypt/benchmark/benchmark.c ]; then
    echo "Missing source file: $WOLFSSL_SRC_DIR/wolfcrypt/benchmark/benchmark.h"
    exit 1
fi

ZEPHYR_DIR=
if [ $# -ne 1 ]; then
    echo "Need location of zephyr project as a command line argument"
    exit 1
else
    ZEPHYR_DIR=$1
fi
if [ ! -d $ZEPHR_DIR ]; then
    echo "Zephyr project directory does not exist: $ZEPHYR_DIR"
    exit 1
fi
ZEPHYR_SAMPLES_DIR=$ZEPHYR_DIR/zephyr/samples/modules
if [ ! -d $ZEPHYR_SAMPLES_DIR ]; then
    echo "Zephyr samples/modules directory does not exist: $ZEPHYR_SAMPLES_DIR"
    exit 1
fi
ZEPHYR_WOLFSSL_DIR=$ZEPHYR_SAMPLES_DIR/wolfssl_benchmark

echo "wolfSSL directory:"
echo "  $ZEPHYR_WOLFSSL_DIR"
rm -rf $ZEPHYR_WOLFSSL_DIR
mkdir $ZEPHYR_WOLFSSL_DIR

echo "Copy in Build files ..."
cp -r * $ZEPHYR_WOLFSSL_DIR/
rm $ZEPHYR_WOLFSSL_DIR/$0

echo "Copy Source Code ..."
rm -rf $ZEPHYR_WOLFSSL_DIR/src
mkdir $ZEPHYR_WOLFSSL_DIR/src

cp -rf ${WOLFSSL_SRC_DIR}/wolfcrypt/benchmark/benchmark.c $ZEPHYR_WOLFSSL_DIR/src/
cp -rf ${WOLFSSL_SRC_DIR}/wolfcrypt/benchmark/benchmark.h $ZEPHYR_WOLFSSL_DIR/src/

echo "Done"


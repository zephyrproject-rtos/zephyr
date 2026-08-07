# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

if [ -z "$RUNNING_FROM_MAIN_SCRIPT" ]; then
    echo "Do not run this script directly!"
    echo "Run $ZEPHYR_BASE/scripts/net/run-sample-tests.sh instead."
    exit 1
fi

start_configuration || return $?
start_zephyr
start_docker "/net-tools/libcoap/examples/etsi_coaptest.sh -i eth0 192.0.2.1" \
    || return $?

wait $docker_pid
result=$?

stop_docker
stop_zephyr

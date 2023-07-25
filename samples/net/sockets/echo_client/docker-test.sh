# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

if [ -z "$RUNNING_FROM_MAIN_SCRIPT" ]; then
    echo "Do not run this script directly!"
    echo "Run $ZEPHYR_BASE/scripts/net/run-sample-tests.sh instead."
    exit 1
fi

start_configuration "--ip=192.0.2.1 --ip6=2001:db8::1" || return $?
start_docker "/net-tools/echo-server -i eth0" || return $?

start_zephyr "$overlay" "-DCONFIG_NET_SAMPLE_SEND_ITERATIONS=10"

wait_zephyr
result=$?

stop_docker

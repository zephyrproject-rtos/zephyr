# Copyright (c) 2023 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if [ -z "$RUNNING_FROM_MAIN_SCRIPT" ]; then
    echo "Do not run this script directly!"
    echo "Run $ZEPHYR_BASE/scripts/net/run-sample-tests.sh instead."
    exit 1
fi

IP="--ip=192.0.2.2 --ip6=2001:db8::2"
FWD="-p 8080:8080 -p 8081:8081 -p 5683:5683/udp"

start_configuration "$IP $FWD" || return $?
start_docker "/net-tools/start-leshan.sh" || return $?

twister -p native_posix -T ./ --enable-slow -vv

result=$?

stop_docker

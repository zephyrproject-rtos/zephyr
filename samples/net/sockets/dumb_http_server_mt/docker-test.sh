# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

if [ -z "$RUNNING_FROM_MAIN_SCRIPT" ]; then
    echo "Do not run this script directly!"
    echo "Run $ZEPHYR_BASE/scripts/net/run-sample-tests.sh instead."
    exit 1
fi

# First the non-tls version
start_configuration || return $?
start_zephyr "$overlay" "-DCONFIG_NET_SAMPLE_SERVE_LARGE_FILE=y" || return $?
start_docker "/usr/local/bin/http-get-file-test.sh 5" || return $?
wait_docker
docker_result=$?

# curl timeout is return code 28. If we get that, zephyr will never
# return so we can just kill it here
if [ $docker_result -eq 28 ]; then
	stop_zephyr
	result=1
else
	sleep 1
	wait_zephyr
	result=$?
fi

stop_docker
stop_zephyr

if [ $result -ne 0 ] || [ $docker_result -ne 0 ]; then
    return $result
fi

# Then the TLS version
if [ -n "$zephyr_overlay" ]; then
	overlay="${zephyr_overlay};overlay-tls.conf"
else
	overlay="-DEXTRA_CONF_FILE=overlay-tls.conf"
fi

start_configuration || return $?
start_zephyr "$overlay" "-DCONFIG_NET_SAMPLE_SERVE_LARGE_FILE=y" || return $?
start_docker "/usr/local/bin/https-get-file-test.sh 5" || return $?
wait_docker
docker_result=$?

# curl timeout is return code 28. If we get that, zephyr will never
# return so we can just kill it here
if [ $docker_result -eq 28 ]; then
	stop_zephyr
	result=1
else
	sleep 1
	wait_zephyr
	result=$?
fi

stop_docker
stop_zephyr

if [ $result -ne 0 ] || [ $docker_result -ne 0 ]; then
    return $result
fi

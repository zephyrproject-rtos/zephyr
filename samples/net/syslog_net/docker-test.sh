# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

if [ -z "$RUNNING_FROM_MAIN_SCRIPT" ]; then
    echo "Do not run this script directly!"
    echo "Run $ZEPHYR_BASE/scripts/net/run-sample-tests.sh instead."
    exit 1
fi

MSG_COUNT=5

start_configuration || return $?

# First IPv6
start_docker "/usr/local/bin/syslog-receiver.py 2001:db8::2" || return $?
start_zephyr "$overlay" "-DCONFIG_LOG_BACKEND_NET_SERVER=\"[2001:db8::2]:514\"" \
             "-DCONFIG_NET_SAMPLE_SEND_ITERATIONS=$MSG_COUNT"
wait $docker_pid
docker_result=$?
wait_zephyr
result=$?
stop_docker
if [ $result -ne 0 ] || [ $docker_result -ne 0 ]; then
    return ${result}${docker_result}
fi

# Then IPv4
start_docker "/usr/local/bin/syslog-receiver.py 192.0.2.2" || return $?
start_zephyr "$overlay" "-DCONFIG_LOG_BACKEND_NET_SERVER=\"192.0.2.2:514\"" \
             "-DCONFIG_NET_SAMPLE_SEND_ITERATIONS=$MSG_COUNT"
wait $docker_pid
docker_result=$?
wait_zephyr
result=$?
stop_docker
if [ $result -ne 0 ] || [ $docker_result -ne 0 ]; then
    return ${result}${docker_result}
fi

return 0

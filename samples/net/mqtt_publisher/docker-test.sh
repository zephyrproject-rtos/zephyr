# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

if [ -z "$RUNNING_FROM_MAIN_SCRIPT" ]; then
    echo "Do not run this script directly!"
    echo "Run $ZEPHYR_BASE/scripts/net/run-sample-tests.sh instead."
    exit 1
fi

echo "Starting MQTT test"

start_configuration || return $?
start_docker \
    "/usr/local/sbin/mosquitto -v -c /usr/local/etc/mosquitto/mosquitto.conf" || return $?

start_zephyr -DOVERLAY_CONFIG=overlay-sample.conf "$overlay"

wait_zephyr
result=$?

if [ $result -ne 0 ]; then
	return $result
fi

stop_docker

# test TLS
echo "Starting MQTT TLS test"

start_docker \
    "/usr/local/sbin/mosquitto -v -c /usr/local/etc/mosquitto/mosquitto-tls.conf" || return $?

start_zephyr -DOVERLAY_CONFIG="overlay-tls.conf overlay-sample.conf" "$overlay"

wait_zephyr
result=$?

if [ $result -ne 0 ]; then
	return $result
fi

stop_docker
return $result

# FIXME: proxy test does not work as expected

# TLS and SOCKS5, mosquitto TLS is already running
echo "Starting MQTT TLS + proxy test"

start_docker "/usr/sbin/danted" || return $?

start_zephyr -DOVERLAY_CONFIG="overlay-tls.conf overlay-sample.conf overlay-socks5.conf" "$overlay" || return $?

wait_zephyr
result=$?

stop_docker

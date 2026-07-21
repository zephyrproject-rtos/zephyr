# Copyright (c) 2020 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

if [ -z "$RUNNING_FROM_MAIN_SCRIPT" ]; then
    echo "Do not run this script directly!"
    echo "Run $ZEPHYR_BASE/scripts/net/run-sample-tests.sh instead."
    exit 1
fi

# First the non-tls version
echo "HTTP client test"
start_configuration "--ip=192.0.2.2 --ip6=2001:db8::2" || return $?
start_docker "/usr/local/bin/http-server.py" || return $?
start_zephyr "$overlay" "-DCONFIG_NET_SAMPLE_SEND_ITERATIONS=5"
wait_zephyr
result=$?
stop_docker

if [ $result -ne 0 ]; then
	return $result
fi

return $result

# If everything is ok so far, do the TLS version
echo "HTTPS client test"

if [ -n "$zephyr_overlay" ]; then
	overlay="${zephyr_overlay};overlay-tls.conf"
else
	overlay="-DEXTRA_CONF_FILE=overlay-tls.conf"
fi

start_configuration "--ip=192.0.2.2 --ip6=2001:db8::2" || return $?
start_docker "/usr/local/bin/https-server.py 192.0.2.2" \
             "/usr/local/bin/https-server.py 2001:db8::2" || return $?
start_zephyr "$overlay" "-DCONFIG_NET_SAMPLE_SEND_ITERATIONS=5"
wait_zephyr
result=$?
stop_docker

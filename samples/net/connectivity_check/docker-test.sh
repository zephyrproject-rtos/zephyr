# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if [ -z "$RUNNING_FROM_MAIN_SCRIPT" ]; then
    echo "Do not run this script directly!"
    echo "Run $ZEPHYR_BASE/scripts/net/run-sample-tests.sh instead."
    exit 1
fi

# First the non-tls version
echo "Connectivity check using HTTP"
start_configuration "--ip=192.0.2.2 --ip6=2001:db8::2 " \
		    "-e DNSMASQ_USER=root --cap-add CAP_NET_ADMIN" || return $?
start_docker "/usr/local/bin/connectivity-check.sh" || return $?

start_zephyr_and_wait_str '^DONE!' 3m "$overlay" \
			  "-DEXTRA_CONF_FILE=overlay-testing-http.conf" \
			  "-DCONFIG_NET_SAMPLE_ONLINE_COUNT=1"
result=$?

wait_zephyr
stop_docker

if [ $result -ne 0 ]; then
	return $result
fi

# If everything is ok so far, do the TLS version
echo "Connectivity check using HTTPS"

if [ -n "$zephyr_overlay" ]; then
	overlay="${zephyr_overlay};overlay-tls.conf"
else
	overlay="-DOVERLAY_CONFIG=overlay-tls.conf"
fi

start_configuration "--ip=192.0.2.2 --ip6=2001:db8::2 " \
		    "-e DNSMASQ_USER=root --cap-add CAP_NET_ADMIN" || return $?
start_docker "/usr/local/bin/connectivity-check.sh" || return $?

# For the HTTP test we enable private address check. This has the effect
# that the test loops/does checks couple of times and then quits.
start_zephyr_and_wait_str '^DONE!' 3m "$overlay" \
			  "-DEXTRA_CONF_FILE=overlay-testing-https.conf" \
			  "-DCONFIG_NET_SAMPLE_ONLINE_COUNT=1"
result=$?

wait_zephyr
stop_docker

return $result

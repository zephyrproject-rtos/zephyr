#!/bin/sh
# Copyright (c) 2019 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

image=net-tools
name=net-tools
network=net-tools0
zephyr_pid=0
docker_pid=0
configuration=""
result=0
sample=""
zephyr_overlay=""

check_dirs ()
{
    local ret_zephyr=0
    local ret_net_tools=0

    if [ -z "$ZEPHYR_BASE" ]
    then
	echo '$ZEPHYR_BASE is unset' >&2
	ret_zephyr=1
    elif [ ! -d "$ZEPHYR_BASE" ]
    then
	echo '$ZEPHYR_BASE is set, but it is not a directory' >&2
	ret_zephyr=1
    fi

    if [ -z "$NET_TOOLS_BASE" ]
    then
	local d

	for d in "$ZEPHYR_BASE/../.." "$ZEPHYR_BASE/.."
	do
	    local l

	    l="$d/net-tools"
	    if [ -d "$l" ]
	    then
		NET_TOOLS_BASE="$l"
		break
	    fi
	done
    fi

    if [ $ret_zephyr -eq 0 ]
    then
	echo "\$ZEPHYR_BASE $ZEPHYR_BASE"
    fi

    if [ -z "$NET_TOOLS_BASE" ]
    then
	echo '$NET_TOOLS_BASE is unset, no net-tools found' >&2
	ret_net_tools=1
    elif [ ! -d "$NET_TOOLS_BASE" ]
    then
	echo '$NET_TOOLS_BASE set, but it is not a directory' >&2
	ret_net_tools=1
    fi

    if [ $ret_net_tools -eq 0 ]
    then
	echo "\$NET_TOOLS_BASE $NET_TOOLS_BASE"
    fi

    if [ $ret_zephyr -ne 0 -o $ret_net_tools -ne 0 ]
    then
	return 1
    fi

    return 0
}

start_configuration ()
{
    local bridge_interface=""
    local addresses="--ip=192.0.2.2 --ip6=2001:db8::2"

    if ! docker image ls | grep "$image" > /dev/null
    then
	echo "Docker image '$image' not found" >&2
	return 1
    fi

    if ! docker network ls | grep "$network" > /dev/null
    then
	bridge_interface=$("$NET_TOOLS_BASE/net-setup.sh" \
			   --config "$NET_TOOLS_BASE/docker.conf" \
			   start 2>/dev/null | tail -1)
	if [ $? != 0 ]
	then
	    echo "Could not start Docker network '$network'" >&2
	    return 1
	fi

	echo "Started Docker network '$network' bridge interface" \
	     "'$bridge_interface'..."
    fi

    if [ -n "$*" ]
    then
	addresses="$*"
    fi

    if docker ps | grep "$name" > /dev/null
    then
	docker stop "$name"
    fi

    if docker run --hostname=$name --name=$name \
	 $addresses \
         --rm -dit --network=$network $image > /dev/null
    then
	echo -n "Started Docker container '$name'"
	if [ -n "$*" ]
	then
	    echo -n " with extra arguments '$*'"
	fi
	echo "..."
    else
	echo "Could not start Docker container '$image'"
	return 1
    fi
}

stop_configuration ()
{
    local bridge_interface=""

    if docker ps | grep "$name" > /dev/null
    then
	docker stop "$name" > /dev/null
	if [ $? -eq 0 ]
	then
	    echo "Stopped Docker container '$name'..."
	else
	    echo "Could not stop Docker container '$name'" >&2
	fi
    fi

    bridge_interface=$(docker network ls | grep "$network" | cut -d " " -f 1)
    if [ -n "$bridge_interface" ]
    then
	docker network rm "$network" > /dev/null
	if [ $? -eq 0 ]
	then
	    echo "Stopped Docker network '$network' bridge interface" \
		 "'br-$bridge_interface'..."
	else
	    echo "Could not stop Docker network '$network'" >&2
	fi
    fi
}

start_zephyr ()
{
    if [ -n "$*" ]
    then
	echo "Building Zephyr with additional arguments '$@'..." >&2
    fi

    rm -rf build && mkdir build && \
	cmake -GNinja -DBOARD=native_posix -B build "$@" && \
	ninja -C build

    ninja -C build run &
    zephyr_pid=$!

    sleep 3
    echo "Zephyr PID $zephyr_pid"
}

list_children () {
    local pid="$(ps -o pid= --ppid "$1")"

    for p in $pid
    do
	list_children $p
    done

    if [ -n "$pid" ]
    then
	echo -n "$pid "
    fi
}

stop_zephyr ()
{
    if [ "$zephyr_pid" -ne 0 ]
    then
	local zephyrs="$zephyr_pid $(list_children "$zephyr_pid")"

	echo "Stopping Zephyr PIDs $zephyrs"
	kill $zephyrs
    fi

    zephyr_pid=0
}

wait_zephyr ()
{
    local result=""

    echo "Waiting for Zephyr $zephyr_pid..."
    wait $zephyr_pid
    result=$?

    zephyr_pid=0

    return $result
}


docker_run ()
{
    local test=""
    local result=0

    for test in "$@"
    do
	echo "Running '$test' in the container..."
	docker container exec net-tools $test || return $?
    done
}

start_docker ()
{
    docker_run "$@" &
    docker_pid=$!

    echo "Docker PID $docker_pid"
}

stop_docker ()
{
    if [ "$docker_pid" -ne 0 -a "$configuration" != "keep" ]
    then
	local dockers="$docker_pid $(list_children "$docker_pid")"

	echo "Stopping Docker PIDs $dockers"
	kill $dockers
    fi

    docker_pid=0
}

wait_docker ()
{
    local result=""

    echo "Waiting for Docker PID $docker_pid..."
    wait $docker_pid
    result=$?

    docker_pid=0

    echo "Docker returned '$result'"
    return $result
}

docker_exec ()
{
    local result=0
    local overlay=""

    if [ -n "$zephyr_overlay" ]
    then
       overlay="-DOVERLAY_CONFIG=$zephyr_overlay"
    fi

    case "$1" in
	echo_server)
	    start_configuration || return $?
	    start_zephyr "$overlay" || return $?

	    start_docker \
		"/net-tools/echo-client -i eth0 192.0.2.1" \
		"/net-tools/echo-client -i eth0 2001:db8::1" \
		"/net-tools/echo-client -i eth0 192.0.2.1 -t" \
		"/net-tools/echo-client -i eth0 2001:db8::1 -t"

	    wait_docker
	    result=$?

	    stop_zephyr
	    ;;

	echo_client)
	    start_configuration "--ip=192.0.2.1 --ip6=2001:db8::1" || return $?
	    start_docker \
		"/net-tools/echo-server -i eth0" || return $?

	    start_zephyr "$overlay" \
			 "-DCONFIG_NET_SAMPLE_SEND_ITERATIONS=10"

	    wait_zephyr
	    result=$?

	    stop_docker
	    ;;

	coap_server)
	    start_configuration || return $?
	    start_zephyr
	    start_docker "/net-tools/libcoap/examples/etsi_coaptest.sh \
			-i eth0 192.0.2.1" || return $?
	    wait $docker_pid
	    result=$?

	    stop_zephyr
	    ;;

	mqtt_publisher)
	    start_configuration || return $?
	    start_docker "/usr/local/sbin/mosquitto -v
			  -c /usr/local/etc/mosquitto/mosquitto.conf" || \
			      return $?

	    start_zephyr -DOVERLAY_CONFIG=overlay-sample.conf "$overlay"

	    wait_zephyr
	    result=$?

	    if [ $result -ne 0 ]
	    then
		break
	    fi

	    # test TLS
	    start_docker "/usr/local/sbin/mosquitto -v
			  -c /usr/local/etc/mosquitto/mosquitto-tls.conf" || \
			      return $?

	    start_zephyr \
		-DOVERLAY_CONFIG="overlay-tls.conf overlay-sample.conf" \
		 "$overlay"

	    wait_zephyr
	    result=$?

	    if [ $result -ne 0 ]
	    then
		break
	    fi

	    # TLS and SOCKS5, mosquitto TLS is already running
	    start_docker "/usr/sbin/danted" || \
		return $?

	    start_zephyr \
		-DOVERLAY_CONFIG="overlay-tls.conf overlay-sample.conf overlay-socks5.conf" \
		 "$overlay" || return $?

	    wait_zephyr
	    result=$?

	    stop_docker

	    return $result
	    ;;

	*)
	    echo "No sample test corresponding to directory '$1' found" >&2
	    return 1
	    ;;
    esac

    return $result
}

run_test ()
{
    local test="$(basename $(pwd))"
    local result=0

    if [ -n "$1" ]
    then
	source "$1"
	result=$?
    else
	docker_exec "$test"
	result=$?
    fi

    if [ $result -eq 0 ]
    then
	echo "Sample '$test' successful"
    else
	echo "Sample '$test' failed with return value '$result'"
    fi

    return $result
}

usage ()
{
    echo "$0 [-Z <zephyr base directory>] [-N <net-tools base directory>]"
    echo "\t[<test script>]"
    echo "This script runs Zephyr sample tests using Docker container and"
    echo "network implemented by the 'net-tools' subproject."
    echo "-Z|--zephyr-dir <dir>\t\tset Zephyr base directory"
    echo "-N|--net-tools-dir <dir>\tset net-tools directory"
    echo "--start\t\t\t\tonly start Docker container and network and exit"
    echo "--stop\t\t\t\tonly stop Docker container and network"
    echo "--keep\t\t\t\tkeep Docker container and network after test"
    echo -n "--overlay <config files>\tadditional configuration/overlay "
    echo "files for the\n\t\t\t\tZephyr build process"
    echo "<test script>\t\t\tsample script to run instead of test based on"
    echo "\t\t\t\tcurrent directory"
    echo "The automatically detected directories are:"
    check_dirs
}

stop_sample_test () {
    echo "Interrupted..." >&2

    stop_zephyr
    stop_docker

    stop_configuration
    exit 2
}

trap stop_sample_test ABRT INT HUP TERM

while test -n "$1"
do
    case "$1" in
	-Z|--zephyr-dir)
	    shift
	    ZEPHYR_BASE="$1"
	    ;;

	-N|--net-tools-dir)
	    shift
	    NET_TOOLS_BASE="$1"
	    ;;

	-h|--help)
	    usage
	    return 0
	    ;;
	--start)
	    if [ -n "$configuration" ]
	    then
		echo "--start or --stop specified multiple times" >&2
		return 1
	    fi
	    configuration=start_only
	    ;;
	--stop)
	    if [ -n "$configuration" ]
	    then
		echo "--start or --stop specified multiple times" >&2
		return 1
	    fi
	    configuration=stop_only
	    ;;
	--keep)
	    configuration=keep
	    ;;

	--overlay)
	    shift
	    if [ -n "$zephyr_overlay" ]
	    then
		zephyr_overlay="$zephyr_overlay $1"
	    else
		zephyr_overlay="$1"
	    fi
	    ;;

	-*)
	    echo "Argument '$1' not recognised" >&2
	    usage
	    return 0
	    ;;
	*)
	    if [ -n "$sample" ]
	    then
		echo "Sample already specified" >&2
		return 1
	    fi
	    sample="$1"
	    ;;
    esac

    shift
done

check_dirs || exit $?

if [ -z "$configuration" -o "$configuration" = "start_only" -o \
	"$configuration" = "keep" ]
then
    if [ "$configuration" = start_only ]
    then
	start_configuration
	result=$?
    else
	run_test "$sample"
	result=$?
    fi
fi

if [ -z "$configuration" -o "$configuration" = stop_only ]
then
    stop_configuration
fi

return $result

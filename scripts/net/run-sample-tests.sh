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
dirs=""
zephyr_overlay=""
docker_test_script_name=docker-test.sh
scan_dirs=0

RUNNING_FROM_MAIN_SCRIPT=1

check_dirs ()
{
    local ret_zephyr=0
    local ret_net_tools=0

    if [ -z "$ZEPHYR_BASE" ]; then
	    echo '$ZEPHYR_BASE is unset' >&2
	    ret_zephyr=1
    elif [ ! -d "$ZEPHYR_BASE" ]; then
	    echo '$ZEPHYR_BASE is set, but it is not a directory' >&2
	    ret_zephyr=1
    fi

    if [ -z "$NET_TOOLS_BASE" ]; then
	    local d

	    for d in "$ZEPHYR_BASE/../.." "$ZEPHYR_BASE/.."
	    do
	        local l

	        l="$d/net-tools"
	        if [ -d "$l" ]; then
		        NET_TOOLS_BASE="$l"
		        break
	        fi
	    done
    fi

    if [ $ret_zephyr -eq 0 ]; then
	    echo "\$ZEPHYR_BASE $ZEPHYR_BASE"
    fi

    if [ -z "$NET_TOOLS_BASE" ]; then
	    echo '$NET_TOOLS_BASE is unset, no net-tools found' >&2
	    ret_net_tools=1
    elif [ ! -d "$NET_TOOLS_BASE" ]; then
	    echo '$NET_TOOLS_BASE set, but it is not a directory' >&2
	    ret_net_tools=1
    fi

    if [ $ret_net_tools -eq 0 ]; then
	    echo "\$NET_TOOLS_BASE $NET_TOOLS_BASE"
    fi

    if [ $ret_zephyr -ne 0 -o $ret_net_tools -ne 0 ]; then
	    return 1
    fi

    return 0
}

scan_dirs ()
{
    echo
    echo "Following directories under $ZEPHYR_BASE can be used by this script:"
    find "$ZEPHYR_BASE" -type f -name $docker_test_script_name | \
        awk -F "$ZEPHYR_BASE/" '{ print $2 }' | \
        sed "s/\/$docker_test_script_name//"
}

start_configuration ()
{
    local bridge_interface=""
    local addresses="--ip=192.0.2.2 --ip6=2001:db8::2"

    if ! docker image ls | grep "$image" > /dev/null; then
	    echo "Docker image '$image' not found" >&2
	    return 1
    fi

    if ! docker network ls | grep "$network" > /dev/null; then
	    bridge_interface=$("$NET_TOOLS_BASE/net-setup.sh" \
		                   --config "$NET_TOOLS_BASE/docker.conf" \
			               start 2>/dev/null | tail -1)
	    if [ $? != 0 ]; then
	        echo "Could not start Docker network '$network'" >&2
	        return 1
	    fi

	    echo "Started Docker network '$network' bridge interface" \
	         "'$bridge_interface'..."
    fi

    if [ -n "$*" ]; then
	    addresses="$*"
    fi

    if docker ps | grep "$name" > /dev/null; then
	    docker stop "$name"
    fi

    if docker run --hostname=$name --name=$name $addresses \
              --rm -dit --network=$network $image > /dev/null; then
	    echo -n "Started Docker container '$name'"
	    if [ -n "$*" ]; then
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

    if docker ps | grep "$name" > /dev/null; then
	    docker stop "$name" > /dev/null
	    if [ $? -eq 0 ]; then
	        echo "Stopped Docker container '$name'..."
	    else
	        echo "Could not stop Docker container '$name'" >&2
	    fi
    fi

    bridge_interface=$(docker network ls | grep "$network" | cut -d " " -f 1)
    if [ -n "$bridge_interface" ]; then
	    docker network rm "$network" > /dev/null
	    if [ $? -eq 0 ]; then
	        echo "Stopped Docker network '$network' bridge interface" \
		         "'br-$bridge_interface'..."
	    else
	        echo "Could not stop Docker network '$network'" >&2
	    fi
    fi

    # No need to keep the zephyr eth interface around
    "$NET_TOOLS_BASE/net-setup.sh" --config "$NET_TOOLS_BASE/docker.conf" \
			               stop > /dev/null 2>&1
}

start_zephyr ()
{
    if [ -n "$*" ]; then
	    echo "Building Zephyr with additional arguments '$@'..." >&2
    fi

    rm -rf build && mkdir build && \
	cmake -GNinja -DBOARD=native_posix -B build "$@" . && \
	ninja -C build

    # Run the binary directly so that ninja does not print errors that
    # could be confusing.
    #ninja -C build run &
    build/zephyr/zephyr.exe &
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

    if [ -n "$pid" ]; then
	    echo -n "$pid "
    fi
}

stop_zephyr ()
{
    if [ "$zephyr_pid" -ne 0 ]; then
	    local zephyrs="$zephyr_pid $(list_children "$zephyr_pid")"

	    echo "Stopping Zephyr PIDs $zephyrs"
	    kill $zephyrs 2> /dev/null
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
    if [ "$docker_pid" -ne 0 -a "$configuration" != "keep" ]; then
	    local dockers="$docker_pid $(list_children "$docker_pid")"

	    echo "Stopping Docker PIDs $dockers"
	    kill $dockers 2> /dev/null
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

run_test ()
{
    local test="$(basename $(pwd))"
    local result=0
    local overlay=""

    if [ -n "$zephyr_overlay" ]; then
        overlay="-DOVERLAY_CONFIG=$zephyr_overlay"
    fi

	source "$1"
	result=$?

    if [ $result -eq 0 ]; then
	    echo "Sample '$test' successful"
    else
	    echo "Sample '$test' failed with return value '$result'"
    fi

    return $result
}

usage ()
{
    BASENAME=`basename $0`

    cat <<EOF

$BASENAME [-Z <zephyr base directory>] [-N <net-tools base directory>] [<list of test directories>]

This script runs Zephyr sample tests using Docker container and
network implemented by the 'net-tools' subproject.

-Z|--zephyr-dir <dir>
	set Zephyr base directory
-N|--net-tools-dir <dir>
	set net-tools directory
--start
	only start Docker container and network and exit
--stop
	only stop Docker container and network
--keep
	keep Docker container and network after test
--overlay <config files>
	additional configuration/overlay files for the Zephyr build process
--scan
    find out the directories that can be used for this testing
<list of test directories>
	run the tests in these directories instead of current directory

The automatically detected directories are:
EOF
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
	    exit
	    ;;
	--start)
	    if [ -n "$configuration" ]; then
		    echo "--start or --stop specified multiple times" >&2
		    exit 1
	    fi
	    configuration=start_only
	    ;;
	--stop)
	    if [ -n "$configuration" ]; then
		    echo "--start or --stop specified multiple times" >&2
		    exit 1
	    fi
	    configuration=stop_only
	    ;;
	--keep)
	    configuration=keep
	    ;;

	--overlay)
	    shift
	    if [ -n "$zephyr_overlay" ]; then
		    zephyr_overlay="$zephyr_overlay $1"
	    else
		    zephyr_overlay="$1"
	    fi
	    ;;

    --scan)
        scan_dirs=1
        ;;
	-*)
	    echo "Argument '$1' not recognised" >&2
	    usage
	    return 0
	    ;;
	*)
	    dirs="$dirs $1"
	    ;;
    esac

    shift
done

check_dirs || exit $?

if [ $scan_dirs -eq 1 ]; then
    scan_dirs
    exit 0
fi

if [ -z "$configuration" -o "$configuration" = "start_only" -o \
	"$configuration" = "keep" ]; then
    if [ "$configuration" = start_only ]; then
	    start_configuration
	    result=$?
    else
        result=0
        found=0

        if [ -z "$dirs" ]; then
            dirs="."
        fi

        for d in $dirs; do
            if [ ! -d "$d" ]; then
                echo "No such directory $d, skipping it"
                continue
            fi

            if [ ! -f "$d/$docker_test_script_name" ]; then
                echo "No such file $d/$docker_test_script_name, skipping directory"
                continue
            fi

            found=$(expr $found + 1)

            CURR_DIR=`pwd`
            cd "$d"
	        run_test "./$docker_test_script_name"
	        test_result=$?
            cd "$CURR_DIR"

            if [ $test_result -ne 0 ]; then
                result=1
            fi
        done

        if [ $found -eq 0 ]; then
            exit 1
        fi
    fi
fi

if [ -z "$configuration" -o "$configuration" = stop_only ]; then
    stop_configuration
fi

exit $result

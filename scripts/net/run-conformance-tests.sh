#!/bin/bash
# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
#
# Run the TTCN-3 protocol conformance suites the way the nightly workflow does,
# but from a developer's own checkout. See the documentation under
# doc/services/connectivity/networking/conformance for what the suites are.

all_suites="mdns dnssd dns sntp coap dhcpv4 dhcpv4_server arp ndp tcp"
suites=""
outdir=""
keep=0
fetch=0
action="run"
created_zeth=0
created_zethL2=0
result=0

# The suites that answer on the shared interface all claim the same address, and
# the harness serialises them with a lock keyed on the effective user id. A
# privileged run and an unprivileged one would take different locks and collide,
# so everything has to go through a single Twister invocation.

find_dirs ()
{
	local d

	if [ -z "$ZEPHYR_BASE" ]; then
		ZEPHYR_BASE="$(cd "$(dirname "$0")/../.." && pwd)"
	fi

	if [ ! -d "$ZEPHYR_BASE" ]; then
		echo "\$ZEPHYR_BASE is set, but it is not a directory" >&2
		return 1
	fi

	if [ -z "$NET_TOOLS_BASE" ]; then
		for d in "$ZEPHYR_BASE/.." "$ZEPHYR_BASE/../.."
		do
			if [ -d "$d/tools/net-tools" ]; then
				NET_TOOLS_BASE="$(cd "$d/tools/net-tools" && pwd)"
				break
			fi
		done
	fi

	if [ -z "$NET_TOOLS_BASE" ]; then
		echo "No net-tools found. Use --net-tools-dir to point at it." >&2
		return 1
	fi

	if [ ! -d "$NET_TOOLS_BASE/ttcn3" ]; then
		echo "'$NET_TOOLS_BASE' has no ttcn3 directory." >&2
		echo "The conformance suites need a net-tools with TTCN-3 support." >&2
		return 1
	fi

	return 0
}

# The same three traits the pytest harness reads, from the same file, so that
# the script and the harness cannot disagree about what a suite needs.
suite_trait ()
{
	local conf="$NET_TOOLS_BASE/ttcn3/suites/$1/build.conf"

	[ -f "$conf" ] && grep -q "^$2\$" "$conf"
}

needs_root ()
{
	suite_trait "$1" "PRIVILEGED=yes"
}

needs_l2 ()
{
	suite_trait "$1" "L2=yes"
}

is_parallel ()
{
	suite_trait "$1" "MODE=parallel"
}

check_suites ()
{
	local suite
	local known

	for suite in $suites
	do
		known=0

		for k in $all_suites
		do
			if [ "$suite" = "$k" ]; then
				known=1
				break
			fi
		done

		if [ $known -eq 0 ]; then
			echo "Unknown suite '$suite'." >&2
			echo "Known suites: $all_suites" >&2
			return 1
		fi

		if [ ! -d "$ZEPHYR_BASE/tests/net/conformance/$suite" ]; then
			echo "No system under test for '$suite' in this tree." >&2
			return 1
		fi
	done

	return 0
}

check_titan ()
{
	if [ -z "$TTCN3_DIR" ]; then
		echo "\$TTCN3_DIR is unset, so there is no Titan to build the suites with." >&2
		echo "On Debian and Ubuntu:" >&2
		echo "    sudo apt install --no-install-recommends eclipse-titan expect" >&2
		echo "    export TTCN3_DIR=/usr" >&2
		return 1
	fi

	if [ ! -x "$TTCN3_DIR/bin/compiler" ]; then
		echo "'$TTCN3_DIR' does not look like a Titan installation." >&2
		echo "No compiler found at '$TTCN3_DIR/bin/compiler'." >&2
		return 1
	fi

	if ! command -v make > /dev/null; then
		echo "'make' is not installed, and Titan builds a suite with a makefile." >&2
		return 1
	fi

	for suite in $suites
	do
		if is_parallel "$suite" && [ ! -x "$TTCN3_DIR/bin/ttcn3_start" ]; then
			echo "The '$suite' suite needs Titan's main controller," >&2
			echo "and there is no ttcn3_start in '$TTCN3_DIR/bin'." >&2
			return 1
		fi
	done

	return 0
}

fetch_modules ()
{
	local ttcn3="$NET_TOOLS_BASE/ttcn3"

	if [ $fetch -eq 0 -a -d "$ttcn3/modules" ]; then
		return 0
	fi

	echo "Fetching the third party TTCN-3 modules..."
	"$ttcn3/fetch-modules.sh" || return 1
}

iface_exists ()
{
	[ -d "/sys/class/net/$1" ]
}

start_interfaces ()
{
	local suite
	local want_l2=0

	for suite in $suites
	do
		if needs_l2 "$suite"; then
			want_l2=1
		fi
	done

	if ! iface_exists zeth; then
		echo "Creating the zeth interface..."
		"$NET_TOOLS_BASE/net-setup.sh" --config "$NET_TOOLS_BASE/zeth.conf" \
			--iface zeth start || return 1
		created_zeth=1
	fi

	if [ $want_l2 -eq 1 ] && ! iface_exists zethL2; then
		echo "Creating the zethL2 interface..."
		"$NET_TOOLS_BASE/net-setup.sh" --config "$NET_TOOLS_BASE/zeth-l2.conf" \
			--iface zethL2 start || return 1
		created_zethL2=1
	fi

	return 0
}

stop_interfaces ()
{
	# Only remove what this run created, so that an interface someone else set
	# up for other work is left alone.
	if [ $created_zeth -eq 1 ]; then
		echo "Removing the zeth interface..."
		"$NET_TOOLS_BASE/net-setup.sh" --config "$NET_TOOLS_BASE/zeth.conf" \
			--iface zeth stop > /dev/null 2>&1
		created_zeth=0
	fi

	if [ $created_zethL2 -eq 1 ]; then
		echo "Removing the zethL2 interface..."
		"$NET_TOOLS_BASE/net-setup.sh" --config "$NET_TOOLS_BASE/zeth-l2.conf" \
			--iface zethL2 stop > /dev/null 2>&1
		created_zethL2=0
	fi
}

stop_all_interfaces ()
{
	created_zeth=1
	created_zethL2=1
	stop_interfaces
}

# A privileged run creates files as root in directories the invoking user owns.
# Keep the ones we can out of the tree entirely, and hand back the rest.
restore_ownership ()
{
	local owner

	if [ -z "$SUDO_UID" -o -z "$SUDO_GID" ]; then
		return 0
	fi

	owner="$SUDO_UID:$SUDO_GID"

	chown -R "$owner" "$outdir" 2> /dev/null
	chown -R "$owner" "$NET_TOOLS_BASE/ttcn3/suites"/*/build 2> /dev/null
	chown -R "$owner" "$pytest_cache" 2> /dev/null
}

run_twister ()
{
	local args=""
	local suite

	if [ "$suites" != "$all_suites" ]; then
		for suite in $suites
		do
			args="$args -s net.conformance.$suite"
		done
	fi

	# Python and pytest both write into the source tree by default, and as root
	# that leaves directories the invoking user can no longer write.
	export PYTHONDONTWRITEBYTECODE=1
	export PYTEST_ADDOPTS="-o cache_dir=$pytest_cache${PYTEST_ADDOPTS:+ $PYTEST_ADDOPTS}"

	echo "Running: $suites"

	"$ZEPHYR_BASE/scripts/twister" -p native_sim --enable-slow --inline-logs \
		-O "$outdir" -T "$ZEPHYR_BASE/tests/net/conformance" $args
}

list_suites ()
{
	local suite
	local iface
	local privilege
	local mode

	printf '%-8s %-10s %-10s %s\n' "SUITE" "INTERFACE" "RUNS AS" "MODE"

	for suite in $all_suites
	do
		if needs_l2 "$suite"; then
			iface="zethL2"
		else
			iface="zeth"
		fi

		if needs_root "$suite"; then
			privilege="root"
		else
			privilege="any user"
		fi

		if is_parallel "$suite"; then
			mode="parallel (needs expect)"
		else
			mode="single"
		fi

		printf '%-8s %-10s %-10s %s\n' "$suite" "$iface" "$privilege" "$mode"
	done
}

usage ()
{
	local basename

	basename="$(basename "$0")"

	cat <<EOF

$basename [options] [suite ...]

Run the TTCN-3 protocol conformance suites against Zephyr. With no suite names,
all of them are run. The suites themselves live in the net-tools repository;
this script finds them, creates the interfaces they need, runs Twister once and
tidies up afterwards.

-Z|--zephyr-dir <dir>
	set the Zephyr base directory
-N|--net-tools-dir <dir>
	set the net-tools directory
-T|--titan-dir <dir>
	set the Eclipse Titan installation directory (\$TTCN3_DIR)
-O|--outdir <dir>
	Twister output directory, twister-out by default
--start
	only create the network interfaces and exit
--stop
	only remove the network interfaces
--keep
	leave the network interfaces up afterwards
--fetch
	fetch the third party TTCN-3 modules even if they are already there
--list
	list the suites and what each one needs
-h|--help
	this help

Three suites bind a privileged port or open a packet socket, so a run that
includes any of them re-runs itself under sudo. Naming only the unprivileged
suites avoids that:

    $basename mdns dns coap

The detected directories are:
EOF
	if find_dirs; then
		echo "	\$ZEPHYR_BASE    $ZEPHYR_BASE"
		echo "	\$NET_TOOLS_BASE $NET_TOOLS_BASE"
	fi

	if [ -n "$TTCN3_DIR" ]; then
		echo "	\$TTCN3_DIR      $TTCN3_DIR"
	else
		echo "	\$TTCN3_DIR      (unset)"
	fi

	echo
}

interrupted ()
{
	echo "Interrupted..." >&2

	restore_ownership
	stop_interfaces
	exit 2
}

trap interrupted ABRT INT HUP TERM

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

		-T|--titan-dir)
			shift
			TTCN3_DIR="$1"
			;;

		-O|--outdir)
			shift
			outdir="$1"
			;;

		--start)
			action="start"
			;;

		--stop)
			action="stop"
			;;

		--keep)
			keep=1
			;;

		--fetch)
			fetch=1
			;;

		--list)
			action="list"
			;;

		-h|--help)
			usage
			exit 0
			;;

		-*)
			echo "Unknown option '$1'" >&2
			usage
			exit 1
			;;

		*)
			suites="$suites $1"
			;;
	esac

	shift
done

export ZEPHYR_BASE
export NET_TOOLS_BASE
export TTCN3_DIR

find_dirs || exit 1

if [ -z "$suites" ]; then
	suites="$all_suites"
else
	suites="$(echo $suites)"
fi

if [ -z "$outdir" ]; then
	outdir="$ZEPHYR_BASE/twister-out"
fi

# pytest resolves a relative cache directory against its own rootdir, which is
# the directory the test lives in, so a relative output directory would put the
# cache inside the tree - owned by root, on a privileged run. The sudo re-exec
# below also passes this on from a different working directory.
case "$outdir" in
	/*) ;;
	*) outdir="$PWD/$outdir" ;;
esac

pytest_cache="$outdir/.pytest_cache"

case "$action" in
	list)
		list_suites
		exit 0
		;;

	stop)
		stop_all_interfaces
		exit 0
		;;
esac

check_suites || exit 1

# Everything that can be done as the invoking user is done before asking for a
# password: a missing Titan should not cost a sudo prompt first, and the module
# clones should not end up owned by root.
check_titan || exit 1
fetch_modules || exit 1

# Then re-run under sudo, so that a single Twister invocation covers every
# selected suite and there is only one interface lock.
if [ "$(id -u)" != 0 ]; then
	privileged=""

	for suite in $suites
	do
		if needs_root "$suite"; then
			privileged="$privileged $suite"
		fi
	done

	if [ -n "$privileged" ]; then
		echo "Need root for:$privileged"
		echo "Re-running under sudo..."

		exec sudo -E env "PATH=$PATH" "ZEPHYR_BASE=$ZEPHYR_BASE" \
			"NET_TOOLS_BASE=$NET_TOOLS_BASE" "TTCN3_DIR=$TTCN3_DIR" \
			"$0" -O "$outdir" \
			$([ $keep -eq 1 ] && echo --keep) \
			$([ $fetch -eq 1 ] && echo --fetch) \
			$([ "$action" = "start" ] && echo --start) \
			$suites
	fi
fi

start_interfaces || exit 1

if [ "$action" = "start" ]; then
	echo "Interfaces are up. Remove them with --stop."
	exit 0
fi

run_twister
result=$?

restore_ownership

if [ $keep -eq 0 ]; then
	stop_interfaces
else
	echo "Leaving the network interfaces up."
fi

exit $result

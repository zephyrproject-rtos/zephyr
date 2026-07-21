#!/bin/sh
# Write a fixed-size string message on the ivshmem shared memory
# Usage write_shared_memory.sh [-s size in bytes] [-l location] [-m message] [-v]
#
# Copyright (c) 2023 Huawei Technologies France SASU
#
# SPDX-License-Identifier: Apache-2.0

# default parameters
SHM_SIZE=4194304
SHM_LOC="/dev/shm/ivshmem"
SHM_MSG="This is a message test for ivshmem shared memory"
SHM_DD_VERBOSE="status=none"

usage()
{
	echo "Usage: $0 [-s size in bytes] [-l location] [-m message] [-v]"
	exit 1
}

write_message()
{
	WM_SIZE=$1
	WM_LOC=$2
	WM_MSG=$3
	WM_DD=$4

	WM_MSG_LEN=${#WM_MSG}

	if [ "$WM_MSG_LEN" -gt "$WM_SIZE" ]; then
		# make sure we only read and write up to WM_SIZE
		printf %s "$WM_MSG" | dd of="$WM_LOC" count=1 bs="$WM_SIZE" "$WM_DD" || exit 1
	else
		# pad to WM_SIZE
		printf %s "$WM_MSG" | dd of="$WM_LOC" ibs="$WM_SIZE" conv=sync "$WM_DD" || exit 1
	fi
}

while :
do
	# make sure we only read $1 if it is defined (no unset)
	PARAMS="${1:-0}"

	case "$PARAMS" in
	-s)
		shift
		SHM_SIZE="$1"
		;;
	-l)
		shift
		SHM_LOC="$1"
		;;
	-h)
		usage
		;;
	-m)
		shift
		SHM_MSG="$1"
		;;
	-v)
		SHM_DD_VERBOSE="status=progress"
		;;
	-*)
		usage
		;;
	*)
		break
		;;
	esac

	shift
done

write_message "$SHM_SIZE" "$SHM_LOC" "$SHM_MSG" "$SHM_DD_VERBOSE"

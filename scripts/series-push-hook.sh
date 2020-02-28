#!/bin/sh
#
# Copyright (c) 2019 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
#

remote=$1
url=$2
local_ref=$3
local_sha=$4
remote_ref=$5
remote_sha=$6
z40=0000000000000000000000000000000000000000

set -e exec

echo "Run push "

if [ "$local_sha" = $z40 ]
then
	# Handle delete
	:
else
	if [ "$remote_sha" = $z40 ]
	then
		# New branch, examine all commits since $remote/master
		base_commit=`git rev-parse $remote/master`
		range="$base_commit..$local_sha"
	else
		# Update to existing branch, examine new commits
		range="$remote_sha..$local_sha"
	fi

	echo "Perform check patch"
	${ZEPHYR_BASE}/scripts/checkpatch.pl --git $range
fi

#!/usr/bin/env sh
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 Christophe Dufaza <chris@openmarl.org>
#

# Check RFC for compliance with contribution guidelines.
#
# Refs:
# - https://docs.zephyrproject.org/latest/contribute/guidelines.html#running-ci-tests-locally
# - https://docs.zephyrproject.org/latest/contribute/guidelines.html#coding-style

print_usage() {
	echo 'chkrfc [-c|--commits COMMITS]'
	echo 'COMMITS: commit range, e.g. 30 for HEAD~30..HEAD, defaults to 1'
}

chkrfc() {
	arg_commits=$1
	cd "$ZEPHYR_BASE" || return

	scripts/ci/check_compliance.py -c "$arg_commits"
}

while test $# -gt 0; do
	key=$1
	case $key in
	-h)
		print_usage
		exit 0
		;;
	-c | --commits)
		arg_commits=$2
		shift
		shift
		;;
	*)
		echo "Unexpected argument: $key"
		exit 2
		;;
	esac
done

if [ -z "$arg_commits" ]; then
	chk_commits="HEAD~$arg_commits..HEAD"
else
	chk_commits="HEAD~1..HEAD"
fi

if [ -d "$ZEPHYR_BASE" ]; then
	chkrfc "$chk_commits"
else
	echo 'ZEPHYR_BASE is not set!'
	exit 2
fi

exit 0

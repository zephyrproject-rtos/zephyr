#!/bin/bash
# Copyright (c) 2020 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
set -eE

function cleanup()
{
	# Rename twister junit xml for use with junit-annotate-buildkite-plugin
	# create dummy file if twister did nothing
	if [ ! -f twister-out/twister.xml ]; then
	   touch twister-out/twister.xml
	fi
	mv twister-out/twister.xml twister-${BUILDKITE_JOB_ID}.xml
	buildkite-agent artifact upload twister-${BUILDKITE_JOB_ID}.xml


	# Upload test_file to get list of tests that are build/run
	if [ -f test_file.txt ]; then
		buildkite-agent artifact upload test_file.txt
	fi

	# ccache stats
	echo "--- ccache stats at finish"
	ccache -s

	# Cleanup on exit
	rm -fr *

	# disk usage
	echo "--- disk usage at finish"
	df -h
}

trap cleanup ERR

echo "--- run $0"

git log -n 5 --oneline --decorate --abbrev=12

# Setup module cache
cd /workdir
ln -s /var/lib/buildkite-agent/zephyr-module-cache/modules
ln -s /var/lib/buildkite-agent/zephyr-module-cache/tools
ln -s /var/lib/buildkite-agent/zephyr-module-cache/bootloader
cd /workdir/zephyr

export JOB_NUM=$((${BUILDKITE_PARALLEL_JOB}+1))

# ccache stats
echo ""
echo "--- ccache stats at start"
ccache -s


if [ -n "${DAILY_BUILD}" ]; then
   TWISTER_OPTIONS=" --inline-logs -M -N --build-only --all --retry-failed 3 -v "
   echo "--- DAILY BUILD"
   west init -l .
   west update 1> west.update.log || west update 1> west.update-2.log
   west forall -c 'git reset --hard HEAD'
   source zephyr-env.sh
   ./scripts/twister --subset ${JOB_NUM}/${BUILDKITE_PARALLEL_JOB_COUNT} ${TWISTER_OPTIONS}
else
   if [ -n "${BUILDKITE_PULL_REQUEST_BASE_BRANCH}" ]; then
      ./scripts/ci/run_ci.sh  -c -b ${BUILDKITE_PULL_REQUEST_BASE_BRANCH} -r origin \
	   -m ${JOB_NUM} -M ${BUILDKITE_PARALLEL_JOB_COUNT} -p ${BUILDKITE_PULL_REQUEST}
   else
       ./scripts/ci/run_ci.sh -c -b ${BUILDKITE_BRANCH} -r origin \
	   -m ${JOB_NUM} -M ${BUILDKITE_PARALLEL_JOB_COUNT};
   fi
fi

TWISTER_EXIT_STATUS=$?

cleanup

exit ${TWISTER_EXIT_STATUS}

#!/bin/bash
# Copyright (c) 2020 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0
set -eE

function cleanup()
{
	# Rename sanitycheck junit xml for use with junit-annotate-buildkite-plugin
	# create dummy file if sanitycheck did nothing
	if [ ! -f sanity-out/sanitycheck.xml ]; then
	   touch sanity-out/sanitycheck.xml
	fi
	mv sanity-out/sanitycheck.xml sanitycheck-${BUILDKITE_JOB_ID}.xml
	buildkite-agent artifact upload sanitycheck-${BUILDKITE_JOB_ID}.xml


	# Upload test_file to get list of tests that are build/run
	if [ -f test_file.txt ]; then
		buildkite-agent artifact upload test_file.txt
	fi

	# ccache stats
	echo "--- ccache stats at finish"
	ccache -s

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

# Temporary fix: Install lpc_checksum, needed to build images for
# lpcxpresso11u68 boards
pip3 install lpc_checksum

if [ -n "${DAILY_BUILD}" ]; then
   SANITYCHECK_OPTIONS=" --inline-logs -N --build-only --all --retry-failed 3 -v "
   echo "--- DAILY BUILD"
   west init -l .
   west update 1> west.update.log || west update 1> west.update-2.log
   west forall -c 'git reset --hard HEAD'
   source zephyr-env.sh
   ./scripts/sanitycheck --subset ${JOB_NUM}/${BUILDKITE_PARALLEL_JOB_COUNT} ${SANITYCHECK_OPTIONS}
else
   if [ -n "${BUILDKITE_PULL_REQUEST_BASE_BRANCH}" ]; then
      ./scripts/ci/run_ci.sh  -c -b ${BUILDKITE_PULL_REQUEST_BASE_BRANCH} -r origin \
	   -m ${JOB_NUM} -M ${BUILDKITE_PARALLEL_JOB_COUNT} -p ${BUILDKITE_PULL_REQUEST}
   else
       ./scripts/ci/run_ci.sh -c -b ${BUILDKITE_BRANCH} -r origin \
	   -m ${JOB_NUM} -M ${BUILDKITE_PARALLEL_JOB_COUNT};
   fi
fi

SANITY_EXIT_STATUS=$?

cleanup

exit ${SANITY_EXIT_STATUS}

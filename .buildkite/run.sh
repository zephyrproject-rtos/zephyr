#!/bin/bash
# Copyright (c) 2020 Linaro Limited
#
# SPDX-License-Identifier: Apache-2.0

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

if [ -n "${BUILDKITE_PULL_REQUEST_BASE_BRANCH}" ]; then
   ./scripts/ci/run_ci.sh  -c -b ${BUILDKITE_PULL_REQUEST_BASE_BRANCH} -r origin \
	   -m ${JOB_NUM} -M ${BUILDKITE_PARALLEL_JOB_COUNT} -p ${BUILDKITE_PULL_REQUEST}
else
   ./scripts/ci/run_ci.sh -c -b ${BUILDKITE_BRANCH} -r origin \
	   -m ${JOB_NUM} -M ${BUILDKITE_PARALLEL_JOB_COUNT};
fi;

SANITY_EXIT_STATUS=$?

# Rename sanitycheck junit xml for use with junit-annotate-buildkite-plugin
# create dummy file if sanitycheck did nothing
if [ ! -f sanity-out/sanitycheck.xml ]; then
   touch sanity-out/sanitycheck.xml
fi
mv sanity-out/sanitycheck.xml sanitycheck-${BUILDKITE_JOB_ID}.xml
buildkite-agent artifact upload sanitycheck-${BUILDKITE_JOB_ID}.xml

# ccache stats
echo "--- ccache stats at finish"
ccache -s

# disk usage
echo "--- disk usage at finish"
df -h

exit ${SANITY_EXIT_STATUS}

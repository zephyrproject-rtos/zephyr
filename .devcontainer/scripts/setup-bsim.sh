#!/usr/bin/env bash
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu

echo "Updating and rebuilding BSIM"

export BSIM_VERSION=$( west list bsim -f {revision} )
echo "Manifest points to bsim sha $BSIM_VERSION"

cd /opt/bsim_west/bsim

git fetch -n origin ${BSIM_VERSION}
git -c advice.detachedHead=false checkout ${BSIM_VERSION}

west update
make everything -s -j 8

echo "Updated and re-built BSIM"

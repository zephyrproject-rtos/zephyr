#!/usr/bin/env bash
# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set -eu

echo "Setting up environment"

export ZEPHYR_SDK_INSTALL_DIR="/opt/toolchains/zephyr-sdk-$( cat SDK_VERSION )"

# Skip if a workspace already exists
config_path="/workspaces/.west/config"
if [ -f "$config_path" ]; then
    echo "West .config exists, skipping init and update."
    exit 0
fi

# Can that have bad consequences if host UID != 1000?
sudo chown user:user /workspaces

# When checking out a single PR or branch, vscode will perform a shallow clone.
# Fetch the whole remote in order to be able to run check_compliance.py against
# origin/main (or any other branch in origin).
cd /workspaces/zephyr
git fetch origin
cd -

# Set up west workspace.
# If we are building a bsim devcontainer, we don't need the whole set, just a
# minimal configuration.
if [[ -z "${BSIM_OUT_PATH+x}" ]]; then
    west init -l --mf west.yml
else
    west init -l --mf .devcontainer/bsim/west-bsim.yml
fi
west config --global update.narrow true
west update

# Reset every project except the main zephyr repo.
#
# I can't "just" use $(pwd) or shell expansion as it will be expanded _before_
# the command is executed in every west project.
west forall -c 'pwd | xargs basename | xargs test "zephyr" != && git reset --hard HEAD' || true

echo "West initialized successfully"

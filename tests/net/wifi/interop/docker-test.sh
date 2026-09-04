#!/bin/sh
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
#
# Build and run the native_sim Wi-Fi interop test inside an all-in-one
# privileged container. mac80211_hwsim is a host kernel module, so this needs
# --privileged, --network=host, and the host's /lib/modules mounted so the
# in-container modprobe can find and load the module into the (shared) host
# kernel. The container only packages the userspace tooling. This is a
# convenience wrapper, not a substitute for the host-setup path in README.md.

set -eu

IMAGE=zephyr-wifi-interop
IFACE=zwifi

# Resolve the west workspace root (two levels above this zephyr checkout).
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ZEPHYR_BASE=$(CDPATH= cd -- "$SCRIPT_DIR/../../../.." && pwd)
WORKSPACE=$(CDPATH= cd -- "$ZEPHYR_BASE/.." && pwd)

docker build -t "$IMAGE" "$SCRIPT_DIR/docker"

docker run --rm -it \
    --privileged \
    --network=host \
    -v /lib/modules:/lib/modules:ro \
    -v "$WORKSPACE":"$WORKSPACE" \
    -w "$ZEPHYR_BASE" \
    "$IMAGE" \
    bash -lc "
        set -e
        # The build's toolchain-capability cache otherwise falls back to
        # \$ZEPHYR_BASE/.cache (the bind-mounted, host-owned, non-writable
        # source tree); point it at a container-writable location.
        export XDG_CACHE_HOME=/tmp/zephyr-cache
        mkdir -p /tmp/zephyr-cache
        # net-setup.sh references its hostapd/dnsmasq configs by relative path
        # (and modprobes mac80211_hwsim itself), so run it from net-tools.
        ( cd ../tools/net-tools && sudo ./net-setup.sh -c zwifi.conf -i $IFACE start )
        trap '( cd ../tools/net-tools && sudo ./net-setup.sh -c zwifi.conf -i $IFACE stop )' EXIT
        # The container runs as a non-root user, so the test setcaps the binary
        # (via the image's passwordless sudo) to give zephyr.exe net access.
        # Write twister output to /tmp: the bind-mounted workspace is owned by
        # the host user and is not writable by the container user.
        ./scripts/twister -p native_sim --enable-slow -i -c \
            -O /tmp/twister-wifi-interop \
            -T tests/net/wifi/interop
    "

# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0
#
# Starting point for an all-in-one container that can build and run the
# native_sim Wi-Fi interop test. It must be run with --privileged and
# --network=host (see docker-test.sh): mac80211_hwsim is a host kernel module,
# so the container cannot isolate it - it only packages the userspace tooling.

FROM ghcr.io/zephyrproject-rtos/zephyr-build:latest

USER root

# Runtime tooling for the simulated radio + APs, plus the 32-bit libnl the
# native_sim driver links against.
RUN dpkg --add-architecture i386 && \
    apt-get update && apt-get install -y --no-install-recommends \
        iw \
        hostapd \
        dnsmasq \
        iproute2 \
        kmod \
        libnl-3-dev:i386 \
        libnl-genl-3-dev:i386 \
        gcc-multilib \
    && rm -rf /var/lib/apt/lists/*

USER user

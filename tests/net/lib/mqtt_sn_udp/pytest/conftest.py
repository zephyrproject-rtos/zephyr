# Copyright (c) 2026 sevenlab engineering GmbH
# SPDX-License-Identifier: Apache-2.0

import ctypes
import logging
import os
import socket
import subprocess
import time
from pathlib import Path

import psutil
import pytest

testdir = Path(__file__).resolve().parent
logger = logging.getLogger(__name__)

libc = ctypes.cdll.LoadLibrary("libc.so.6")
PR_SET_PDEATHSIG = 1

ADDR_BRIDGE_V4 = "192.0.2.254"
ADDR_BRIDGE_V6 = "2001:db8::254"

ADDR_ZEPHYR_V4 = "192.0.2.1"
ADDR_ZEPHYR_V6 = "2001:db8::2"

ADDR_MULTICAST_V4 = "225.1.1.1"
ADDR_MULTICAST_V6 = "ff02::1"


def enter_netns():
    uid = os.getuid()
    gid = os.getgid()

    os.unshare(os.CLONE_NEWUSER | os.CLONE_NEWNET)
    with open("/proc/self/uid_map", "w") as f:
        f.write(f"0 {uid} 1")
    with open("/proc/self/setgroups", "w") as f:
        f.write("deny")
    with open("/proc/self/gid_map", "w") as f:
        f.write(f"0 {gid} 1")


def ip_capture(*args):
    args = [str(item) for item in args]
    return subprocess.check_output(["ip"] + args).decode("utf-8")


def ip(*args):
    args = [str(item) for item in args]
    return subprocess.run(["ip"] + args, check=True)


def pytest_sessionstart(session):
    enter_netns()

    # Add a bridge, where each service is added to
    ip("link", "add", "name", "br0", "type", "bridge")
    ip("address", "add", f"{ADDR_BRIDGE_V4}/24", "dev", "br0")
    ip("address", "add", f"{ADDR_BRIDGE_V6}/64", "dev", "br0")
    ip("link", "set", "dev", "br0", "up")
    ip("route", "add", "224.0.0.0/4", "dev", "br0")
    ip("route", "add", "ff00::/8", "dev", "br0")

    # Add Zephyr tap
    ip("tuntap", "add", "zeth", "mode", "tap")
    ip("link", "set", "dev", "zeth", "master", "br0")
    ip("link", "set", "dev", "zeth", "up")

    while True:
        output = ip_capture("-6", "addr", "show", "dev", "br0")
        if "tentative" not in output:
            break
        time.sleep(0.5)


class IpConfig:
    def __init__(self, config):
        self.supports_ipv4 = config.read_bool("CONFIG_NET_IPV4", False)
        self.supports_ipv6 = config.read_bool("CONFIG_NET_IPV6", False)

    def supports(self, version: int):
        if version == 4 and self.supports_ipv4:
            return True
        return version == 6 and self.supports_ipv6

    def skip_if_unsupported(self, version: int):
        if not self.supports(version):
            pytest.skip(f"Target doesn't support IP version {version}")

    def addr_bridge(self, version, brackets=False):
        if version == 4:
            return ADDR_BRIDGE_V4
        if version == 6:
            if brackets:
                return f"[{ADDR_BRIDGE_V6}]"
            else:
                return f"{ADDR_BRIDGE_V6}"
        pytest.fail("unimplemented")

    def addr_zephyr(self, version):
        if version == 4:
            return ADDR_ZEPHYR_V4
        if version == 6:
            return f"{ADDR_ZEPHYR_V6}"
        pytest.fail("unimplemented")

    def addr_multicast(self, version, brackets=False):
        if version == 4:
            return ADDR_MULTICAST_V4
        if version == 6:
            if brackets:
                return f"[{ADDR_MULTICAST_V6}]"
            else:
                return f"{ADDR_MULTICAST_V6}"
        pytest.fail("unimplemented")

    def inet(self, version):
        if version == 4:
            return socket.AF_INET
        if version == 6:
            return socket.AF_INET6
        pytest.fail("unimplemented")

    def addr_multicast_src(self, version):
        for addr in psutil.net_if_addrs()["br0"]:
            if addr.family == socket.AF_INET and version == 4:
                return addr.address
            elif addr.family == socket.AF_INET6 and version == 6:
                address = addr.address.split("%")[0]
                if not address.startswith("fe80:"):
                    continue
                return address


@pytest.fixture
def ipconfig(device_object, config_reader):
    config = config_reader(device_object.device_config.build_dir / "zephyr/.config")
    return IpConfig(config)

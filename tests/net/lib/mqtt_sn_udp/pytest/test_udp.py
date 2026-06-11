# Copyright (c) 2026 sevenlab engineering GmbH
# SPDX-License-Identifier: Apache-2.0

import base64
import logging
import socket
import struct
import time

import pytest
from twister_harness import Shell

logger = logging.getLogger(__name__)

IP_RECVTTL = 12
PORT = 1883


def shellexec(shell: Shell, command: str):
    return shell.get_filtered_output(shell.exec_command(command))


def ancdata_get_dst_addr(ancdata):
    for cmsg_level, cmsg_type, cmsg_data in ancdata:
        if cmsg_level == socket.IPPROTO_IP and cmsg_type == socket.IP_PKTINFO:
            _ifindex, spec_dst, addr = struct.unpack("@I4s4s", cmsg_data)
            spec_dst = socket.inet_ntoa(spec_dst)
            addr = socket.inet_ntoa(addr)

            return addr
        if cmsg_level == socket.IPPROTO_IPV6 and cmsg_type == socket.IPV6_PKTINFO:
            return socket.inet_ntop(socket.AF_INET6, cmsg_data[:16])

    return None


def ancdata_get_ttl(ancdata):
    for cmsg_level, cmsg_type, cmsg_data in ancdata:
        if cmsg_level == socket.IPPROTO_IP and cmsg_type == socket.IP_TTL:
            (ttl,) = struct.unpack("@i", cmsg_data)
            return ttl
        if cmsg_level == socket.IPPROTO_IPV6 and cmsg_type == socket.IPV6_HOPLIMIT:
            (ttl,) = struct.unpack("@i", cmsg_data)
            return ttl

    return None


@pytest.mark.parametrize("ipversion", [4, 6])
def test_recvfrom(shell: Shell, ipconfig, ipversion):
    ipconfig.skip_if_unsupported(ipversion)
    inet = ipconfig.inet(ipversion)
    addr_multicast = ipconfig.addr_multicast(ipversion)
    addr_multicast_brackets = ipconfig.addr_multicast(ipversion, brackets=True)
    addr_multicast_src = ipconfig.addr_multicast_src(ipversion)

    message = b"Hello World"

    assert shellexec(shell, f"mqttsn udp_create {addr_multicast_brackets}:{PORT}") == ["success"]
    assert shellexec(shell, "mqttsn udp_init") == ["success"]
    assert shellexec(shell, "mqttsn udp_poll") == ["0"]

    s = socket.socket(inet, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    s.sendto(message, (addr_multicast, PORT))
    srcport = s.getsockname()[1]

    time.sleep(1)
    assert shellexec(shell, "mqttsn udp_poll") == ["1"]

    rec = shellexec(shell, "mqttsn udp_recvfrom")
    assert len(rec) == 2
    assert rec[0] == f"{addr_multicast_src}:{srcport}"
    assert base64.b64decode(rec[1], validate=True) == message

    assert shellexec(shell, "mqttsn udp_poll") == ["0"]


@pytest.mark.parametrize("ipversion", [4, 6])
def test_sendto_unicast(shell: Shell, ipconfig, ipversion):
    ipconfig.skip_if_unsupported(ipversion)
    inet = ipconfig.inet(ipversion)
    addr_multicast_brackets = ipconfig.addr_multicast(ipversion, brackets=True)
    addr_bridge = ipconfig.addr_bridge(ipversion)
    addr_bridge_brackets = ipconfig.addr_bridge(ipversion, brackets=True)
    addr_zephyr = ipconfig.addr_zephyr(ipversion)

    message = b"Hello Server"

    assert shellexec(shell, f"mqttsn udp_create {addr_multicast_brackets}:{PORT}") == ["success"]
    assert shellexec(shell, "mqttsn udp_init") == ["success"]

    s = socket.socket(inet, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    if ipversion == 4:
        s.setsockopt(socket.IPPROTO_IP, socket.IP_PKTINFO, 1)
        s.bind(("0.0.0.0", PORT))
    elif ipversion == 6:
        s.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_RECVPKTINFO, 1)
        s.bind(("::", PORT))

    message_b64 = base64.b64encode(message).decode("utf-8")
    assert shellexec(shell, f"mqttsn udp_sendto {message_b64} {addr_bridge_brackets}:{PORT}") == [
        "success"
    ]

    recv_data, recv_ancdata, _recv_msgflags, recv_srcaddr = s.recvmsg(4096, 4096)
    assert recv_data == message
    assert recv_srcaddr[0] == addr_zephyr
    # The UDP transport shall send this from it's multicast listener socket,
    # that's why the port is 1883.
    assert recv_srcaddr[1] == PORT
    assert ancdata_get_dst_addr(recv_ancdata) == addr_bridge


@pytest.mark.parametrize("ipversion", [4, 6])
def test_sendto_multicast(shell: Shell, ipconfig, ipversion):
    ipconfig.skip_if_unsupported(ipversion)
    inet = ipconfig.inet(ipversion)
    addr_multicast = ipconfig.addr_multicast(ipversion)
    addr_multicast_brackets = ipconfig.addr_multicast(ipversion, brackets=True)
    addr_zephyr = ipconfig.addr_zephyr(ipversion)

    message = b"Hello World"

    assert shellexec(shell, f"mqttsn udp_create {addr_multicast_brackets}:{PORT}") == ["success"]
    assert shellexec(shell, "mqttsn udp_init") == ["success"]

    s = socket.socket(inet, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    if ipversion == 4:
        s.setsockopt(socket.IPPROTO_IP, socket.IP_PKTINFO, 1)
        s.setsockopt(socket.IPPROTO_IP, IP_RECVTTL, 1)
        s.bind(("0.0.0.0", PORT))

        mreqn = struct.pack("@4sLi", socket.inet_aton(addr_multicast), socket.INADDR_ANY, 0)
        s.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreqn)
    elif ipversion == 6:
        s.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_RECVPKTINFO, 1)
        s.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_RECVHOPLIMIT, 1)
        s.bind(("::", PORT))

        mreq = socket.inet_pton(inet, addr_multicast) + struct.pack("@i", 0)
        s.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_JOIN_GROUP, mreq)

    message_b64 = base64.b64encode(message).decode("utf-8")
    assert shellexec(shell, f"mqttsn udp_sendto_bcast {message_b64} 42") == ["success"]

    recv_data, recv_ancdata, _recv_msgflags, recv_srcaddr = s.recvmsg(4096, 4096)
    assert recv_data == message
    if ipversion == 4:
        assert recv_srcaddr[0] == addr_zephyr
    elif ipversion == 6:
        assert recv_srcaddr[0].startswith("fe80:")
    assert recv_srcaddr[1] == PORT
    assert ancdata_get_dst_addr(recv_ancdata) == addr_multicast
    assert ancdata_get_ttl(recv_ancdata) == 42

    # Send another one to make sure it's not hardcoded
    assert shellexec(shell, f"mqttsn udp_sendto_bcast {message_b64} 1") == ["success"]

    recv_data, recv_ancdata, _recv_msgflags, recv_srcaddr = s.recvmsg(4096, 4096)
    assert recv_data == message
    assert recv_data == message
    if ipversion == 4:
        assert recv_srcaddr[0] == addr_zephyr
    elif ipversion == 6:
        assert recv_srcaddr[0].startswith("fe80:")
    assert recv_srcaddr[1] == PORT
    assert ancdata_get_dst_addr(recv_ancdata) == addr_multicast
    assert ancdata_get_ttl(recv_ancdata) == 1


@pytest.mark.parametrize("ipversion", [4, 6])
def test_deinit(shell: Shell, ipconfig, ipversion):
    ipconfig.skip_if_unsupported(ipversion)
    addr_multicast_brackets = ipconfig.addr_multicast(ipversion, brackets=True)

    assert shellexec(shell, f"mqttsn udp_create {addr_multicast_brackets}:{PORT}") == ["success"]
    assert shellexec(shell, "mqttsn udp_init") == ["success"]
    assert shellexec(shell, "mqttsn udp_deinit") == ["success"]
    assert shellexec(shell, "mqttsn udp_init") == ["success"]
    assert shellexec(shell, "mqttsn udp_deinit") == ["success"]

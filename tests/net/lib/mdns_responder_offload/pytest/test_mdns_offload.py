# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""
Verify that the mDNS responder works over offloaded sockets (NSOS).

The Zephyr application runs on native_sim with CONFIG_NET_NATIVE_OFFLOADED_SOCKETS,
so its mDNS responder binds host sockets and joins the mDNS multicast group on the
host. This test browses for the DNS-SD service published by the responder and checks,
per address family, that the resolved A/AAAA records match routable addresses that
actually belong to the host (which the offloaded interface mirrors onto the Zephyr
net_if).

Whether a missing host family is a skip or a failure is controlled from tests.yaml
via the --require-ipv4 / --require-ipv6 pytest options (see conftest.py).
"""

import ipaddress
import logging
import threading

import psutil
import pytest
from twister_harness import DeviceAdapter
from zeroconf import IPVersion, ServiceBrowser, ServiceStateChange, Zeroconf

logger = logging.getLogger(__name__)

SERVICE_TYPE = "_zephyr._tcp.local."


def _routable(ip: ipaddress._BaseAddress) -> bool:
    # Loopback and link-local addresses are not mirrored/advertised by the
    # responder and cannot be matched reliably across host and querier.
    return not (ip.is_loopback or ip.is_link_local or ip.is_unspecified)


def _host_addresses(version: int) -> set[str]:
    """Routable IPv4 (version 4) or IPv6 (version 6) addresses of this host."""
    addresses: set[str] = set()
    for iface_addresses in psutil.net_if_addrs().values():
        for addr in iface_addresses:
            # Drop the IPv6 zone id (e.g. "fe80::1%eth0") before parsing.
            try:
                ip = ipaddress.ip_address(addr.address.split("%")[0])
            except ValueError:
                continue
            if ip.version == version and _routable(ip):
                addresses.add(str(ip))
    return addresses


@pytest.fixture(scope="module")
def resolved_addresses(unlaunched_dut: DeviceAdapter) -> set[str]:
    """Launch the DUT once and browse for the responder's advertised addresses."""
    dut = unlaunched_dut
    dut.launch()
    dut.readlines_until(regex="mDNS offload responder ready", timeout=10.0)

    # Only wait for the families the host can actually match against, so a
    # single-stack host does not block until timeout waiting for the other one.
    wanted_versions = {version for version in (4, 6) if _host_addresses(version)}
    found = threading.Event()
    resolved: set[str] = set()

    def on_state_change(zeroconf, service_type, name, state_change):
        # The responder answers the browse (PTR) query with the service records
        # plus its A/AAAA addresses as additional records.
        if state_change is not ServiceStateChange.Added:
            return
        info = zeroconf.get_service_info(service_type, name)
        if info is None:
            return
        addrs = {
            str(ip)
            for ip in map(ipaddress.ip_address, info.parsed_addresses())
            if _routable(ip)
        }
        if not addrs:
            return
        resolved.update(addrs)
        # Tear down only once every host-matchable family has been advertised, so
        # an A record arriving before the AAAA does not end the browse early.
        if {ipaddress.ip_address(a).version for a in resolved} >= wanted_versions:
            found.set()

    zeroconf = Zeroconf(ip_version=IPVersion.All)
    browser = ServiceBrowser(zeroconf, SERVICE_TYPE, handlers=[on_state_change])
    try:
        found.wait(timeout=10.0)
    finally:
        browser.cancel()
        zeroconf.close()

    assert resolved, "mDNS responder was not discovered over offloaded sockets"
    logger.info("Responder advertised %s", sorted(resolved))
    return resolved


@pytest.mark.parametrize("version", [4, 6], ids=["ipv4", "ipv6"])
def test_mdns_offload(resolved_addresses, request, version):
    host_addresses = _host_addresses(version)
    if not host_addresses:
        message = f"host has no routable IPv{version} address"
        if request.config.getoption(f"--require-ipv{version}"):
            pytest.fail(message)
        pytest.skip(message)

    resolved = {a for a in resolved_addresses if ipaddress.ip_address(a).version == version}
    assert resolved & host_addresses, (
        f"responder returned no IPv{version} host address; "
        f"resolved={sorted(resolved_addresses)}, host={sorted(host_addresses)}"
    )

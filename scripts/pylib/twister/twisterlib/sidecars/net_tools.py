# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""net-tools sidecar: bring up the host network environment a QEMU net test needs."""

from __future__ import annotations

import hashlib
import logging
import os
import shlex
import shutil
import subprocess
from dataclasses import dataclass, field

from twisterlib.environment import ZEPHYR_BASE
from twisterlib.handlers import terminate_process
from twisterlib.sidecars.base import Sidecar
from twisterlib.statuses import TwisterStatus
from twisterlib.testinstance import TestInstance

logger = logging.getLogger('twister')


def get_net_iface_name(build_dir: str) -> str:
    """Return a unique host tap interface name for a build directory.

    Linux interface names are limited to 15 characters (IFNAMSIZ - 1), so use a
    short hash of ``build_dir`` after the ``zeth`` prefix. Both the sidecar (which
    creates the tap) and its :meth:`~NetToolsSidecar.cmake_args` (which bakes
    CONFIG_ETH_QEMU_IFACE_NAME into the build so QEMU attaches to it) derive this
    independently from ``build_dir``.
    """
    digest = hashlib.sha1(os.path.abspath(build_dir).encode()).hexdigest()[:8]
    return f"zeth{digest}"


def get_net_addresses(build_dir: str) -> tuple[str, str, int]:
    """Return ``(host_ipv4, guest_ipv4, prefix_len)`` unique per build directory.

    Each instance gets its own /24 out of the 10.0.0.0/8 private range so that
    parallel net tests do not share a subnet on the host (which would make the
    host's reply routing ambiguous). The host is ``.2`` and the guest ``.1``.
    """
    digest = hashlib.sha1(os.path.abspath(build_dir).encode()).digest()
    octet_a, octet_b = digest[0], digest[1]
    return f"10.{octet_a}.{octet_b}.2", f"10.{octet_a}.{octet_b}.1", 24


# The fixed addresses net-tools uses by default (zeth.conf): host ``.2``,
# guest ``.1``. Used by tests whose companion hard-codes these; see the
# ``shared_subnet`` config key.
NET_TOOLS_SHARED_SUBNET = ("192.0.2.2", "192.0.2.1", 24)


class NetToolsSidecar(Sidecar):
    """Bring up the host network environment a QEMU net test needs.

    Zephyr's net-tools ``net-setup.sh`` creates a ``tap`` interface that a guest
    built with ``CONFIG_NET_QEMU_ETHERNET`` attaches its NIC to (``-netdev tap,
    ifname=<iface>,script=no``). QEMU cannot open the tap unless it already
    exists, so the interface must be up before the guest boots and torn down
    afterwards - exactly a sidecar's :meth:`setup` / :meth:`teardown`.

    To run in parallel, each instance gets a unique interface name and its own
    /24 (see :func:`get_net_iface_name` / :func:`get_net_addresses`); the sidecar
    writes a matching net-setup.sh config for the host side and
    :meth:`cmake_args` bakes ``CONFIG_ETH_QEMU_IFACE_NAME`` and the guest/peer
    IPv4 addresses into the build so both ends agree. A test that manages its own
    addressing can override with ``iface`` / ``config_file`` and the sidecar then
    leaves the IPs alone.

    Creating a tap needs root; the sidecar runs ``net-setup.sh`` through
    ``sudo -n`` when not already root and skips the test (rather than hanging on
    a password prompt) when that is not available, or when net-tools is not
    found.

    Companion host apps (a net-tools echo-server, dnsmasq, ...) can be started
    alongside the interface with the ``apps`` key so that functional tests have
    something to talk to. Each app is a command run after the interface is up and
    stopped before it is torn down; the tokens ``{net_tools}``, ``{iface}``,
    ``{host_ip}`` and ``{guest_ip}`` are substituted, and bare known names expand
    to a default command (see :data:`KNOWN_APPS`). Apps bind to this instance's
    interface, so they stay isolated across parallel runs.

    Configured through the ``net-tools`` block of ``sidecar_config``:

    - ``iface``: interface name to create (default: derived per build dir).
    - ``config_file``: explicit net-setup.sh config file; disables the auto subnet.
    - ``setup_script``: explicit path to ``net-setup.sh``.
    - ``apps``: list of companion host apps/commands to run.
    - ``shared_subnet``: use the fixed 192.0.2.0/24 addresses instead of a
      private per-instance subnet (for companions that hard-code them).
    """

    NAME = 'net-tools'

    @dataclass
    class Config:
        iface: str | None = None
        config_file: str | None = None
        setup_script: str | None = None
        apps: list[str] = field(default_factory=list)
        shared_subnet: bool = False

    # net-tools is a sibling of the zephyr repo in a west workspace.
    NET_TOOLS_FALLBACK_PATHS = (
        os.path.join(ZEPHYR_BASE, '..', 'tools', 'net-tools'),
        os.path.join(ZEPHYR_BASE, '..', 'net-tools'),
    )

    # Shortcuts: a bare app name expands to this command template. Placeholders:
    # {net_tools} (net-tools dir), {iface}, {host_ip}, {guest_ip}.
    KNOWN_APPS = {
        # Passive server for guest echo clients; binds to the iface (parallel-safe).
        'echo-server': '{net_tools}/echo-server -i {iface}',
        # Active client that drives a guest echo server; -e keeps retrying so it
        # tolerates the guest still booting, -i binds it to this instance's iface.
        'echo-client': '{net_tools}/echo-client -i {iface} -e {guest_ip}',
        # Stdlib HTTP server on port 8000 for guest HTTP clients. It binds all
        # interfaces on a fixed port, so only one may run at a time.
        'http-server': 'python3 {net_tools}/http-server.py',
        # Echo websocket server on port 9001. It hard-codes the 192.0.2.2
        # address, so pair it with shared_subnet.
        'websocket-server': 'python3 {net_tools}/zephyr-websocket-server.py',
    }

    @classmethod
    def find_net_setup(cls):
        base = os.environ.get('NET_TOOLS_BASE')
        candidates = [os.path.join(base, 'net-setup.sh')] if base else []
        candidates += [os.path.join(p, 'net-setup.sh') for p in cls.NET_TOOLS_FALLBACK_PATHS]
        for candidate in candidates:
            if os.path.exists(candidate):
                return os.path.abspath(candidate)
        return shutil.which('net-setup.sh')

    def configure(self, instance: TestInstance):
        super().configure(instance)
        cfg = self.config
        self.net_setup = cfg.setup_script or self.find_net_setup()
        self.iface = cfg.iface or get_net_iface_name(instance.build_dir)
        if cfg.shared_subnet:
            # Some companions (e.g. the net-tools websocket server) hard-code the
            # 192.0.2.0/24 addresses. Use them instead of a private per-instance
            # subnet; the interface is still unique, but the shared addresses
            # mean only one such test may run at a time.
            self.host_ip, self.guest_ip, self.prefix = NET_TOOLS_SHARED_SUBNET
        else:
            self.host_ip, self.guest_ip, self.prefix = get_net_addresses(instance.build_dir)
        # An explicit config disables the auto per-instance subnet.
        self.net_config = cfg.config_file
        self.net_tools_apps = cfg.apps
        self._started = False
        self._apps = []

    def cmake_args(self, build_dir: str) -> list[str]:
        # Bake the interface name so QEMU attaches its NIC to this instance's tap
        # and, unless the test manages its own addressing, the guest/peer IPv4
        # addresses so both ends agree and parallel runs do not collide.
        args = [f'-DCONFIG_ETH_QEMU_IFACE_NAME="{self.iface}"']
        if not self.net_config and not self.config.shared_subnet:
            args.append(f'-DCONFIG_NET_CONFIG_MY_IPV4_ADDR="{self.guest_ip}"')
            args.append(f'-DCONFIG_NET_CONFIG_PEER_IPV4_ADDR="{self.host_ip}"')
        return args

    def _net_setup_cmd(self, action):
        cmd = []
        if os.geteuid() != 0:
            # -n so a missing passwordless sudo fails fast instead of prompting.
            cmd += ['sudo', '-n']
        cmd += [self.net_setup, '--iface', self.iface]
        if self.net_config:
            cmd += ['--config', self.net_config]
        cmd.append(action)
        return cmd

    def setup(self) -> bool:
        if not self.net_setup:
            self._skip("net-tools net-setup.sh not found "
                       "(set NET_TOOLS_BASE or install net-tools)")
            return False

        if os.geteuid() != 0 and subprocess.run(
            ['sudo', '-n', 'true'], capture_output=True
        ).returncode != 0:
            self._skip("creating a tap interface needs root or passwordless sudo")
            return False

        # Without an explicit config, write a per-instance one that puts the host
        # on this instance's own subnet (matching the guest IPs cmake_args bakes
        # in), so parallel tests do not share addresses.
        if not self.net_config:
            self.net_config = os.path.join(self.instance.build_dir, 'net-tools.conf')
            with open(self.net_config, 'w') as f:
                # Adding the address installs the connected /prefix route.
                # arp_accept lets a passive guest server become reachable: it
                # announces its address via gratuitous ARP (RFC 5227) on boot,
                # and this makes the host cache that MAC without needing the
                # guest to answer a broadcast ARP request.
                f.write(
                    'INTERFACE="$1"\n'
                    'ip link set dev "$INTERFACE" up\n'
                    f'ip address add {self.host_ip}/{self.prefix} dev "$INTERFACE"\n'
                    'sysctl -q -w "net.ipv4.conf.$INTERFACE.arp_accept=1"\n'
                )

        # Remove a stale interface left by a previous, crashed run.
        subprocess.run(self._net_setup_cmd('stop'), capture_output=True)

        result = subprocess.run(self._net_setup_cmd('start'), capture_output=True, text=True)
        if result.returncode != 0:
            self.instance.status = TwisterStatus.ERROR
            self.instance.reason = f"net-setup.sh failed: {result.stderr.strip()}"
            self.instance.add_missing_case_status(TwisterStatus.ERROR, self.instance.reason)
            logger.error(f"SIDECAR:{self.__class__.__name__}: {self.instance.reason}")
            return False

        self._started = True
        logger.debug(f"SIDECAR:{self.__class__.__name__}: brought up {self.iface}")

        return self._start_apps()

    def _start_apps(self) -> bool:
        net_tools = os.path.dirname(self.net_setup)
        for spec in self.net_tools_apps:
            command = self.KNOWN_APPS.get(spec, spec).format(
                net_tools=net_tools, iface=self.iface,
                host_ip=self.host_ip, guest_ip=self.guest_ip,
            )
            argv = shlex.split(command)
            binary = argv[0] if os.path.sep in argv[0] else shutil.which(argv[0])
            if not binary or not os.path.exists(binary):
                self._skip(f"companion app not found: {argv[0]} (build net-tools?)")
                return False

            # For interpreted apps (e.g. "python3 .../http-server.py") the binary
            # is the interpreter, so also confirm any net-tools file it runs exists.
            missing = next((tok for tok in argv[1:]
                            if net_tools in tok and not os.path.exists(tok)), None)
            if missing:
                self._skip(f"companion app not found: {missing} (build net-tools?)")
                return False

            name = os.path.basename(argv[0])
            log = open(os.path.join(self.instance.build_dir,  # noqa: SIM115
                                    f"net-tools-{name}.log"), 'w')
            # Own session so teardown can signal the whole process group.
            proc = subprocess.Popen(argv, stdout=log, stderr=subprocess.STDOUT,
                                    start_new_session=True)
            self._apps.append((proc, log))
            logger.debug(f"SIDECAR:{self.__class__.__name__}: started {name}"
                         f" (pid {proc.pid})")
        return True

    def teardown(self) -> None:
        for proc, log in self._apps:
            terminate_process(proc)
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
            log.close()
        self._apps = []

        if self._started:
            subprocess.run(self._net_setup_cmd('stop'), capture_output=True)
            self._started = False

    def _skip(self, reason):
        self.instance.status = TwisterStatus.SKIP
        self.instance.reason = reason
        self.instance.add_missing_case_status(TwisterStatus.SKIP, reason)
        logger.warning(f"SIDECAR:{self.__class__.__name__}: {reason},"
                       f" skipping {self.instance.name}")

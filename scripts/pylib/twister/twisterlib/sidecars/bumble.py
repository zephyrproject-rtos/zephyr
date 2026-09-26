# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Bumble sidecar: virtual Bluetooth controllers for native_sim tests."""

from __future__ import annotations

import logging
import os
import shlex
import socket
import subprocess
import time
from dataclasses import dataclass, field

from twisterlib.constants import ZEPHYR_BASE
from twisterlib.handlers import terminate_process
from twisterlib.sidecars.base import Sidecar
from twisterlib.statuses import TwisterStatus
from twisterlib.testinstance import TestInstance

logger = logging.getLogger('twister')


def allocate_tcp_port() -> int:
    """Bind port 0 to let the kernel pick a free TCP port, and return it.

    The socket is closed before the port is used, so another process could in
    principle grab it in between; in practice the kernel does not immediately
    reuse ports it just handed out, and each caller gets a distinct port even
    across parallel twister workers.
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(('127.0.0.1', 0))
        return sock.getsockname()[1]


class BumbleSidecar(Sidecar):
    """Runs Bumble virtual Bluetooth controllers for native_sim tests.

    Zephyr's Bluetooth host stack runs on native_sim and reaches a controller
    through the HCI user-channel driver, which connects to a TCP HCI server
    with ``--bt-dev=<ip:port>``. This sidecar starts one or more *linked*
    Bumble virtual controllers (``controllers.py`` from the Bluetooth Classic
    simulation framework), each listening on a per-instance TCP port with its
    own Bluetooth device address, so a Zephyr guest and the additional Zephyr
    peers the sidecar launches can discover and connect to each other over a
    simulated radio without any hardware.

    Configured through the ``bumble`` block of ``sidecar_config``:

    - ``addresses``: Bluetooth device address per controller (index 0..N-1).
      Defaults to two sequential addresses.
    - ``devices``: argument string per Zephyr instance sharing the simulated
      bus. Device 0 is this guest, run by the normal handler with the
      arguments injected; devices 1.. are peers the sidecar launches from the
      same executable. ``{addrN}`` and ``{ctrlN}`` placeholders expand to
      controller N's address and ``ip:port``.
    - ``controllers_script``: path to ``controllers.py``, relative to
      ``ZEPHYR_BASE``. Defaults to the copy in the Bluetooth Classic
      simulation framework.

    The test keeps its normal harness (typically ``ztest``): the harness
    consumes the guest's console output while this sidecar provisions the
    controllers and peers around it. A peer that does not exit cleanly fails
    the test in :meth:`teardown` even if the guest side passed, since both
    sides must complete the exchange.
    """

    NAME = 'bumble'

    DEFAULT_ADDRESSES = ('00:00:01:00:00:01', '00:00:01:00:00:02')
    DEFAULT_CONTROLLERS_SCRIPT = os.path.join(
        'tests', 'bluetooth', 'classic', 'bumble', 'common', 'controllers.py'
    )

    @dataclass
    class Config:
        addresses: list[str] = field(default_factory=list)
        devices: list[str] = field(default_factory=list)
        controllers_script: str | None = None

    def configure(self, instance: TestInstance):
        super().configure(instance)
        self.addresses = list(self.config.addresses or self.DEFAULT_ADDRESSES)
        self.devices = list(self.config.devices or [''])
        script = self.config.controllers_script or self.DEFAULT_CONTROLLERS_SCRIPT
        self.script = os.path.join(ZEPHYR_BASE, script)
        self.ports: list[int] = []
        self._proc = None
        self._log = None
        self._peers: list[tuple] = []

    def _resolve(self, spec: str) -> str:
        subs = {}
        for i, addr in enumerate(self.addresses):
            subs[f'addr{i}'] = addr
            subs[f'ctrl{i}'] = f'127.0.0.1:{self.ports[i]}'
        return spec.format(**subs)

    def _skip(self, reason: str) -> None:
        self.instance.status = TwisterStatus.SKIP
        self.instance.reason = reason
        self.instance.add_missing_case_status(TwisterStatus.SKIP, reason)
        logger.warning(
            f"SIDECAR:{self.__class__.__name__}: {reason}, skipping {self.instance.name}"
        )

    def setup(self) -> bool:
        if not os.path.exists(self.script):
            self._skip(
                "Bumble controllers.py not found (Bluetooth Classic simulation framework present?)"
            )
            return False

        # Allocate a TCP port per controller and start them, linked, in one
        # process. Each controller is an HCI TCP server the guest connects to;
        # controllers.py takes "<bumble-transport>@<bd_address>" per
        # controller.
        self.ports = [allocate_tcp_port() for _ in self.addresses]
        transport_args = [
            f'tcp-server:_:{port}@{addr}'
            for port, addr in zip(self.ports, self.addresses, strict=True)
        ]
        log_path = os.path.join(self.instance.build_dir, 'bumble-controllers.log')
        # The controllers outlive setup(); the handle is closed in teardown().
        self._log = open(log_path, 'w')  # noqa: SIM115
        self._proc = subprocess.Popen(
            ['python3', self.script, *transport_args],
            cwd=os.path.dirname(self.script),
            stdout=self._log,
            stderr=subprocess.STDOUT,
            start_new_session=True,
        )

        if not self._wait_for_ports():
            self._skip("Bumble controllers did not start (Bumble installed?)")
            return False

        exe = os.path.join(self.instance.build_dir, 'zephyr', 'zephyr.exe')

        # Device 0 runs under the harness: inject its --bt-dev and arguments
        # so the handler launches it against controller 0.
        main_args = shlex.split(self._resolve(self.devices[0])) if self.devices[0] else []
        existing = list(self.instance.handler.extra_test_args or [])
        self.instance.handler.extra_test_args = existing + [
            f'--bt-dev=127.0.0.1:{self.ports[0]}',
            *main_args,
        ]

        # Devices 1.. are peers the sidecar launches on their own controllers.
        for i, spec in enumerate(self.devices[1:], start=1):
            args = shlex.split(self._resolve(spec))
            command = [exe, f'--bt-dev=127.0.0.1:{self.ports[i]}', *args]
            log_path = os.path.join(self.instance.build_dir, f'bumble-peer{i}.log')
            log = open(log_path, 'w')  # noqa: SIM115
            proc = subprocess.Popen(
                command, stdout=log, stderr=subprocess.STDOUT, start_new_session=True
            )
            self._peers.append((proc, log))
            logger.debug(f"SIDECAR:{self.__class__.__name__}: started peer {i} (pid {proc.pid})")
        return True

    def _wait_for_ports(self) -> bool:
        for _ in range(50):
            if all(self._port_open(port) for port in self.ports):
                return True
            if self._proc.poll() is not None:
                return False
            time.sleep(0.1)
        return False

    @staticmethod
    def _port_open(port: int) -> bool:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.settimeout(0.2)
            return sock.connect_ex(('127.0.0.1', port)) == 0

    def teardown(self) -> None:
        # Reap the peer executables; a peer failing fails the test even if
        # this guest passed, since both sides must complete the exchange.
        for i, (proc, log) in enumerate(self._peers, start=1):
            try:
                ret = proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                proc.kill()
                ret = proc.wait()
            log.close()
            if ret != 0 and self.instance.status == TwisterStatus.PASS:
                self.instance.status = TwisterStatus.FAIL
                self.instance.reason = f"Bumble peer {i} exited with {ret}"
                logger.error(f"SIDECAR:{self.__class__.__name__}: {self.instance.reason}")
        self._peers = []

        if self._proc is not None:
            terminate_process(self._proc)
            try:
                self._proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self._proc.kill()
            self._proc = None
        if self._log is not None:
            self._log.close()
            self._log = None

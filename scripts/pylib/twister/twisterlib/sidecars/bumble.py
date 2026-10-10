# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Bumble sidecar: virtual Bluetooth controllers for native_sim tests."""

from __future__ import annotations

import importlib.util
import logging
import os
import select
import shlex
import subprocess
import sys
import time
from dataclasses import dataclass, field
from typing import IO, NamedTuple

from domains import Domains
from twisterlib.constants import ZEPHYR_BASE
from twisterlib.handlers import terminate_process
from twisterlib.harness import Harness, Test
from twisterlib.sidecars.base import Sidecar
from twisterlib.statuses import TwisterStatus
from twisterlib.testinstance import TestInstance

logger = logging.getLogger('twister')


class Peer(NamedTuple):
    """A device the sidecar runs itself, next to the guest."""

    proc: subprocess.Popen
    log: IO
    log_path: str


class BumbleSidecar(Sidecar):
    """Runs Bumble virtual Bluetooth controllers for native_sim tests.

    Zephyr's Bluetooth host stack runs on native_sim and reaches a controller
    through the HCI user-channel driver, which connects to a TCP HCI server
    with ``--bt-dev=<ip:port>``. This sidecar starts one or more *linked*
    Bumble virtual controllers (``controllers.py`` from the Bluetooth Classic
    simulation framework), each listening on a kernel-chosen loopback TCP
    port with its own Bluetooth device address, so a Zephyr guest and the
    additional Zephyr peers the sidecar launches can discover and connect to
    each other over a simulated radio without any hardware.

    Configured through the ``bumble`` block of ``sidecar_config``:

    - ``addresses``: Bluetooth device address per controller (index 0..N-1).
      Defaults to two sequential addresses.
    - ``devices``: argument string per Zephyr instance sharing the simulated
      bus. Device 0 is this guest, run by the normal handler with the
      arguments injected; devices 1.. are peers the sidecar launches.
      ``{addrN}`` and ``{ctrlN}`` placeholders expand to controller N's
      address and ``ip:port``. A peer runs the same executable as the guest,
      unless it is written as a mapping of ``args`` and ``image``, where the
      image names one of the scenario's ``required_applications``.
    - ``controller_option``: option that gives device N the endpoint of
      controller N. Defaults to ``--bt-dev``, the one of the HCI user-channel
      driver. An empty string adds nothing, for a test that passes
      ``{ctrlN}`` in its arguments itself.
    - ``controllers_script``: path to ``controllers.py``, relative to
      ``ZEPHYR_BASE``. Defaults to the copy in the Bluetooth Classic
      simulation framework.

    The test keeps its normal harness (typically ``ztest``): the harness
    consumes the guest's console output while this sidecar provisions the
    controllers and peers around it. The handler watches the guest only, so
    in :meth:`teardown` the sidecar reads the Ztest results each peer printed
    and reports the cases it ran. A peer that fails, ends without a verdict
    or does not finish fails the test even if the guest side passed, since
    both sides must complete the exchange.

    Bumble must be importable by the interpreter running twister, which also
    runs ``controllers.py``; without it the test is built but not run.
    """

    NAME = 'bumble'

    DEFAULT_ADDRESSES = ('00:00:01:00:00:01', '00:00:01:00:00:02')
    DEFAULT_CONTROLLERS_SCRIPT = os.path.join(
        'tests', 'bluetooth', 'classic', 'bumble', 'common', 'controllers.py'
    )
    #: Seconds the controllers get to come up and report their ports.
    START_TIMEOUT = 10.0
    #: Seconds the peers always get to exit after the guest.
    PEER_GRACE = 10.0
    #: Lines that only a Ztest run prints, besides those that carry a result.
    ZTEST_MARKS = (Test.test_suite_end_pattern, Test.test_suite_summary_pattern)
    DEFAULT_CONTROLLER_OPTION = '--bt-dev'

    @dataclass
    class Config:
        addresses: list[str] = field(default_factory=list)
        devices: list = field(default_factory=list)
        controller_option: str | None = None
        controllers_script: str | None = None

    @classmethod
    def config_schema(cls) -> dict:
        # A device is its argument string, or a mapping when it needs more.
        schema = super().config_schema()
        schema['properties']['devices'] = {
            'type': 'array',
            'items': {
                'anyOf': [
                    {'type': 'string'},
                    {
                        'type': 'object',
                        'properties': {'args': {'type': 'string'}, 'image': {'type': 'string'}},
                        'additionalProperties': False,
                    },
                ]
            },
        }
        return schema

    def configure(self, instance: TestInstance):
        super().configure(instance)
        self.addresses = list(self.config.addresses or self.DEFAULT_ADDRESSES)
        entries = [
            {'args': entry} if isinstance(entry, str) else entry
            for entry in (self.config.devices or [''])
        ]
        self.devices = [entry.get('args', '') for entry in entries]
        self.images = [entry.get('image') for entry in entries]
        option = self.config.controller_option
        self.controller_option = self.DEFAULT_CONTROLLER_OPTION if option is None else option
        script = self.config.controllers_script or self.DEFAULT_CONTROLLERS_SCRIPT
        self.script = os.path.join(ZEPHYR_BASE, script)
        self.ports: list[int] = []
        self._proc = None
        self._log = None
        self._peers: list[Peer] = []
        self._deadline = 0.0

    def host_ready(self) -> bool:
        """The test needs Bumble in twister's interpreter and the controllers script."""
        return importlib.util.find_spec('bumble') is not None and os.path.exists(self.script)

    def _resolve(self, spec: str) -> str:
        subs = {}
        for i, addr in enumerate(self.addresses):
            subs[f'addr{i}'] = addr
            subs[f'ctrl{i}'] = f'127.0.0.1:{self.ports[i]}'
        return spec.format(**subs)

    def _executable(self, index: int) -> str | None:
        """Executable of device ``index``, or None if its image is unknown.

        A device runs this test's own image unless it names one of the
        scenario's required applications, which twister has built by now.
        """
        build_dir = self.instance.build_dir
        if self.images[index] is not None:
            required = zip(
                self.instance.testsuite.required_applications,
                self.instance.required_build_dirs,
                strict=False,
            )
            build_dir = next(
                (path for app, path in required if app.application == self.images[index]), None
            )
        if build_dir is None:
            return None
        # As the handler does for the guest: a sysbuild build keeps the
        # application in the directory of its default domain.
        domains = os.path.join(build_dir, 'domains.yaml')
        if os.path.exists(domains):
            build_dir = Domains.from_file(domains).get_default_domain().build_dir
        return os.path.join(build_dir, 'zephyr', 'zephyr.exe')

    def _device_args(self, index: int, spec: str) -> list[str]:
        """Arguments of device ``index``, attached to its controller."""
        args = shlex.split(self._resolve(spec))
        if not self.controller_option:
            return args
        return [f'{self.controller_option}=127.0.0.1:{self.ports[index]}', *args]

    def _skip(self, reason: str) -> None:
        self.instance.status = TwisterStatus.SKIP
        self.instance.reason = reason
        self.instance.add_missing_case_status(TwisterStatus.SKIP, reason)
        logger.warning(
            f"SIDECAR:{self.__class__.__name__}: {reason}, skipping {self.instance.name}"
        )

    def _error(self, reason: str) -> None:
        self.instance.status = TwisterStatus.ERROR
        self.instance.reason = reason
        self.instance.add_missing_case_status(TwisterStatus.ERROR, reason)
        logger.error(f"SIDECAR:{self.__class__.__name__}: {reason}")

    def setup(self) -> bool:
        if not self.host_ready():
            self._skip("Bumble or the Bluetooth Classic simulation framework not found")
            return False

        if len(self.devices) > len(self.addresses):
            self._error(
                f"{len(self.devices)} devices but only {len(self.addresses)} controller addresses"
            )
            return False

        if self.images[0] is not None:
            self._error("device 0 is this test's own image and cannot name another")
            return False

        executables = [self._executable(i) for i in range(len(self.devices))]
        if None in executables:
            unknown = self.images[executables.index(None)]
            self._error(f"image '{unknown}' is not among the required applications")
            return False

        # Start the controllers, linked, in one process. Each is an HCI TCP
        # server the guest connects to; controllers.py takes
        # "<bumble-transport>@<bd_address>" per controller. Port 0 makes it
        # bind a kernel-chosen loopback port and report it on stdout, so
        # parallel twister workers never race for the same port.
        transport_args = [f'tcp-server:127.0.0.1:0@{addr}' for addr in self.addresses]
        log_path = os.path.join(self.instance.build_dir, 'bumble-controllers.log')
        # The controllers outlive setup(); the handle is closed in teardown().
        self._log = open(log_path, 'w')  # noqa: SIM115
        self._proc = subprocess.Popen(
            [sys.executable, self.script, *transport_args],
            cwd=os.path.dirname(self.script),
            stdout=subprocess.PIPE,
            stderr=self._log,
            start_new_session=True,
        )

        # The host looked ready, so controllers that do not come up are an
        # error in the simulation backend or its Bumble, not a reason to skip.
        if not self._wait_for_ports():
            self._error(f"Bumble controllers did not start (see {log_path})")
            return False

        try:
            device_args = [self._device_args(i, spec) for i, spec in enumerate(self.devices)]
        except (KeyError, IndexError, ValueError) as err:
            self._error(f"invalid device arguments: {err!r}")
            return False

        # The devices share the scenario's timeout, whichever of them the
        # handler watches.
        self._deadline = time.monotonic() + self.instance.handler.get_test_timeout()

        # Device 0 runs under the harness: inject its arguments so the
        # handler launches it against its controller.
        existing = list(self.instance.handler.extra_test_args or [])
        self.instance.handler.extra_test_args = existing + device_args[0]

        # Devices 1.. are peers the sidecar launches on their own controllers.
        for i, args in enumerate(device_args[1:], start=1):
            command = [executables[i], *args]
            log_path = os.path.join(self.instance.build_dir, f'bumble-peer{i}.log')
            log = open(log_path, 'w')  # noqa: SIM115
            proc = subprocess.Popen(
                command, stdout=log, stderr=subprocess.STDOUT, start_new_session=True
            )
            self._peers.append(Peer(proc, log, log_path))
            logger.debug(f"SIDECAR:{self.__class__.__name__}: started peer {i} (pid {proc.pid})")
        return True

    def _wait_for_ports(self) -> bool:
        """Collect the port each controller reports once it is listening.

        controllers.py prints one ``HCI<n> <address> <port>`` line per
        controller, in order, as soon as that controller's transport is
        bound. Returns False if the process exits or the start timeout passes
        before every controller has reported.
        """
        fd = self._proc.stdout.fileno()
        deadline = time.monotonic() + self.START_TIMEOUT
        pending = b''
        self.ports = []
        while len(self.ports) < len(self.addresses):
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                return False
            readable, _, _ = select.select([fd], [], [], min(remaining, 0.1))
            if not readable:
                if self._proc.poll() is not None:
                    return False
                continue
            chunk = os.read(fd, 4096)
            if not chunk:
                return False
            pending += chunk
            while b'\n' in pending:
                line, pending = pending.split(b'\n', 1)
                self._take_ready_line(line.decode(errors='replace'))
        return True

    def _take_ready_line(self, line: str) -> None:
        parts = line.split()
        if len(parts) != 3 or parts[0] != f'HCI{len(self.ports)}' or not parts[2].isdigit():
            return
        port = int(parts[2])
        if port > 0:
            self.ports.append(port)

    def _reap_peers(self, peers: list[Peer]) -> list[str | None]:
        """Wait for the peers to exit; returns why each one failed, if it did.

        A peer can outlive the guest, so together the peers get what is left
        of the scenario's timeout, and the grace period in any case. Once the
        guest has failed the result is settled and the grace period is all
        they get.
        """
        deadline = time.monotonic() + self.PEER_GRACE
        if self.instance.status == TwisterStatus.PASS:
            deadline = max(deadline, self._deadline)
        reasons = []
        for i, peer in enumerate(peers, start=1):
            try:
                ret = peer.proc.wait(timeout=max(deadline - time.monotonic(), 0))
                reasons.append(f"Bumble peer {i} exited with {ret}" if ret != 0 else None)
            except subprocess.TimeoutExpired:
                peer.proc.kill()
                peer.proc.wait()
                reasons.append(f"Bumble peer {i} did not finish")
            peer.log.close()
        return reasons

    def _stop_controllers(self) -> None:
        if self._proc is not None:
            terminate_process(self._proc)
            try:
                self._proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self._proc.kill()
                self._proc.wait()
            self._proc.stdout.close()
            self._proc = None
        if self._log is not None:
            self._log.close()
            self._log = None

    @staticmethod
    def _peer_results(log_path: str) -> tuple[dict[str, tuple[TwisterStatus, float]], bool | None]:
        """The cases a peer ran and its verdict, read from its Ztest output.

        The patterns are the harness's. The names are taken from the output
        itself rather than from what twister found in the guest's sources, so
        a peer built from other sources reports its own cases. Every device
        lists the cases it left out as skipped, which says nothing about
        them, so those are dropped.

        The verdict is True or False as the peer printed it, and None if it
        printed none. Without any Ztest output there is nothing to give a
        verdict on, which reads as True.
        """

        def seconds(text: str) -> float:
            return float(text.replace(',', '.') or 0)

        cases: dict[str, tuple[TwisterStatus, float]] = {}
        suite = ''
        ztest = False
        verdict = None
        with open(log_path, errors='replace') as log:
            for line in log:
                if match := Test.test_suite_start_pattern.search(line):
                    suite = match.group('suite_name')
                    ztest = True
                # Outside a suite, a line that looks like a case is the
                # application's own logging.
                elif suite and (match := Test.test_case_start_pattern.search(line)):
                    cases[f'{suite}.{match.group(2)}'] = (TwisterStatus.STARTED, 0.0)
                elif suite and (match := Test.test_case_end_pattern.match(line)):
                    status = TwisterStatus[match.group(1)]
                    cases[f'{suite}.{match.group(3)}'] = (status, seconds(match.group(4)))
                elif match := Test.test_case_summary_pattern.match(line):
                    status = TwisterStatus[match.group(1)]
                    name = f'{match.group(2)}.{match.group(4)}'
                    cases[name] = (status, seconds(match.group(5)))
                    ztest = True
                elif any(mark.search(line) for mark in BumbleSidecar.ZTEST_MARKS):
                    ztest = True
                if Harness.RUN_PASSED in line:
                    verdict = True
                elif Harness.RUN_FAILED in line:
                    verdict = False
        if verdict is None and not ztest:
            verdict = True
        ran = {name: result for name, result in cases.items() if result[0] != TwisterStatus.SKIP}
        return ran, verdict

    def _peer_failure(self, index: int, log_path: str, ended: str | None) -> str | None:
        """Report the cases a peer ran; returns why the peer failed, if it did."""
        cases, verdict = self._peer_results(log_path)
        failed = []
        for name, (status, duration) in cases.items():
            case = self.instance.get_case_or_create(self.instance.testsuite.compose_case_name(name))
            if status != TwisterStatus.PASS:
                # Also a case that started and never ended.
                case.status = TwisterStatus.FAIL
                case.reason = f"Failed on Bumble peer {index}"
                case.duration = duration
                failed.append(case.name)
            elif case.status != TwisterStatus.FAIL:
                case.status = TwisterStatus.PASS
                case.reason = None
                case.duration = duration
        if failed:
            return f"Bumble peer {index}: {', '.join(failed)} failed"
        if ended is not None:
            return ended
        if verdict is False:
            return f"Bumble peer {index} failed"
        # The handler does not take a guest's exit status for a result either.
        if verdict is None:
            return f"Bumble peer {index} ended without a verdict"
        return None

    def teardown(self) -> None:
        # Stop everything before looking at any result, so that nothing is
        # left running whatever the results turn out to be.
        peers, self._peers = self._peers, []
        ended = self._reap_peers(peers)
        self._stop_controllers()

        # A peer failing fails the test even if this guest passed, since both
        # sides must complete the exchange.
        for i, peer in enumerate(peers, start=1):
            reason = self._peer_failure(i, peer.log_path, ended[i - 1])
            if reason is not None and self.instance.status == TwisterStatus.PASS:
                self.instance.status = TwisterStatus.FAIL
                self.instance.reason = reason
                logger.error(f"SIDECAR:{self.__class__.__name__}: {self.instance.reason}")

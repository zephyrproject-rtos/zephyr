# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""can sidecar: brings up the host CAN bus a QEMU or native CAN test needs."""

from __future__ import annotations

import hashlib
import logging
import os
import shlex
import subprocess
from dataclasses import dataclass, field

from twisterlib.handlers import terminate_process
from twisterlib.sidecars.base import Sidecar
from twisterlib.statuses import TwisterStatus
from twisterlib.testinstance import TestInstance

logger = logging.getLogger('twister')


def get_can_iface_name(build_dir: str) -> str:
    """Return a unique host SocketCAN interface name for a build directory.

    Both the CMake configure step (which bakes the name into
    ``CONFIG_CAN_QEMU_IFACE_NAME`` so QEMU connects its CAN bus to it) and the
    sidecar (which creates the interface) compute this independently from
    ``build_dir``, so they always agree without sharing any state. Kept within
    the 15 character interface name limit the kernel enforces.
    """
    digest = hashlib.sha1(os.path.abspath(build_dir).encode()).hexdigest()[:8]
    return f'zcan{digest}'


class CanSidecar(Sidecar):
    """Brings up the host CAN bus a QEMU or native CAN test needs.

    The guest reaches the host interface differently per platform, and the
    sidecar wires up both itself:

    - QEMU connects its emulated CAN controller (e.g. the Kvaser PCIcan on x86)
      to a host SocketCAN interface through ``-object can-host-socketcan``, which
      cmake/emu/qemu.cmake adds when ``CONFIG_CAN_QEMU_IFACE_NAME`` is set.
      :meth:`cmake_args` bakes the interface name into the build.
    - native_sim's SocketCAN driver takes the interface at run time instead, so
      :meth:`setup` passes it to the binary via ``--can-if``.

    The interface must exist before the guest boots, so the sidecar creates a
    virtual CAN (``vcan``) interface in :meth:`setup` and removes it in
    :meth:`teardown`. Each instance gets a unique name (see
    :func:`get_can_iface_name`), so runs do not collide.

    QEMU's CAN models do not self-receive in loopback mode, so a functional test
    exchanges frames in normal mode with a companion on the bus. Companion host
    apps are given with ``tools_apps``: each is a command run after the interface
    is up and stopped before it is torn down, with ``{iface}`` and
    ``{source_dir}`` (the test's directory, so a test can ship its own companion)
    substituted.

    Creating a ``vcan`` interface needs root; the sidecar runs ``ip`` through
    ``sudo -n`` when not already root and skips the test (rather than hanging on
    a password prompt) when that is not available or ``vcan`` is unsupported.

    Configured through the ``can`` block of ``sidecar_config``:

    - ``iface``: interface name to create. Defaults to a name derived from the
      build directory.
    - ``tools_apps``: list of companion host apps/commands to run on the bus.
    """

    NAME = 'can'

    @dataclass
    class Config:
        iface: str | None = None
        tools_apps: list[str] = field(default_factory=list)

    def configure(self, instance: TestInstance):
        super().configure(instance)
        self.iface = self.config.iface or get_can_iface_name(instance.build_dir)
        self.tools_apps = self.config.tools_apps
        self._started = False
        self._apps = []

    def cmake_args(self, build_dir: str) -> list[str]:
        # Bake the interface name into the build so cmake/emu/qemu.cmake connects
        # QEMU's CAN bus to it. Recomputed from build_dir (rather than read off
        # self) because the configure step builds its own sidecar instance.
        iface = self.config.iface or get_can_iface_name(build_dir)
        return [f'-DCONFIG_CAN_QEMU_IFACE_NAME="{iface}"']

    def _ip(self, *args):
        cmd = []
        if os.geteuid() != 0:
            # -n so a missing passwordless sudo fails fast instead of prompting.
            cmd += ['sudo', '-n']
        return [*cmd, 'ip', *args]

    def setup(self) -> bool:
        if (
            os.geteuid() != 0
            and subprocess.run(['sudo', '-n', 'true'], capture_output=True).returncode != 0
        ):
            self._skip("creating a vcan interface needs root or passwordless sudo")
            return False

        # Best effort: the vcan module is usually autoloaded by "ip link add".
        modprobe = (['sudo', '-n'] if os.geteuid() != 0 else []) + ['modprobe', 'vcan']
        subprocess.run(modprobe, capture_output=True)

        # Remove a stale interface left by a previous, crashed run.
        subprocess.run(self._ip('link', 'del', self.iface), capture_output=True)

        result = subprocess.run(
            self._ip('link', 'add', 'dev', self.iface, 'type', 'vcan'),
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            self._skip(
                f"could not create vcan interface (vcan supported?): {result.stderr.strip()}"
            )
            return False
        subprocess.run(self._ip('link', 'set', 'up', self.iface), capture_output=True)

        self._started = True
        logger.debug(f"SIDECAR:{self.__class__.__name__}: brought up {self.iface}")

        # On native platforms the SocketCAN driver takes the host interface at
        # run time rather than from CONFIG_CAN_QEMU_IFACE_NAME baked for QEMU.
        # Done here, not in the runner, so this stays a drop-in sidecar module.
        if self.instance.platform.arch == 'posix':
            handler = self.instance.handler
            arg = f'--can-if={self.iface}'
            args = list(handler.extra_test_args or [])
            # setup() may run again for a retry; do not append the flag twice.
            if arg not in args:
                handler.extra_test_args = args + [arg]

        return self._start_apps()

    def _start_apps(self) -> bool:
        source_dir = self.instance.testsuite.source_dir
        for spec in self.tools_apps:
            command = spec.format(iface=self.iface, source_dir=source_dir)
            argv = shlex.split(command)
            missing = next(
                (tok for tok in argv if os.path.sep in tok and not os.path.exists(tok)), None
            )
            if missing:
                self._skip(f"companion app not found: {missing}")
                return False

            name = os.path.basename(argv[0])
            # The companion outlives _start_apps(); closed in teardown().
            log = open(os.path.join(self.instance.build_dir, f"can-{name}.log"), 'w')  # noqa: SIM115
            proc = subprocess.Popen(
                argv, stdout=log, stderr=subprocess.STDOUT, start_new_session=True
            )
            self._apps.append((proc, log))
            logger.debug(f"SIDECAR:{self.__class__.__name__}: started {name} (pid {proc.pid})")
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
            subprocess.run(self._ip('link', 'del', self.iface), capture_output=True)
            self._started = False

    def _skip(self, reason):
        self.instance.status = TwisterStatus.SKIP
        self.instance.reason = reason
        self.instance.add_missing_case_status(TwisterStatus.SKIP, reason)
        logger.warning(
            f"SIDECAR:{self.__class__.__name__}: {reason}, skipping {self.instance.name}"
        )

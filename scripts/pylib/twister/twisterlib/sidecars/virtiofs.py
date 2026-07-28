# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""virtiofs sidecar: shares a host directory with the guest over virtiofs."""

from __future__ import annotations

import hashlib
import logging
import os
import shutil
import subprocess
import tempfile
from dataclasses import dataclass, field

from twisterlib.handlers import terminate_process
from twisterlib.sidecars.base import Sidecar
from twisterlib.statuses import TwisterStatus
from twisterlib.testinstance import TestInstance

logger = logging.getLogger('twister')


def get_virtiofs_socket_path(build_dir: str) -> str:
    """Return a short, unique vhost-user socket path for a build directory.

    The virtiofs sidecar launches ``virtiofsd`` on a UNIX socket that QEMU
    connects to through a ``-chardev socket`` flag. AF_UNIX paths are limited
    to roughly 108 bytes, so the socket cannot live inside ``build_dir`` whose
    path is often too long. Derive a short unique name from a hash of
    ``build_dir`` and place it in the system temporary directory instead.

    Both the CMake configure step (which bakes the path into QEMU's flags) and
    the sidecar (which launches ``virtiofsd``) compute this independently from
    ``build_dir``, so they always agree without sharing any state.
    """
    digest = hashlib.sha1(os.path.abspath(build_dir).encode()).hexdigest()[:16]
    return os.path.join(tempfile.gettempdir(), f'twister-virtiofs-{digest}.sock')


class VirtiofsSidecar(Sidecar):
    """Shares a host directory with the guest over virtiofs.

    Tests using this sidecar run on QEMU with a ``vhost-user-fs-pci`` device.
    That device needs a ``virtiofsd`` daemon running on the host and connected
    to QEMU through a UNIX socket before QEMU boots. The sidecar starts the
    daemon in :meth:`setup` and stops it in :meth:`teardown`.

    The socket path is derived from the build directory (see
    :func:`get_virtiofs_socket_path`) and injected into QEMU's flags by
    :meth:`cmake_env` at CMake configure time, so many tests can run in parallel
    without colliding on a shared socket.

    Configured through the ``virtiofs`` block of ``sidecar_config``:

    - ``shared``: path, relative to the test source directory, of a template
      directory whose contents seed the shared filesystem. A private writable
      copy is made under the build directory so tests may modify it.
    - ``bin``: explicit path to the ``virtiofsd`` binary. Defaults to the first
      ``virtiofsd`` found on ``PATH`` or in a common libexec location.
    - ``extra_args``: list of additional arguments passed to ``virtiofsd``.
    """

    NAME = 'virtiofs'

    @dataclass
    class Config:
        shared: str | None = None
        bin: str | None = None
        extra_args: list[str] = field(default_factory=list)

    # Common locations for the Rust virtiofsd, which is frequently installed
    # outside PATH (e.g. as a QEMU helper) by distribution packages.
    VIRTIOFSD_FALLBACK_PATHS = (
        '/usr/libexec/virtiofsd',
        '/usr/lib/qemu/virtiofsd',
        '/usr/local/libexec/virtiofsd',
    )

    @classmethod
    def find_virtiofsd(cls):
        found = shutil.which('virtiofsd')
        if found:
            return found
        for candidate in cls.VIRTIOFSD_FALLBACK_PATHS:
            if os.access(candidate, os.X_OK):
                return candidate
        return None

    def configure(self, instance: TestInstance):
        super().configure(instance)
        self.virtiofsd_bin = self.config.bin or self.find_virtiofsd()
        self.virtiofs_shared = self.config.shared
        self.virtiofs_extra_args = self.config.extra_args
        self.socket_path = get_virtiofs_socket_path(instance.build_dir)
        self.shared_dir = os.path.join(instance.build_dir, 'virtiofs-shared')
        self._virtiofsd_proc = None
        self._virtiofsd_log = None

    def cmake_env(self, build_dir: str) -> dict[str, str]:
        # The static vhost-user-fs device lives in the test's
        # CONFIG_QEMU_EXTRA_FLAGS; inject only the dynamic -chardev socket path
        # here via QEMU_EXTRA_FLAGS, which cmake/emu/qemu.cmake appends before
        # the config flags so the chardev is defined before the device that
        # references it.
        socket_path = get_virtiofs_socket_path(build_dir)
        chardev = f"-chardev socket,id=char0,path={socket_path}"
        existing = os.environ.get('QEMU_EXTRA_FLAGS', '')
        return {'QEMU_EXTRA_FLAGS': f"{chardev} {existing}".strip()}

    def setup(self) -> bool:
        if not self.virtiofsd_bin:
            self.instance.status = TwisterStatus.SKIP
            self.instance.reason = "virtiofsd not found on host"
            self.instance.add_missing_case_status(TwisterStatus.SKIP, self.instance.reason)
            logger.warning(
                f"SIDECAR:{self.__class__.__name__}: virtiofsd not found,"
                f" skipping {self.instance.name}"
            )
            return False

        # Seed a private, writable copy of the shared directory so tests that
        # create or modify files do not mutate the checked-in template.
        self._remove_shared_dir()
        if self.virtiofs_shared:
            template = os.path.join(self.instance.testsuite.source_dir, self.virtiofs_shared)
            shutil.copytree(template, self.shared_dir)
        else:
            os.makedirs(self.shared_dir, exist_ok=True)

        # A stale socket from a previous run would make virtiofsd refuse to bind.
        if os.path.exists(self.socket_path):
            os.unlink(self.socket_path)

        command = [
            self.virtiofsd_bin,
            f'--socket-path={self.socket_path}',
            '--shared-dir',
            self.shared_dir,
        ]
        command.extend(self.virtiofs_extra_args)

        log_path = os.path.join(self.instance.build_dir, 'virtiofsd.log')
        try:
            # The daemon outlives setup(); the handle is closed in teardown().
            self._virtiofsd_log = open(log_path, 'w')  # noqa: SIM115
            self._virtiofsd_proc = subprocess.Popen(
                command,
                stdout=self._virtiofsd_log,
                stderr=subprocess.STDOUT,
            )
        except OSError as err:
            self.instance.status = TwisterStatus.ERROR
            self.instance.reason = f"could not launch virtiofsd: {err}"
            self.instance.add_missing_case_status(TwisterStatus.ERROR, self.instance.reason)
            logger.error(f"SIDECAR:{self.__class__.__name__}: {self.instance.reason}")
            return False

        logger.debug(
            f"SIDECAR:{self.__class__.__name__}: started virtiofsd"
            f" (pid {self._virtiofsd_proc.pid}) on {self.socket_path}"
        )
        return True

    def teardown(self) -> None:
        if self._virtiofsd_proc is not None:
            terminate_process(self._virtiofsd_proc)
            try:
                self._virtiofsd_proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self._virtiofsd_proc.kill()
            self._virtiofsd_proc = None
        if self._virtiofsd_log is not None:
            self._virtiofsd_log.close()
            self._virtiofsd_log = None
        if os.path.exists(self.socket_path):
            os.unlink(self.socket_path)
        # Tests may create host directories with restrictive permissions, which
        # would make a later shutil.rmtree (here or when twister clobbers the
        # output) fail. Remove the share tolerantly now.
        self._remove_shared_dir()

    def _remove_shared_dir(self):
        if not os.path.exists(self.shared_dir):
            return

        def _on_error(func, path, _exc):
            # A directory the guest created without write/exec permission cannot
            # be traversed or unlinked; restore access and retry.
            os.chmod(os.path.dirname(path), 0o700)
            os.chmod(path, 0o700)
            func(path)

        shutil.rmtree(self.shared_dir, onexc=_on_error)

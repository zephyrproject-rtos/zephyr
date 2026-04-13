# Copyright (c) 2026 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
"""Robot Framework library for Zephyr QEMU robot tests using named FIFO pipes."""

import atexit
import contextlib
import os
import subprocess
import threading
import time

from SimLibraryBase import SimLibraryBase


class QemuSimLibrary(SimLibraryBase):
    """Keyword library that starts QEMU via the Zephyr build system and provides
    UART-like interaction over the named FIFOs that Zephyr's QEMU target sets up
    (QEMU_PIPE=<build_dir>/qemu-fifo, so FIFOs are <build_dir>/qemu-fifo.in/.out).
    """

    def __init__(self):
        super().__init__()
        self._qemu_thread = None
        self._fifo_in_fp = None

    def start_qemu(self, build_dir, generator_cmd):
        """Create FIFOs and start QEMU via ``generator_cmd -C build_dir run``.

        The QEMU_PIPE cmake variable is already baked into the build as
        ``<build_dir>/qemu-fifo`` from the cmake configure step, so QEMU will
        open <build_dir>/qemu-fifo.in for reading and <build_dir>/qemu-fifo.out
        for writing.  This keyword creates those FIFOs and opens them from the
        host side so robot keywords can read/write QEMU's serial output.
        """
        fifo_fn = os.path.join(build_dir, "qemu-fifo")
        fifo_in = fifo_fn + ".in"
        fifo_out = fifo_fn + ".out"

        # Remove stale FIFOs from a previous run
        for path in (fifo_in, fifo_out):
            if os.path.exists(path):
                os.unlink(path)
            os.mkfifo(path)

        # Start QEMU in a daemon thread so it does not block the keyword
        def _run_qemu():
            log_path = os.path.join(build_dir, "qemu.robot.log")
            with open(log_path, "w") as log_fp:
                proc = subprocess.Popen(
                    [generator_cmd, "-C", build_dir, "run"],
                    stdout=log_fp,
                    stderr=subprocess.STDOUT,
                    cwd=build_dir,
                )
                proc.wait()

        self._qemu_thread = threading.Thread(target=_run_qemu, daemon=True)
        self._qemu_thread.start()
        atexit.register(self.stop_qemu)

        # Open FIFOs – each open() blocks until QEMU opens the other end.
        # fifo_in:  host writes → QEMU reads (host opens O_WRONLY)
        # fifo_out: QEMU writes → host reads (host opens O_RDONLY)
        def _open_and_read():
            with (
                open(fifo_in, "wb") as fifo_in_fp,
                open(fifo_out, "rb", buffering=0) as fifo_out_fp,
            ):
                self._fifo_in_fp = fifo_in_fp
                self._read_fifo(fifo_out_fp)
            self._fifo_in_fp = None

        # Buffer QEMU output in a background thread
        self._reader = threading.Thread(target=_open_and_read, daemon=True)
        self._reader.start()

        # Wait until FIFOs are open before returning
        while self._fifo_in_fp is None and self._reader.is_alive():
            time.sleep(0.01)

    def _read_fifo(self, fp):
        while True:
            try:
                data = fp.read(256)
                if not data:
                    break
                with self._lock:
                    self._buffer += self._strip_ansi(data)
            except OSError:
                break

    def write_to_uart(self, text):
        """Write *text* followed by a newline to QEMU's serial input."""
        if self._fifo_in_fp is None:
            raise RuntimeError('QEMU is not running')
        self._fifo_in_fp.write((text + '\n').encode())
        self._fifo_in_fp.flush()

    def stop_qemu(self):
        """Close FIFOs; the QEMU process exits naturally when FIFOs are closed."""
        if self._fifo_in_fp:
            with contextlib.suppress(OSError):
                self._fifo_in_fp.close()
            self._fifo_in_fp = None

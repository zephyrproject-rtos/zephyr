# scripts/west_commands/runners/qemu.py
"""
Minimal QEMU runner for Zephyr (starter).
Drop this under scripts/west_commands/runners/ and extend as needed.
"""

import contextlib
import os
import shlex
import shutil
import subprocess
import tempfile
from pathlib import Path

from runners.core import RunnerConfig, ZephyrBinaryRunner, log


# Runner metadata
class QemuRunner(ZephyrBinaryRunner):
    name = "qemu"
    description = "Run Zephyr ELF using QEMU (starter implementation)"

    @classmethod
    def add_parser(cls, parser):
        super().add_parser(parser)
        parser.add_argument(
            "--qemu-binary",
            help="Path to qemu-system-<arch> binary (if omitted, a best-guess is used)",
        )
        parser.add_argument(
            "--qemu-arg",
            action="append",
            help="Extra CLI args to append to qemu (can be given multiple times)",
        )
        parser.add_argument(
            "--qemu-serial",
            default=None,
            help="Serial backend: stdio, pty, file:<path> (default: stdio if available)",
        )
        parser.add_argument(
            "--qemu-keep-fifos",
            action="store_true",
            help="Do not remove temporary FIFO/tty files on exit (for debugging)",
        )

    @classmethod
    def create(cls, cfg: RunnerConfig):
        """
        Factory called by west. Return QemuRunner(cfg) if the runner can run in this environment,
        or None otherwise.
        """
        # Only create runner if we have an ELF
        if not cfg.elf_file:
            return None

        # Basic availability check is deferred until do_run; allow create to
        # succeed so 'west flash --context' lists it.
        return QemuRunner(cfg)

    def __init__(self, cfg: RunnerConfig):
        super().__init__(cfg)
        self.cfg = cfg
        self.elf = cfg.elf_file
        self.build_dir = cfg.build_dir or os.getcwd()
        self._tmpdir = None
        self._fifos = []

    def do_run(self, command, timeout=0, **kwargs):
        """
        Entry point invoked when user runs "west flash/run -r qemu" (or similar).
        This should prepare FIFOs/serial, build command line, and spawn QEMU.
        """
        # 1) Ensure qemu binary
        qemu_bin = kwargs.get("qemu_binary") or self._guess_qemu_binary()
        if not qemu_bin or not shutil.which(qemu_bin):
            raise RuntimeError(
                "QEMU binary not found: set --qemu-binary or install qemu-system-* on PATH"
            )

        # 2) Prepare temporary directory for FIFOs/ptys
        self._tmpdir = Path(tempfile.mkdtemp(prefix="zephyr-qemu-"))
        log.debug("Using temporary qemu dir: %s", str(self._tmpdir))

        # 3) Prepare serial backend (simple: use stdio or create FIFO for outside tools)
        serial_spec = kwargs.get("qemu_serial") or "stdio"
        serial_option = self._prepare_serial(serial_spec)

        # 4) Build qemu commandline
        qemu_cmd = [qemu_bin]
        qemu_cmd += self._platform_defaults()
        qemu_cmd += ["-kernel", str(self.elf)]
        qemu_cmd += serial_option
        extra_args = kwargs.get("qemu_arg") or []
        # allow both list or single-string extra args
        if isinstance(extra_args, str):
            extra_args = shlex.split(extra_args)
        for arg in extra_args:
            if isinstance(arg, str):
                qemu_cmd += shlex.split(arg)

        # Logging / dry-run support:
        log.inf("QEMU cmd: %s", " ".join(shlex.quote(x) for x in qemu_cmd))

        # 5) Spawn QEMU process
        proc = subprocess.Popen(qemu_cmd, cwd=self.build_dir)

        try:
            # Wait: if timeout==0 then wait forever until QEMU exits
            ret = proc.wait(timeout=None if timeout == 0 else timeout)
            return ret
        finally:
            # 6) Cleanup
            if not kwargs.get("qemu_keep_fifos"):
                self._cleanup()

    def _guess_qemu_binary(self):
        # Default fallback
        for p in (
            "qemu-system-x86_64",
            "qemu-system-x86",
            "qemu-system-arm",
            "qemu-system-riscv64",
        ):
            if shutil.which(p):
                return p
        return None

    def _platform_defaults(self):
        # Minimal defaults. Extend per-board: memory, machine type etc.
        # For common x86 qemu_x86: use -M pc -m 512M -nographic
        board = (self.cfg.board or "").lower() if hasattr(self.cfg, "board") else ""
        if "qemu_x86" in board or "qemu_x86" in str(self.cfg.board_dir):
            return ["-M", "pc", "-m", "512", "-nographic"]
        # cortex-m3 qemu example:
        if "cortex_m3" in board or "qemu_cortex_m3" in str(self.cfg.board_dir):
            return ["-M", "lm3s6965evb", "-nographic"]
        # fall back to minimal no-graphic
        return ["-nographic"]

    def _prepare_serial(self, spec):
        """
        Return list of qemu args for serial based on spec.
        Spec examples:
        - "stdio": returns ["-serial", "stdio"]
        - "file:/tmp/qemu-serial": creates file, returns ["-serial", "file:..."]
        - "fifo": creates fifo in tmpdir, returns ["-serial", "pipe:..."]
        """
        if spec == "stdio":
            return ["-serial", "stdio"]
        if spec.startswith("file:"):
            path = spec.split(":", 1)[1]
            path = os.path.expanduser(path)
            # ensure containing dir exists
            Path(path).parent.mkdir(parents=True, exist_ok=True)
            return ["-serial", f"file:{path}"]
        if spec == "fifo" or spec.startswith("fifo:"):
            # Make two FIFOs for serial in/out (POSIX)
            in_fifo = str(self._tmpdir / "qemu-serial-in")
            out_fifo = str(self._tmpdir / "qemu-serial-out")
            try:
                os.mkfifo(in_fifo)
                os.mkfifo(out_fifo)
                self._fifos += [in_fifo, out_fifo]
                # Example approach: use tty server or pty trick -- keep this simple for now
                # QEMU supports -serial pipe:<name> on some configurations;
                # here we return one FIFO file for debugging.
                return ["-serial", f"file:{out_fifo}"]
            except Exception as e:
                log.err("Failed creating FIFOs: %s", e)
                return ["-serial", "stdio"]
        # fallback
        return ["-serial", "stdio"]

    def _cleanup(self):
        # Remove FIFOs and tmpdir
        for p in self._fifos:
            with contextlib.suppress(Exception):
                os.remove(p)
        if self._tmpdir:
            with contextlib.suppress(Exception):
                shutil.rmtree(self._tmpdir)

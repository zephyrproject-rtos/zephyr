#!/usr/bin/env python3
#
# Copyright (c) 2026 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""Read a QEMU console served over a Windows named pipe.

Diagnostic aid for the qemu_cortex_m0 stall seen on Windows runners. This
starts QEMU with the same pipe chardev twister uses and reads the guest
console from it, without any of the twister machinery around it, so that a
stall in the guest can be told apart from a stall in twister's reader.

Every line is timestamped relative to QEMU start, which shows whether output
stops dead or merely slows down.
"""

import argparse
import os
import subprocess
import sys
import threading
import time

# Poll interval while waiting for QEMU to create the pipe. QEMU discards
# console output written before a client attaches, so this stays small.
PIPE_OPEN_RETRY_INTERVAL = 0.01


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--qemu", required=True, help="path to qemu-system-arm")
    parser.add_argument("--elf", required=True, help="path to zephyr.elf")
    parser.add_argument("--fifo", required=True, help="pipe path passed to QEMU")
    parser.add_argument("--timeout", type=float, default=30.0)
    return parser.parse_args()


def reader(fifo, start, deadline, out):
    """Attach to the pipe and echo whatever QEMU writes, with timestamps."""
    pipe_path = r"\\.\pipe\\" + fifo
    handle = None

    while time.time() < deadline:
        if handle is None:
            try:
                handle = os.open(pipe_path, os.O_RDONLY)
                out.append(f"[{time.time() - start:7.3f}] <reader attached>")
            except OSError:
                time.sleep(PIPE_OPEN_RETRY_INTERVAL)
            continue

        try:
            c = os.read(handle, 1)
        except OSError as e:
            out.append(f"[{time.time() - start:7.3f}] <read failed: {e}>")
            return

        if c == b"":
            out.append(f"[{time.time() - start:7.3f}] <eof>")
            return

        out.append(c)

    out.append(f"[{time.time() - start:7.3f}] <deadline reached>")


def main():
    args = parse_args()

    command = [
        args.qemu,
        "-cpu", "cortex-m0",
        "-machine", "microbit",
        "-nographic",
        "-chardev", f"pipe,id=con,mux=on,path={args.fifo}",
        "-serial", "chardev:con",
        # twister muxes the monitor onto the same chardev, and the mux is
        # itself a suspect, so mirror it exactly.
        "-mon", "chardev=con,mode=readline",
        "-net", "none",
        "-kernel", args.elf,
    ]
    print("command:", " ".join(command), flush=True)

    # QEMU expects the path to exist before it opens the chardev, the same way
    # the twister build does with a cmake -E touch.
    with open(args.fifo, "a"):
        pass

    start = time.time()
    deadline = start + args.timeout
    chunks = []

    thread = threading.Thread(target=reader, args=(args.fifo, start, deadline, chunks))
    thread.daemon = True
    thread.start()

    proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    try:
        proc.wait(timeout=args.timeout)
    except subprocess.TimeoutExpired:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()

    thread.join(timeout=5)

    qemu_output = proc.stdout.read() if proc.stdout else b""

    # Reassemble, keeping the reader's own markers on their own lines.
    text = ""
    for chunk in chunks:
        text += chunk.decode("utf-8", errors="replace") if isinstance(chunk, bytes) \
            else f"\n{chunk}\n"

    print(f"--- console via pipe ({len(text)} chars) ---")
    print(text)
    print("--- qemu process output ---")
    print(qemu_output.decode("utf-8", errors="replace"))
    print(f"--- qemu exit code: {proc.returncode} ---")

    return 0


if __name__ == "__main__":
    sys.exit(main())

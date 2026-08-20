#!/usr/bin/env python3

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: Apache-2.0

import os
import shlex
import sys

from elftools.elf.elffile import ELFFile

VERBOSE = False


def get_e_entry_from_elf(elf_path: str) -> int | None:
    '''Read e_entry from header of ELF located at elf_path'''
    with open(elf_path, 'rb') as f:
        try:
            return ELFFile(f).header['e_entry']
        except Exception:
            return None


def run_qemu(real_qemu, qemu_args):
    if VERBOSE:
        print(
            f'\n[qemu_entry_wrapper] Executing: {shlex.join([real_qemu] + qemu_args)}',
            file=sys.stderr,
        )
    os.execvp(real_qemu, [real_qemu] + qemu_args)


def main():
    argv = sys.argv[1:]

    real_qemu = argv[0]
    num_cpus = int(argv[1])
    qemu_args = argv[2:]

    # Obtain "-kernel <elf_path>" value provided by QEMU invocation through Zephyr.
    # This value represents the path the ELF QEMU will execute.
    kernel = None
    for prev, arg in zip(qemu_args, qemu_args[1:], strict=True):
        if prev == '-kernel':
            kernel = arg
            break

    if kernel is None:
        run_qemu(real_qemu, qemu_args)
        return

    # Extract "e_entry" from the ELF file. If not specified, run QEMU unmodified.
    entry = get_e_entry_from_elf(kernel)
    if entry is None:
        print(
            f'[qemu_entry_wrapper] failed to read entry point from {kernel}, '
            'running qemu unmodified',
            file=sys.stderr,
        )
        run_qemu(real_qemu, qemu_args)
        return

    # Update PC for each core to the extracted entry point.
    for cpu in range(num_cpus):
        qemu_args += ['-device', f'loader,addr={entry},cpu-num={cpu}']

        print(f'\n[qemu_entry_wrapper] Running QEMU with entry point at 0x{entry:x} for CPU {cpu}')

    print('')
    run_qemu(real_qemu, qemu_args)


if __name__ == '__main__':
    main()

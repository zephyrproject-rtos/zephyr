#!/usr/bin/env python3

# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors

"""
Compiler launcher wrapper that captures what appears to be Devicetree-related build errors, and
diagnoses them using diagnose_build_error.py.

The tool is meant to be configured as a CMAKE_<LANG>_COMPILER_LAUNCHER or as a
CMAKE_<LANG>_LINKER_LAUNCHER.

Example usage:

  ./scripts/dts/dtdoctor_sca_wrapper.py --edt-pickle <path> \\
    -- <compiler-or-linker> <args...>
"""

import argparse
import os
import re
import subprocess
import sys


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument(
        "--edt-pickle", help="path to edt.pickle file corresponding to the build to analyze"
    )

    if "--" in sys.argv:
        idx = sys.argv.index("--")
        args, cmd = parser.parse_known_args(sys.argv[1:idx])[0], sys.argv[idx + 1 :]
    else:
        args, cmd = parser.parse_known_args(sys.argv[1:])[0], sys.argv[1:]

    # Run compiler/linker command
    proc = subprocess.run(cmd, capture_output=True, text=True)
    sys.stdout.write(proc.stdout)
    sys.stderr.write(proc.stderr)

    # Extract __device_dts_ord_xxx symbols from errors and run diagnostics
    if proc.returncode != 0 and args.edt_pickle:
        patterns = [
            r"(__device_dts_ord_\d+).*undeclared here",  # gcc
            r"undefined reference to.*(__device_dts_ord_\d+)",  # ld
            r"use of undeclared identifier '(__device_dts_ord_\d+)'",  # LLVM/clang (ATfE)
            r"undefined symbol: \(__device_dts_ord_(\d+)",  # LLVM/lld (ATfE)
        ]
        symbols = {m for p in patterns for m in re.findall(p, proc.stderr)}

        diag_script = os.path.join(os.path.dirname(__file__), "dtdoctor_analyzer.py")
        for symbol in sorted(symbols):
            subprocess.run(
                [
                    sys.executable,
                    diag_script,
                    "--edt-pickle",
                    args.edt_pickle,
                    "--symbol",
                    symbol,
                ]
            )

    return proc.returncode


if __name__ == "__main__":
    sys.exit(main())

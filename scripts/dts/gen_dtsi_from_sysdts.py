# Copyright (c) 2022, 2023 Nordic Semiconductor ASA

'''
Helper script that converts a system DT to a
'regular' devicetree include file using devicetree.sysdtlib.
'''

from pathlib import Path
import argparse
import os
import subprocess
import sys
from pathlib import Path
import pickle

HERE = Path(__file__).parent

sys.path.insert(0, str(HERE / 'python-devicetree' / 'src'))

from devicetree import sysdtlib
import yaml

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument('--sysdts', required=True,
                        help='path to the system devicetree file')
    parser.add_argument('--domain', required=True,
                        help='''path to the execution domain in the
                             system devicetree which selects the CPUs,
                             etc that should appear in the generated
                             devicetree''')
    parser.add_argument('--sysdt-pickle-out', dest='sysdt_pickle',
                        help='''if given, saves the sysdtlib.SysDT to this
                        file in pickle format''')
    parser.add_argument('--dts-out', dest='dts_out', required=True,
                        help='path to the system devicetree file')
    return parser.parse_args()

def main():
    args = parse_args()
    sysdt = sysdtlib.SysDT(args.sysdts)

    if args.sysdt_pickle:
        with open(args.sysdt_pickle, 'wb') as f:
            # Pickle protocol version 4 is the default as of Python
            # 3.8, so it is both available and recommended on all
            # versions of Python that Zephyr supports.
            #
            # Using a common protocol version here will hopefully avoid
            # reproducibility issues in different Python installations.
            pickle.dump(sysdt, f, protocol=4)

    dt = sysdt.convert_to_dt(args.domain)

    with open(args.dts_out, 'w') as f:
        f.write(f"{dt}")

if __name__ == '__main__':
    main()

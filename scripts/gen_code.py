#!/usr/bin/env python3
#
# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

# Zephyr 'cogeno' launcher which is interoperable with:
#
# 1. "multi-repo" Zephyr installations where 'cogeno' is provided in a
#    separate Git repository side by side to Zephyr.
#
# 2. Zephyr installations that have 'cogeno' installed by PyPi.
#
# 3. "mono-repo" Zephyr installations that have 'cogeno' supplied by a copy
#    in scripts/cogeno.
#

import sys
import subprocess
import importlib.util
from pathlib import Path


if sys.version_info < (3,):
    sys.exit('fatal error: you are running Python 2')

# 1. - cogeno git repository side by side to zephyr

zephyr_path = Path(__file__).resolve().parents[1]

cogeno_module_dir = zephyr_path.parent.joinpath('cogeno')

if cogeno_module_dir.is_dir():
    cogeno_module_dir_s = str(cogeno_module_dir)
    sys.path.insert(0, cogeno_module_dir_s)
    try:
        from cogeno.cogeno import main
        try:
            desc = check_output(['git', 'describe', '--tags'],
                                stderr=DEVNULL,
                                cwd=str(cogeno_module_dir))
            cogeno_version = desc.decode(sys.getdefaultencoding()).strip()
        except:
            cogeno_version = 'unknown'
        cogeno_path = cogeno_module_dir.joinpath('cogeno')
        print("NOTE: you are running cogeno from '{}' ({});".format(cogeno_path, cogeno_version))
    except:
        cogeno_module_dir = None
    finally:
        sys.path.remove(cogeno_module_dir_s)
else:
    cogeno_module_dir = None

if cogeno_module_dir is None:
    cogeno_spec = importlib.util.find_spec('cogeno')
    if cogeno_spec is None:
        print("Error: can not find cogeno;")
    elif 'scripts/cogeno' in cogeno_spec.submodule_search_locations._path[0]:
        # 3. - use cogeno copy in scripts directory
        from cogeno.cogeno.cogeno import main
        cogeno_path = Path(__file__).resolve().parent.joinpath('cogeno/cogeno')
        print("NOTE: you are running a copy of cogeno from '{}';".
            format(cogeno_path),
            "this may be removed from the Zephpyr repository in the future.",
            "Cogeno is now developed separately.")
    else:
        # 2. - cogeno already installed by pip
        from cogeno.cogeno import main
        print("NOTE: you are running cogeno from a local installation;")

if __name__ == '__main__':
    ret = main()
    sys.exit(ret)

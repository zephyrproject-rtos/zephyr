#!/usr/bin/env python3

'''ARC architecture helper script for zephyr_flash_debug.py

FIXME: This is not portable; a better solution is needed.
       See zephyr_flash_debug.py for details.'''

import os
import subprocess
import sys

os.setsid()
command = sys.argv[1:]
subprocess.call(command)

# Copyright 2018 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Zephyr RTOS meta-tool (west)

Main entry point for running this package as a module, e.g.:

py -3 west      # Windows
python3 -m west # Unix
'''

from .main import main

if __name__ == '__main__':
    main()

# Copyright 2018 Open Source Foundries Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Zephyr RTOS meta-tool (west)
'''

from .main import main
import sys

if __name__ == '__main__':
    main(sys.argv[1:])

#!/usr/bin/env python3
#
# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: Apache-2.0

import sys

from codegen.codegen import CodeGen

if __name__ == '__main__':
    ret = CodeGen().callableMain(sys.argv)
    sys.exit(ret)

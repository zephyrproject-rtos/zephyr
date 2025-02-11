#!/usr/bin/env python3
# Copyright(c) 2022 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

import asyncio
import cavstool

if __name__ == "__main__":
    try:
        asyncio.run(cavstool.main())
    except KeyboardInterrupt:
        start_output = False

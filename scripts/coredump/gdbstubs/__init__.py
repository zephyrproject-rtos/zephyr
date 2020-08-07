#!/usr/bin/env python3
#
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

class TgtCode:
    UNKNOWN = 0

def get_gdbstub(logfile, elffile):
    stub = None

    tgt_code = logfile.log_hdr['tgt_code']

    return stub

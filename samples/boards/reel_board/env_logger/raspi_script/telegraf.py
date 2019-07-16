#!/usr/bin/env python3
#
# Copyright (c) 2019, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
import time
import reelboard


out_temp = 244 # TODO: Get temperature from external sensor
ret = reelboard.bt_exchange(out_temp)
print('example temp=%f,hum=%d %d' % (ret['T_reel'] / 10, ret['H_reel'], int(time.time() * 1000000000)))

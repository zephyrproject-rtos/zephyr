# Copyright (c) 2025, Croxel Inc
# Copyright (c) 2025, CogniPilot Foundation
#
# SPDX-License-Identifier: Apache-2.0

import pyrtcm
import pyubx2
import serial


def enableBaseStationMode(serialEndpoint: serial.Serial):
    frame = pyubx2.UBXMessage.config_set(
        layers=1,
        transaction=0,
        cfgData=[
            ("CFG_MSGOUT_RTCM_3X_TYPE1005_USB", 1),
            ("CFG_MSGOUT_RTCM_3X_TYPE1074_USB", 1),
            ("CFG_MSGOUT_RTCM_3X_TYPE1077_USB", 1),
            ("CFG_MSGOUT_RTCM_3X_TYPE1084_USB", 1),
            ("CFG_MSGOUT_RTCM_3X_TYPE1087_USB", 1),
            ("CFG_MSGOUT_RTCM_3X_TYPE1094_USB", 1),
            ("CFG_MSGOUT_RTCM_3X_TYPE1097_USB", 1),
            ("CFG_MSGOUT_RTCM_3X_TYPE1124_USB", 1),
            ("CFG_MSGOUT_RTCM_3X_TYPE1127_USB", 1),
            ("CFG_MSGOUT_RTCM_3X_TYPE1230_USB", 1),
        ],
    )
    serialEndpoint.write(frame.serialize())

    frame = pyubx2.UBXMessage.config_set(
        layers=1,
        transaction=0,
        cfgData=[
            ("CFG_TMODE_MODE", 1),
            ("CFG_TMODE_SVIN_ACC_LIMIT", 250),
            ("CFG_TMODE_SVIN_MIN_DUR", 60),
        ],
    )
    serialEndpoint.write(frame.serialize())


with (
    serial.Serial("/dev/ttyACM0", 115200) as serialIn,
    serial.Serial("/dev/ttyUSB0", 115200) as serialOut,
):
    enableBaseStationMode(serialIn)

    for raw, parsed in pyrtcm.RTCMReader(serialIn):
        print(parsed)
        serialOut.write(raw)

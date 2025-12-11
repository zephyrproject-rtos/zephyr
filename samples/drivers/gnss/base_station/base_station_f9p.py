# Copyright (c) 2025, Croxel Inc
# Copyright (c) 2025, CogniPilot Foundation
#
# SPDX-License-Identifier: Apache-2.0

import argparse

import pyrtcm
import pyubx2
import serial

UBX_F9P_VID = 0x1546
UBX_F9P_PID = 0x01A9


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument(
        '-p', '--port', required=True, help='Serial Port connected to the Rover (e.g: /dev/ttyUSB0)'
    )

    return parser.parse_args()


def getF9PBaseStationSerialPort():
    from serial.tools import list_ports

    for port in list_ports.comports():
        if port.vid == UBX_F9P_VID and port.pid == UBX_F9P_PID:
            return port.device

    raise RuntimeError(
        f'Could not find any Serial Port with VID: {hex(UBX_F9P_VID)}, PID: {hex(UBX_F9P_PID)}'
    )


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


def main():
    args = parse_args()

    with (
        serial.Serial(getF9PBaseStationSerialPort(), 115200) as serialIn,
        serial.Serial(args.port, 115200) as serialOut,
    ):
        enableBaseStationMode(serialIn)

        for raw, parsed in pyrtcm.RTCMReader(serialIn):
            print(parsed)
            serialOut.write(raw)


if __name__ == '__main__':
    main()

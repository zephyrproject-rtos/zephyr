# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import dataclasses
from collections.abc import Sequence
from dataclasses import field

from bumble.hci import STATUS_SPEC, HCI_Command, metadata


@HCI_Command.command
@dataclasses.dataclass
class HCI_Read_Default_Link_Policy_Settings_Command(HCI_Command):
    '''
    See Bluetooth spec @ 7.2.11 Read Default Link Policy Settings command
    '''

    return_parameters_fields = [
        ('status', STATUS_SPEC),
        ('Default_Link_Policy_Settings', 2),
    ]


@HCI_Command.command
@dataclasses.dataclass
class HCI_Write_Current_Iac_Lap_Command(HCI_Command):
    '''
    See Bluetooth spec @ 7.3.45 Write Current IAC LAP command
    '''

    iac_lap: Sequence[int] = field(metadata=metadata(3, list_begin=True, list_end=True))


@HCI_Command.command
@dataclasses.dataclass
class HCI_Host_Number_Of_Completed_Packets_Command(HCI_Command):
    '''
    See Bluetooth spec @ 7.3.40 Host Number Of Completed Packets command
    '''

    connection_handle: Sequence[int] = field(metadata=metadata(2, list_begin=True, list_end=True))
    num_of_completed_packets: int = field(metadata=metadata(2))

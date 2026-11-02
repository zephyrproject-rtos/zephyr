# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import dataclasses
from collections.abc import Sequence
from dataclasses import field

from bumble.hci import HCI_StatusReturnParameters, HCI_SyncCommand, metadata


@dataclasses.dataclass
class HCI_Read_Default_Link_Policy_Settings_ReturnParameters(HCI_StatusReturnParameters):
    default_link_policy_settings: int = field(metadata=metadata(2))


@HCI_SyncCommand.sync_command(HCI_Read_Default_Link_Policy_Settings_ReturnParameters)
@dataclasses.dataclass
class HCI_Read_Default_Link_Policy_Settings_Command(
    HCI_SyncCommand[HCI_Read_Default_Link_Policy_Settings_ReturnParameters]
):
    '''
    See Bluetooth spec @ 7.2.11 Read Default Link Policy Settings command
    '''


@HCI_SyncCommand.sync_command(HCI_StatusReturnParameters)
@dataclasses.dataclass
class HCI_Write_Current_IAC_LAP_Command(HCI_SyncCommand[HCI_StatusReturnParameters]):
    '''
    See Bluetooth spec @ 7.3.45 Write Current IAC LAP command
    '''

    iac_lap: Sequence[int] = field(metadata=metadata(3, list_begin=True, list_end=True))

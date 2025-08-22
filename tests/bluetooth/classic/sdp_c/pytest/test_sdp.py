# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0

import asyncio
import logging
import sys

from bumble.core import (
    BT_ADVANCED_AUDIO_DISTRIBUTION_SERVICE,
    BT_AUDIO_SINK_SERVICE,
    BT_AUDIO_SOURCE_SERVICE,
    BT_AV_REMOTE_CONTROL_CONTROLLER_SERVICE,
    BT_AV_REMOTE_CONTROL_SERVICE,
    BT_AV_REMOTE_CONTROL_TARGET_SERVICE,
    BT_AVCTP_PROTOCOL_ID,
    BT_AVDTP_PROTOCOL_ID,
    BT_BR_EDR_TRANSPORT,
    BT_GENERIC_AUDIO_SERVICE,
    BT_HANDSFREE_SERVICE,
    BT_L2CAP_PROTOCOL_ID,
    BT_RFCOMM_PROTOCOL_ID,
)
from bumble.device import Device
from bumble.hci import Address, HCI_Write_Page_Timeout_Command
from bumble.l2cap import ClassicChannel, L2CAP_Connection_Response
from bumble.sdp import (
    SDP_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_ID,
    SDP_BROWSE_GROUP_LIST_ATTRIBUTE_ID,
    SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID,
    SDP_PUBLIC_BROWSE_ROOT,
    SDP_SERVICE_CLASS_ID_LIST_ATTRIBUTE_ID,
    SDP_SERVICE_RECORD_HANDLE_ATTRIBUTE_ID,
    SDP_SUPPORTED_FEATURES_ATTRIBUTE_ID,
    DataElement,
    ServiceAttribute,
)
from bumble.snoop import BtSnooper
from bumble.transport import open_transport_or_link
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)

AVDTP_PSM = 0x0019
AVDTP_VERSION = 0x0100

SDP_SERVICE_RECORDS_A2DP = {
    0x00010001: [
        ServiceAttribute(
            SDP_SERVICE_RECORD_HANDLE_ATTRIBUTE_ID,
            DataElement.unsigned_integer_32(0x00010001),
        ),
        ServiceAttribute(
            SDP_BROWSE_GROUP_LIST_ATTRIBUTE_ID,
            DataElement.sequence([DataElement.uuid(SDP_PUBLIC_BROWSE_ROOT)]),
        ),
        ServiceAttribute(
            SDP_SERVICE_CLASS_ID_LIST_ATTRIBUTE_ID,
            DataElement.sequence([DataElement.uuid(BT_AUDIO_SOURCE_SERVICE)]),
        ),
        ServiceAttribute(
            SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_L2CAP_PROTOCOL_ID),
                            DataElement.unsigned_integer_16(AVDTP_PSM),
                        ]
                    ),
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_AVDTP_PROTOCOL_ID),
                            DataElement.unsigned_integer_16(AVDTP_VERSION),
                        ]
                    ),
                ]
            ),
        ),
        ServiceAttribute(
            SDP_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_ADVANCED_AUDIO_DISTRIBUTION_SERVICE),
                            DataElement.unsigned_integer_16(AVDTP_VERSION),
                        ]
                    )
                ]
            ),
        ),
    ]
}

RFCOMM_CHANNEL = 1
HFP_VERSION = 0x0108
HFP_FEATURES = 0x00

SDP_SERVICE_RECORDS_HFP = {
    0x00010002: [
        ServiceAttribute(
            SDP_SERVICE_RECORD_HANDLE_ATTRIBUTE_ID,
            DataElement.unsigned_integer_32(0x00010002),
        ),
        ServiceAttribute(
            SDP_SERVICE_CLASS_ID_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.uuid(BT_HANDSFREE_SERVICE),
                    DataElement.uuid(BT_GENERIC_AUDIO_SERVICE),
                ]
            ),
        ),
        ServiceAttribute(
            SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.sequence([DataElement.uuid(BT_L2CAP_PROTOCOL_ID)]),
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_RFCOMM_PROTOCOL_ID),
                            DataElement.unsigned_integer_8(RFCOMM_CHANNEL),
                        ]
                    ),
                ]
            ),
        ),
        ServiceAttribute(
            SDP_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_HANDSFREE_SERVICE),
                            DataElement.unsigned_integer_16(HFP_VERSION),
                        ]
                    )
                ]
            ),
        ),
        ServiceAttribute(
            SDP_SUPPORTED_FEATURES_ATTRIBUTE_ID,
            DataElement.unsigned_integer_16(HFP_FEATURES),
        ),
    ]
}

SDP_SERVICE_ONE_RECORD = SDP_SERVICE_RECORDS_A2DP

AVCTP_PSM = 0x0017
AVCTP_VERSION = 0x0104
AVRCP_VERSION = 0x0106
AVRCP_TARGET_FEATURE = 0

SDP_SERVICE_RECORDS_AVRCP_TARGET = {
    0x00010003: [
        ServiceAttribute(
            SDP_SERVICE_RECORD_HANDLE_ATTRIBUTE_ID,
            DataElement.unsigned_integer_32(0x00010003),
        ),
        ServiceAttribute(
            SDP_BROWSE_GROUP_LIST_ATTRIBUTE_ID,
            DataElement.sequence([DataElement.uuid(SDP_PUBLIC_BROWSE_ROOT)]),
        ),
        ServiceAttribute(
            SDP_SERVICE_CLASS_ID_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.uuid(BT_AV_REMOTE_CONTROL_TARGET_SERVICE),
                ]
            ),
        ),
        ServiceAttribute(
            SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_L2CAP_PROTOCOL_ID),
                            DataElement.unsigned_integer_16(AVCTP_PSM),
                        ]
                    ),
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_AVCTP_PROTOCOL_ID),
                            DataElement.unsigned_integer_16(AVCTP_VERSION),
                        ]
                    ),
                ]
            ),
        ),
        ServiceAttribute(
            SDP_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_AV_REMOTE_CONTROL_SERVICE),
                            DataElement.unsigned_integer_16(AVRCP_VERSION),
                        ]
                    ),
                ]
            ),
        ),
        ServiceAttribute(
            SDP_SUPPORTED_FEATURES_ATTRIBUTE_ID,
            DataElement.unsigned_integer_16(AVRCP_TARGET_FEATURE),
        ),
    ]
}

AVRCP_CONTROLLER_FEATURE = 0

SDP_SERVICE_RECORDS_AVRCP_CONTROLLER = {
    0x00010004: [
        ServiceAttribute(
            SDP_SERVICE_RECORD_HANDLE_ATTRIBUTE_ID,
            DataElement.unsigned_integer_32(0x00010004),
        ),
        ServiceAttribute(
            SDP_BROWSE_GROUP_LIST_ATTRIBUTE_ID,
            DataElement.sequence([DataElement.uuid(SDP_PUBLIC_BROWSE_ROOT)]),
        ),
        ServiceAttribute(
            SDP_SERVICE_CLASS_ID_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.uuid(BT_AV_REMOTE_CONTROL_SERVICE),
                    DataElement.uuid(BT_AV_REMOTE_CONTROL_CONTROLLER_SERVICE),
                ]
            ),
        ),
        ServiceAttribute(
            SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_L2CAP_PROTOCOL_ID),
                            DataElement.unsigned_integer_16(AVCTP_PSM),
                        ]
                    ),
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_AVCTP_PROTOCOL_ID),
                            DataElement.unsigned_integer_16(AVCTP_VERSION),
                        ]
                    ),
                ]
            ),
        ),
        ServiceAttribute(
            SDP_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_AV_REMOTE_CONTROL_SERVICE),
                            DataElement.unsigned_integer_16(AVRCP_VERSION),
                        ]
                    ),
                ]
            ),
        ),
        ServiceAttribute(
            SDP_SUPPORTED_FEATURES_ATTRIBUTE_ID,
            DataElement.unsigned_integer_16(AVRCP_CONTROLLER_FEATURE),
        ),
    ]
}

SDP_SERVICE_RECORDS_A2DP_SINK = {
    0x00010005: [
        ServiceAttribute(
            SDP_SERVICE_RECORD_HANDLE_ATTRIBUTE_ID,
            DataElement.unsigned_integer_32(0x00010005),
        ),
        ServiceAttribute(
            SDP_BROWSE_GROUP_LIST_ATTRIBUTE_ID,
            DataElement.sequence([DataElement.uuid(SDP_PUBLIC_BROWSE_ROOT)]),
        ),
        ServiceAttribute(
            SDP_SERVICE_CLASS_ID_LIST_ATTRIBUTE_ID,
            DataElement.sequence([DataElement.uuid(BT_AUDIO_SINK_SERVICE)]),
        ),
        ServiceAttribute(
            SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_L2CAP_PROTOCOL_ID),
                            DataElement.unsigned_integer_16(AVDTP_PSM),
                        ]
                    ),
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_AVDTP_PROTOCOL_ID),
                            DataElement.unsigned_integer_16(AVDTP_VERSION),
                        ]
                    ),
                ]
            ),
        ),
        ServiceAttribute(
            SDP_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_ADVANCED_AUDIO_DISTRIBUTION_SERVICE),
                            DataElement.unsigned_integer_16(AVDTP_VERSION),
                        ]
                    )
                ]
            ),
        ),
    ]
}


async def wait_for_shell_response(dut, message):
    found = False
    lines = []
    try:
        while found is False:
            read_lines = dut.readlines()
            for line in read_lines:
                logger.info(f'{str(line)}')
                if message in line:
                    found = True
                    break
            lines = lines + read_lines
            await asyncio.sleep(1)
    except Exception as e:
        logger.error(f'{e}!', exc_info=True)
        raise e
    return found, lines


async def device_power_on(device) -> None:
    while True:
        try:
            await device.power_on()
            break
        except Exception:
            continue


async def sdp_ssa_discover_no_record(hci_port, shell, dut, address) -> None:
    logger.info('<<< SDP Discovery ...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        # No SDP Record
        # device.sdp_service_records = SDP_SERVICE_RECORDS
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            target_address = address.split(" ")[0]
            logger.info(f'=== Connecting to {target_address}...')
            try:
                connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
                logger.info(f'=== Connected to {connection.peer_address}!')
            except Exception as e:
                logger.error(f'Fail to connect to {target_address}!')
                raise e

            # Discover SDP Record
            shell.exec_command(f"sdp_client ssa_discovery {BT_L2CAP_PROTOCOL_ID.to_hex_str()}")
            found, lines = await wait_for_shell_response(dut, "No SDP Record")
            logger.info(f'{lines}')
            assert found is True


async def sdp_ssa_discover_one_record(hci_port, shell, dut, address) -> None:
    logger.info('<<< SDP Discovery ...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.sdp_service_records = SDP_SERVICE_ONE_RECORD
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            target_address = address.split(" ")[0]
            logger.info(f'=== Connecting to {target_address}...')
            try:
                connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
                logger.info(f'=== Connected to {connection.peer_address}!')
            except Exception as e:
                logger.error(f'Fail to connect to {target_address}!')
                raise e

            # Discover SDP Record
            shell.exec_command(f"sdp_client ssa_discovery {BT_L2CAP_PROTOCOL_ID.to_hex_str()}")
            found, lines = await wait_for_shell_response(dut, "SDP Discovery Done")
            assert found is True

            found = False
            for line in lines:
                if "PROTOCOL:" in line and "L2CAP" in line.split(':')[1]:
                    assert int(line.split(':')[2]) == AVDTP_PSM
                    found = True

            logger.info(f'Rsp: PROTOCOL: L2CAP: {AVDTP_PSM} is found? {found}')
            assert found is True

            version = BT_ADVANCED_AUDIO_DISTRIBUTION_SERVICE.to_hex_str()

            found = False
            for line in lines:
                if "VERSION:" in line:
                    # logger.info(f"{version}")
                    assert int(line.split(':')[1], base=16) == int(version, base=16)
                    assert int(line.split(':')[2]) == AVDTP_VERSION
                    found = True

            logger.info(f'Rsp: VER: {version}: {AVDTP_VERSION} is found? {found}')
            assert found is True


SDP_SERVICE_TWO_RECORDS = {**SDP_SERVICE_RECORDS_A2DP, **SDP_SERVICE_RECORDS_HFP}


async def sdp_ssa_discover_two_records(hci_port, shell, dut, address) -> None:
    logger.info('<<< SDP Discovery ...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.sdp_service_records = SDP_SERVICE_TWO_RECORDS
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            target_address = address.split(" ")[0]
            logger.info(f'=== Connecting to {target_address}...')
            try:
                connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
                logger.info(f'=== Connected to {connection.peer_address}!')
            except Exception as e:
                logger.error(f'Fail to connect to {target_address}!')
                raise e

            # Discover SDP Record
            shell.exec_command(f"sdp_client ssa_discovery {BT_L2CAP_PROTOCOL_ID.to_hex_str()}")
            found, lines = await wait_for_shell_response(dut, "SDP Discovery Done")
            assert found is True

            found = False
            for line in lines:
                if "PROTOCOL:" in line and "RFCOMM" in line.split(':')[1]:
                    assert int(line.split(':')[2]) == RFCOMM_CHANNEL
                    found = True

            logger.info(f'Rsp: PROTOCOL: RFCOMM: {RFCOMM_CHANNEL} is found? {found}')
            assert found is True

            found = False
            for line in lines:
                if "VERSION:" in line and int(line.split(':')[1], base=16) == int(
                    BT_HANDSFREE_SERVICE.to_hex_str(), base=16
                ):
                    assert int(line.split(':')[2]) == HFP_VERSION
                    found = True

            logger.info(
                f'Rsp: VER: {BT_HANDSFREE_SERVICE.to_hex_str()}: {HFP_VERSION} is found? {found}'
            )
            assert found is True

            found = False
            for line in lines:
                if "FEATURE:" in line:
                    assert int(line.split(':')[1], base=16) == HFP_FEATURES
                    found = True

            logger.info(f'Rsp: FEATURE: {HFP_FEATURES} is found? {found}')
            assert found is True


SDP_SERVICE_MULTIPLE_RECORDS = {
    **SDP_SERVICE_RECORDS_A2DP,
    **SDP_SERVICE_RECORDS_HFP,
    **SDP_SERVICE_RECORDS_AVRCP_TARGET,
}


async def sdp_ssa_discover_multiple_records(hci_port, shell, dut, address) -> None:
    logger.info('<<< SDP Discovery ...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.sdp_service_records = SDP_SERVICE_MULTIPLE_RECORDS
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            target_address = address.split(" ")[0]
            logger.info(f'=== Connecting to {target_address}...')
            try:
                connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
                logger.info(f'=== Connected to {connection.peer_address}!')
            except Exception as e:
                logger.error(f'Fail to connect to {target_address}!')
                raise e

            # Discover SDP Record
            shell.exec_command(f"sdp_client ssa_discovery {BT_L2CAP_PROTOCOL_ID.to_hex_str()}")
            found, lines = await wait_for_shell_response(dut, "SDP Discovery Done")
            logger.info(f'{lines}')
            assert found is True


async def sdp_ssa_discover_multiple_records_with_range(hci_port, shell, dut, address) -> None:
    logger.info('<<< SDP Discovery ...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.sdp_service_records = SDP_SERVICE_MULTIPLE_RECORDS
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            target_address = address.split(" ")[0]
            logger.info(f'=== Connecting to {target_address}...')
            try:
                connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
                logger.info(f'=== Connected to {connection.peer_address}!')
            except Exception as e:
                logger.error(f'Fail to connect to {target_address}!')
                raise e

            # Discover SDP Record with range SDP_SERVICE_RECORD_HANDLE_ATTRIBUTE_ID ~
            # SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID
            shell.exec_command(
                f"sdp_client ssa_discovery {BT_L2CAP_PROTOCOL_ID.to_hex_str()} "
                f"{SDP_SERVICE_RECORD_HANDLE_ATTRIBUTE_ID} "
                f"{SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID}"
            )
            found, lines = await wait_for_shell_response(dut, "SDP Discovery Done")
            logger.info(f'{lines}')
            assert found is True

            # Discover SDP Record with range SDP_SUPPORTED_FEATURES_ATTRIBUTE_ID ~
            # SDP_SUPPORTED_FEATURES_ATTRIBUTE_ID
            shell.exec_command(
                f"sdp_client ssa_discovery {BT_L2CAP_PROTOCOL_ID.to_hex_str()} "
                f"{SDP_SUPPORTED_FEATURES_ATTRIBUTE_ID} "
                f"{SDP_SUPPORTED_FEATURES_ATTRIBUTE_ID}"
            )
            found, lines = await wait_for_shell_response(dut, "SDP Discovery Done")
            logger.info(f'{lines}')
            assert found is True

            # Discover SDP Record with range SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID ~
            # 0xffff
            shell.exec_command(
                f"sdp_client ssa_discovery {BT_L2CAP_PROTOCOL_ID.to_hex_str()} "
                f"{SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID} 0xffff"
            )
            found, lines = await wait_for_shell_response(dut, "SDP Discovery Done")
            logger.info(f'{lines}')
            assert found is True

            # Discover SDP Record with range 0xff00 ~ 0xffff
            shell.exec_command(
                f"sdp_client ssa_discovery {BT_L2CAP_PROTOCOL_ID.to_hex_str()} 0xff00 0xffff"
            )
            found, lines = await wait_for_shell_response(dut, "No SDP Record")
            logger.info(f'{lines}')
            assert found is True


async def sdp_ss_discover_no_record(hci_port, shell, dut, address) -> None:
    logger.info('<<< SDP Discovery ...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        # No SDP Record
        # device.sdp_service_records = SDP_SERVICE_RECORDS
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            target_address = address.split(" ")[0]
            logger.info(f'=== Connecting to {target_address}...')
            try:
                connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
                logger.info(f'=== Connected to {connection.peer_address}!')
            except Exception as e:
                logger.error(f'Fail to connect to {target_address}!')
                raise e

            # Discover SDP Record
            shell.exec_command(f"sdp_client ss_discovery {BT_L2CAP_PROTOCOL_ID.to_hex_str()}")
            found, lines = await wait_for_shell_response(dut, "No SDP Record")
            logger.info(f'{lines}')
            assert found is True


async def sdp_ss_discover_one_record(hci_port, shell, dut, address) -> None:
    logger.info('<<< SDP Discovery ...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.sdp_service_records = SDP_SERVICE_ONE_RECORD
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            target_address = address.split(" ")[0]
            logger.info(f'=== Connecting to {target_address}...')
            try:
                connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
                logger.info(f'=== Connected to {connection.peer_address}!')
            except Exception as e:
                logger.error(f'Fail to connect to {target_address}!')
                raise e

            # Discover SDP Record
            shell.exec_command(f"sdp_client ss_discovery {BT_L2CAP_PROTOCOL_ID.to_hex_str()}")
            found, lines = await wait_for_shell_response(dut, "SDP Discovery Done")
            logger.info(f'{lines}')
            assert found is True

            count = 0
            for line in lines:
                if "RAW:" in line and device.sdp_service_records.get(
                    int(line.split(':')[1], base=16)
                ):
                    count = count + 1

            logger.info(f'Rsp: Service Record Handle count is {count}')
            assert count == 1


async def sdp_ss_discover_two_records(hci_port, shell, dut, address) -> None:
    logger.info('<<< SDP Discovery ...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.sdp_service_records = SDP_SERVICE_TWO_RECORDS
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            target_address = address.split(" ")[0]
            logger.info(f'=== Connecting to {target_address}...')
            try:
                connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
                logger.info(f'=== Connected to {connection.peer_address}!')
            except Exception as e:
                logger.error(f'Fail to connect to {target_address}!')
                raise e

            # Discover SDP Record
            shell.exec_command(f"sdp_client ss_discovery {BT_L2CAP_PROTOCOL_ID.to_hex_str()}")
            found, lines = await wait_for_shell_response(dut, "SDP Discovery Done")
            logger.info(f'{lines}')
            assert found is True

            count = 0
            for line in lines:
                if "RAW:" in line and device.sdp_service_records.get(
                    int(line.split(':')[1], base=16)
                ):
                    count = count + 1

            logger.info(f'Rsp: Service Record Handle count is {count}')
            assert count == 2


async def sdp_ss_discover_multiple_records(hci_port, shell, dut, address) -> None:
    logger.info('<<< SDP Discovery ...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.sdp_service_records = SDP_SERVICE_MULTIPLE_RECORDS
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            target_address = address.split(" ")[0]
            logger.info(f'=== Connecting to {target_address}...')
            try:
                connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
                logger.info(f'=== Connected to {connection.peer_address}!')
            except Exception as e:
                logger.error(f'Fail to connect to {target_address}!')
                raise e

            # Discover SDP Record
            shell.exec_command(f"sdp_client ss_discovery {BT_L2CAP_PROTOCOL_ID.to_hex_str()}")
            found, lines = await wait_for_shell_response(dut, "SDP Discovery Done")
            logger.info(f'{lines}')
            assert found is True

            count = 0
            for line in lines:
                if "RAW:" in line and device.sdp_service_records.get(
                    int(line.split(':')[1], base=16)
                ):
                    count = count + 1

            logger.info(f'Rsp: Service Record Handle count is {count}')
            assert count == 3


async def sdp_sa_discover_no_record(hci_port, shell, dut, address) -> None:
    logger.info('<<< SDP Discovery ...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        # No SDP Record
        # device.sdp_service_records = SDP_SERVICE_RECORDS
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            target_address = address.split(" ")[0]
            logger.info(f'=== Connecting to {target_address}...')
            try:
                connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
                logger.info(f'=== Connected to {connection.peer_address}!')
            except Exception as e:
                logger.error(f'Fail to connect to {target_address}!')
                raise e

            # Discover SDP Record
            shell.exec_command("sdp_client sa_discovery 00010001")
            found, lines = await wait_for_shell_response(dut, "No SDP Record")
            logger.info(f'{lines}')
            assert found is True


async def sdp_sa_discover_one_record(hci_port, shell, dut, address) -> None:
    logger.info('<<< SDP Discovery ...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.sdp_service_records = SDP_SERVICE_ONE_RECORD
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            target_address = address.split(" ")[0]
            logger.info(f'=== Connecting to {target_address}...')
            try:
                connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
                logger.info(f'=== Connected to {connection.peer_address}!')
            except Exception as e:
                logger.error(f'Fail to connect to {target_address}!')
                raise e

            # Discover SDP Record
            shell.exec_command("sdp_client sa_discovery 00010001")
            found, lines = await wait_for_shell_response(dut, "SDP Discovery Done")
            assert found is True

            found = False
            for line in lines:
                if "PROTOCOL:" in line and "L2CAP" in line.split(':')[1]:
                    assert int(line.split(':')[2]) == AVDTP_PSM
                    found = True

            logger.info(f'Rsp: PROTOCOL: L2CAP: {AVDTP_PSM} is found? {found}')
            assert found is True

            version = BT_ADVANCED_AUDIO_DISTRIBUTION_SERVICE.to_hex_str()

            found = False
            for line in lines:
                if "VERSION:" in line:
                    # logger.info(f"{version}")
                    assert int(line.split(':')[1], base=16) == int(version, base=16)
                    assert int(line.split(':')[2]) == AVDTP_VERSION
                    found = True

            logger.info(f'Rsp: VER: {version}: {AVDTP_VERSION} is found? {found}')
            assert found is True


async def sdp_sa_discover_two_records(hci_port, shell, dut, address) -> None:
    logger.info('<<< SDP Discovery ...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.sdp_service_records = SDP_SERVICE_TWO_RECORDS
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            target_address = address.split(" ")[0]
            logger.info(f'=== Connecting to {target_address}...')
            try:
                connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
                logger.info(f'=== Connected to {connection.peer_address}!')
            except Exception as e:
                logger.error(f'Fail to connect to {target_address}!')
                raise e

            # Discover SDP Record
            shell.exec_command("sdp_client sa_discovery 00010002")
            found, lines = await wait_for_shell_response(dut, "SDP Discovery Done")
            assert found is True

            found = False
            for line in lines:
                if "PROTOCOL:" in line and "RFCOMM" in line.split(':')[1]:
                    assert int(line.split(':')[2]) == RFCOMM_CHANNEL
                    found = True

            logger.info(f'Rsp: PROTOCOL: RFCOMM: {RFCOMM_CHANNEL} is found? {found}')
            assert found is True

            found = False
            for line in lines:
                if "VERSION:" in line and int(line.split(':')[1], base=16) == int(
                    BT_HANDSFREE_SERVICE.to_hex_str(), base=16
                ):
                    assert int(line.split(':')[2]) == HFP_VERSION
                    found = True

            logger.info(
                f'Rsp: VER: {BT_HANDSFREE_SERVICE.to_hex_str()}: {HFP_VERSION} is found? {found}'
            )
            assert found is True

            found = False
            for line in lines:
                if "FEATURE:" in line:
                    assert int(line.split(':')[1], base=16) == HFP_FEATURES
                    found = True

            logger.info(f'Rsp: FEATURE: {HFP_FEATURES} is found? {found}')
            assert found is True


async def sdp_sa_discover_multiple_records(hci_port, shell, dut, address) -> None:
    logger.info('<<< SDP Discovery ...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.sdp_service_records = SDP_SERVICE_MULTIPLE_RECORDS
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            target_address = address.split(" ")[0]
            logger.info(f'=== Connecting to {target_address}...')
            try:
                connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
                logger.info(f'=== Connected to {connection.peer_address}!')
            except Exception as e:
                logger.error(f'Fail to connect to {target_address}!')
                raise e

            # Discover SDP Record
            shell.exec_command("sdp_client sa_discovery 00010003")
            found, lines = await wait_for_shell_response(dut, "SDP Discovery Done")
            logger.info(f'{lines}')
            assert found is True


async def sdp_sa_discover_multiple_records_with_range(hci_port, shell, dut, address) -> None:
    logger.info('<<< SDP Discovery ...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.sdp_service_records = SDP_SERVICE_MULTIPLE_RECORDS
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            target_address = address.split(" ")[0]
            logger.info(f'=== Connecting to {target_address}...')
            try:
                connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
                logger.info(f'=== Connected to {connection.peer_address}!')
            except Exception as e:
                logger.error(f'Fail to connect to {target_address}!')
                raise e

            # Discover SDP Record with range SDP_SERVICE_RECORD_HANDLE_ATTRIBUTE_ID ~
            # SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID
            shell.exec_command(
                f"sdp_client sa_discovery 00010003 {SDP_SERVICE_RECORD_HANDLE_ATTRIBUTE_ID} "
                f"{SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID}"
            )
            found, lines = await wait_for_shell_response(dut, "SDP Discovery Done")
            logger.info(f'{lines}')
            assert found is True

            # Discover SDP Record with range SDP_SUPPORTED_FEATURES_ATTRIBUTE_ID ~
            # SDP_SUPPORTED_FEATURES_ATTRIBUTE_ID
            shell.exec_command(
                f"sdp_client sa_discovery 00010003 {SDP_SUPPORTED_FEATURES_ATTRIBUTE_ID} "
                f"{SDP_SUPPORTED_FEATURES_ATTRIBUTE_ID}"
            )
            found, lines = await wait_for_shell_response(dut, "SDP Discovery Done")
            logger.info(f'{lines}')
            assert found is True

            # Discover SDP Record with range SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID ~
            # 0xffff
            shell.exec_command(
                "sdp_client sa_discovery 00010003 "
                f"{SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID} 0xffff"
            )
            found, lines = await wait_for_shell_response(dut, "SDP Discovery Done")
            logger.info(f'{lines}')
            assert found is True

            # Discover SDP Record with range 0xff00 ~ 0xffff
            shell.exec_command("sdp_client sa_discovery 00010003 0xff00 0xffff")
            found, lines = await wait_for_shell_response(dut, "No SDP Record")
            logger.info(f'{lines}')
            assert found is True


async def sdp_ssa_discover_fail(hci_port, shell, dut, address) -> None:
    def on_app_connection_request(self, request) -> None:
        logger.info('Force L2CAP connection failure')
        self.destination_cid = request.source_cid
        self.send_control_frame(
            L2CAP_Connection_Response(
                identifier=request.identifier,
                destination_cid=self.source_cid,
                source_cid=self.destination_cid,
                result=L2CAP_Connection_Response.CONNECTION_REFUSED_NO_RESOURCES_AVAILABLE,
                status=0x0000,
            )
        )

    # Save the origin method
    on_connection_request = ClassicChannel.on_connection_request
    # Replace the origin method with a new one to force L2CAP connection failure
    ClassicChannel.on_connection_request = on_app_connection_request

    logger.info('<<< SDP Discovery ...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            target_address = address.split(" ")[0]
            logger.info(f'=== Connecting to {target_address}...')
            try:
                connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
                logger.info(f'=== Connected to {connection.peer_address}!')
            except Exception as e:
                logger.error(f'Fail to connect to {target_address}!')
                raise e

            # Discover SDP Record
            shell.exec_command("sdp_client ssa_discovery_fail")
            found, lines = await wait_for_shell_response(dut, "test pass")
            logger.info(f'{lines}')
            assert found is True

    # Restore the origin method
    ClassicChannel.on_connection_request = on_connection_request


class TestSdpServer:
    def test_sdp_ssa_discover_no_record(self, shell: Shell, dut: DeviceAdapter, sdp_client_dut):
        """Test case to request SDP records. No SDP record registered."""
        logger.info(f'test_sdp_ssa_discover_no_record {sdp_client_dut}')
        hci, iut_address = sdp_client_dut
        asyncio.run(sdp_ssa_discover_no_record(hci, shell, dut, iut_address))

    def test_sdp_ssa_discover_one_record(self, shell: Shell, dut: DeviceAdapter, sdp_client_dut):
        """Test case to request SDP records. One SDP record registered."""
        logger.info(f'test_sdp_ssa_discover_one_record {sdp_client_dut}')
        hci, iut_address = sdp_client_dut
        asyncio.run(sdp_ssa_discover_one_record(hci, shell, dut, iut_address))

    def test_sdp_ssa_discover_two_records(self, shell: Shell, dut: DeviceAdapter, sdp_client_dut):
        """Test case to request SDP records. Two SDP record registered."""
        logger.info(f'test_sdp_ssa_discover_two_records {sdp_client_dut}')
        hci, iut_address = sdp_client_dut
        asyncio.run(sdp_ssa_discover_two_records(hci, shell, dut, iut_address))

    def test_sdp_ssa_discover_multiple_records(
        self, shell: Shell, dut: DeviceAdapter, sdp_client_dut
    ):
        """Test case to request SDP records. Multiple SDP record registered."""
        logger.info(f'test_sdp_ssa_discover_multiple_records {sdp_client_dut}')
        hci, iut_address = sdp_client_dut
        asyncio.run(sdp_ssa_discover_multiple_records(hci, shell, dut, iut_address))

    def test_sdp_ssa_discover_multiple_records_with_range(
        self, shell: Shell, dut: DeviceAdapter, sdp_client_dut
    ):
        """Test case to request SDP records with range. Multiple SDP record registered."""
        logger.info(f'test_sdp_ssa_discover_multiple_records_with_range {sdp_client_dut}')
        hci, iut_address = sdp_client_dut
        asyncio.run(sdp_ssa_discover_multiple_records_with_range(hci, shell, dut, iut_address))

    def test_sdp_ss_discover_no_record(self, shell: Shell, dut: DeviceAdapter, sdp_client_dut):
        """Test case to request SDP records. No SDP record registered."""
        logger.info(f'test_sdp_ss_discover_no_record {sdp_client_dut}')
        hci, iut_address = sdp_client_dut
        asyncio.run(sdp_ss_discover_no_record(hci, shell, dut, iut_address))

    def test_sdp_ss_discover_one_record(self, shell: Shell, dut: DeviceAdapter, sdp_client_dut):
        """Test case to request SDP records. One SDP record registered."""
        logger.info(f'test_sdp_ss_discover_one_record {sdp_client_dut}')
        hci, iut_address = sdp_client_dut
        asyncio.run(sdp_ss_discover_one_record(hci, shell, dut, iut_address))

    def test_sdp_ss_discover_two_records(self, shell: Shell, dut: DeviceAdapter, sdp_client_dut):
        """Test case to request SDP records. Two SDP record registered."""
        logger.info(f'test_sdp_ss_discover_two_records {sdp_client_dut}')
        hci, iut_address = sdp_client_dut
        asyncio.run(sdp_ss_discover_two_records(hci, shell, dut, iut_address))

    def test_sdp_ss_discover_multiple_records(
        self, shell: Shell, dut: DeviceAdapter, sdp_client_dut
    ):
        """Test case to request SDP records. Multiple SDP record registered."""
        logger.info(f'test_sdp_ss_discover_multiple_records {sdp_client_dut}')
        hci, iut_address = sdp_client_dut
        asyncio.run(sdp_ss_discover_multiple_records(hci, shell, dut, iut_address))

    def test_sdp_sa_discover_no_record(self, shell: Shell, dut: DeviceAdapter, sdp_client_dut):
        """Test case to request SDP records. No SDP record registered."""
        logger.info(f'test_sdp_sa_discover_no_record {sdp_client_dut}')
        hci, iut_address = sdp_client_dut
        asyncio.run(sdp_sa_discover_no_record(hci, shell, dut, iut_address))

    def test_sdp_sa_discover_one_record(self, shell: Shell, dut: DeviceAdapter, sdp_client_dut):
        """Test case to request SDP records. One SDP record registered."""
        logger.info(f'test_sdp_sa_discover_one_record {sdp_client_dut}')
        hci, iut_address = sdp_client_dut
        asyncio.run(sdp_sa_discover_one_record(hci, shell, dut, iut_address))

    def test_sdp_sa_discover_two_records(self, shell: Shell, dut: DeviceAdapter, sdp_client_dut):
        """Test case to request SDP records. Two SDP record registered."""
        logger.info(f'test_sdp_sa_discover_two_records {sdp_client_dut}')
        hci, iut_address = sdp_client_dut
        asyncio.run(sdp_sa_discover_two_records(hci, shell, dut, iut_address))

    def test_sdp_sa_discover_multiple_records(
        self, shell: Shell, dut: DeviceAdapter, sdp_client_dut
    ):
        """Test case to request SDP records. Multiple SDP record registered."""
        logger.info(f'test_sdp_sa_discover_multiple_records {sdp_client_dut}')
        hci, iut_address = sdp_client_dut
        asyncio.run(sdp_sa_discover_multiple_records(hci, shell, dut, iut_address))

    def test_sdp_sa_discover_multiple_records_with_range(
        self, shell: Shell, dut: DeviceAdapter, sdp_client_dut
    ):
        """Test case to request SDP records with range. Multiple SDP record registered."""
        logger.info(f'test_sdp_sa_discover_multiple_records_with_range {sdp_client_dut}')
        hci, iut_address = sdp_client_dut
        asyncio.run(sdp_sa_discover_multiple_records_with_range(hci, shell, dut, iut_address))

    def test_sdp_ssa_discover_fail(self, shell: Shell, dut: DeviceAdapter, sdp_client_dut):
        """Test case to request SDP records. but the L2CAP connecting fail."""
        logger.info(f'test_sdp_ssa_discover_fail {sdp_client_dut}')
        hci, iut_address = sdp_client_dut
        asyncio.run(sdp_ssa_discover_fail(hci, shell, dut, iut_address))

# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0

import asyncio
import logging

from bumble.core import (
    BT_BR_EDR_TRANSPORT,
    BT_L2CAP_PROTOCOL_ID,
    BT_OBEX_PROTOCOL_ID,
    CommandTimeoutError,
    DeviceClass,
)
from bumble.device import Device
from bumble.hci import Address
from bumble.sdp import SDP_ALL_ATTRIBUTES_RANGE, SDP_PUBLIC_BROWSE_ROOT
from bumble.sdp import Client as SDP_Client
from bumble.transport import open_transport_or_link
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)


async def device_power_on(device) -> None:
    while True:
        try:
            await device.power_on()
            break
        except Exception:
            continue


class discovery_listener(Device.Listener):
    def __init__(self, address: str, event, **kwargs):
        self._address = address
        self._event = event
        super().__init__(**kwargs)

    def on_inquiry_result(self, address, class_of_device, data, rssi):
        (
            service_classes,
            major_device_class,
            minor_device_class,
        ) = DeviceClass.split_class_of_device(class_of_device)
        if self._address:
            _address = self._address.split(" ")
            if _address[0] in str(address):
                logger.info('Target device is found')
                self._event.set()
        separator = '\n  '
        logger.info(f'>>> {address}:')
        logger.info(f'  Device Class (raw): {class_of_device:06X}')
        major_class_name = DeviceClass.major_device_class_name(major_device_class)
        logger.info('  Device Major Class: ' f'{major_class_name}')
        minor_class_name = DeviceClass.minor_device_class_name(
            major_device_class, minor_device_class
        )
        logger.info('  Device Minor Class: ' f'{minor_class_name}')
        logger.info(
            '  Device Services: ' f'{", ".join(DeviceClass.service_class_labels(service_classes))}'
        )
        logger.info(f'  RSSI: {rssi}')
        if data.ad_structures:
            logger.info(f'  {data.to_string(separator)}')


async def start_discovery(hci_port, address) -> None:
    logger.info('<<< Start discovery...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        event = asyncio.Event()
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.listener = discovery_listener(address, event)
        await device_power_on(device)

        logger.info('Starting discovery')
        await device.start_discovery()
        await event.wait()


async def br_connect(hci_port, shell, address) -> None:
    logger.info('<<< connect...')
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        await device_power_on(device)

        target_address = address.split(" ")[0]
        logger.info(f'=== Connecting to {target_address}...')
        try:
            connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
            logger.info(f'=== Connected to {connection.peer_address}!')
        except CommandTimeoutError as e:
            logger.info('!!! Connection timed out')
            raise e

        # Connect to the SDP Server
        sdp_client = SDP_Client(connection)
        await sdp_client.connect()

        # List all services in the root browse group
        logger.info("<<< 1 List all services in the root browse group")
        service_record_handles = await sdp_client.search_services([SDP_PUBLIC_BROWSE_ROOT])
        logger.info(f'SERVICES: {service_record_handles}')

        # For each service in the root browse group, get all its attributes
        assert len(service_record_handles) == 0
        for service_record_handle in service_record_handles:
            attributes = await sdp_client.get_attributes(
                service_record_handle, [SDP_ALL_ATTRIBUTES_RANGE]
            )
            logger.info(f'SERVICE {service_record_handle:04X} attributes:')
            for attribute in attributes:
                logger.info(f'  {attribute}')

        # Install SDP Record 0
        shell.exec_command("sdp_server register_sdp 0")

        # List all services in the root browse group
        logger.info("<<< 2 List all services in the root browse group")
        service_record_handles = await sdp_client.search_services([SDP_PUBLIC_BROWSE_ROOT])
        logger.info(f'SERVICES: {service_record_handles}')

        # For each service in the root browse group, get all its attributes
        assert len(service_record_handles) != 0
        for service_record_handle in service_record_handles:
            attributes = await sdp_client.get_attributes(
                service_record_handle, [SDP_ALL_ATTRIBUTES_RANGE]
            )
            logger.info(f'SERVICE {service_record_handle:04X} attributes:')
            for attribute in attributes:
                logger.info(f'  {attribute}')

        logger.info("<<< 3 List all attributes with L2CAP protocol")
        search_result = await sdp_client.search_attributes(
            [BT_L2CAP_PROTOCOL_ID], [SDP_ALL_ATTRIBUTES_RANGE]
        )
        assert len(search_result) != 0
        logger.info('SEARCH RESULTS:')
        for attribute_list in search_result:
            logger.info('SERVICE:')
            logger.info('  ' + '\n  '.join([attribute.to_string() for attribute in attribute_list]))

        logger.info("<<< 4 List all attributes with OBEX protocol")
        search_result = await sdp_client.search_attributes(
            [BT_OBEX_PROTOCOL_ID], [SDP_ALL_ATTRIBUTES_RANGE]
        )
        logger.info(f'SEARCH RESULTS:{search_result}')
        assert len(search_result) == 0

        logger.info("<<< 5 List all attributes with L2CAP protocol (0x1000~0xFFFF)")
        search_result = await sdp_client.search_attributes(
            [BT_L2CAP_PROTOCOL_ID], [(0x1000, 0xFFFF)]
        )
        logger.info(f'SEARCH RESULTS:{search_result}')
        assert len(search_result) == 0

        # Install SDP Record 1
        shell.exec_command("sdp_server register_sdp 1")

        # List all services with L2CAP
        logger.info("<<< 6 List all services with L2CAP protocol")
        service_record_handles = await sdp_client.search_services([BT_L2CAP_PROTOCOL_ID])
        logger.info(f'SERVICES: {service_record_handles}')

        assert len(service_record_handles) != 0
        # For each service in the root browse group, get all its attributes
        for service_record_handle in service_record_handles:
            attributes = await sdp_client.get_attributes(
                service_record_handle, [SDP_ALL_ATTRIBUTES_RANGE]
            )
            logger.info(f'SERVICE {service_record_handle:04X} attributes:')
            for attribute in attributes:
                logger.info(f'  {attribute}')

        # Install SDP Record 2
        shell.exec_command("sdp_server register_sdp 2")

        # List all services with L2CAP
        logger.info("<<< 7 List all services with L2CAP protocol")
        service_record_handles = await sdp_client.search_services([BT_L2CAP_PROTOCOL_ID])
        logger.info(f'SERVICES: {service_record_handles}')

        assert len(service_record_handles) != 0
        # For each service in the root browse group, get all its attributes
        for service_record_handle in service_record_handles:
            attributes = await sdp_client.get_attributes(
                service_record_handle, [SDP_ALL_ATTRIBUTES_RANGE]
            )
            logger.info(f'SERVICE {service_record_handle:04X} attributes:')
            for attribute in attributes:
                logger.info(f'  {attribute}')

        # Install SDP Record all
        shell.exec_command("sdp_server register_sdp_all")

        # List all services with L2CAP
        logger.info("<<< 8 List all services with L2CAP protocol")
        service_record_handles = await sdp_client.search_services([BT_L2CAP_PROTOCOL_ID])
        logger.info(f'SERVICES: {service_record_handles}')

        assert len(service_record_handles) != 0
        # For each service in the root browse group, get all its attributes
        for service_record_handle in service_record_handles:
            attributes = await sdp_client.get_attributes(
                service_record_handle, [SDP_ALL_ATTRIBUTES_RANGE]
            )
            logger.info(f'SERVICE {service_record_handle:04X} attributes:')
            for attribute in attributes:
                logger.info(f'  {attribute}')

        logger.info("<<< 9 List all attributes with L2CAP protocol")
        search_result = await sdp_client.search_attributes(
            [BT_L2CAP_PROTOCOL_ID], [SDP_ALL_ATTRIBUTES_RANGE]
        )
        assert len(search_result) != 0
        logger.info('SEARCH RESULTS:')
        for attribute_list in search_result:
            logger.info('SERVICE:')
            logger.info('  ' + '\n  '.join([attribute.to_string() for attribute in attribute_list]))

        # Install SDP large Record
        shell.exec_command("sdp_server register_sdp_large")

        logger.info("<<< 10 List all services with L2CAP protocol")
        service_record_handles = await sdp_client.search_services([BT_L2CAP_PROTOCOL_ID])
        logger.info(f'SERVICES: {service_record_handles}')

        assert len(service_record_handles) != 0
        # For each service in the root browse group, get all its attributes
        for service_record_handle in service_record_handles:
            attributes = await sdp_client.get_attributes(
                service_record_handle, [SDP_ALL_ATTRIBUTES_RANGE]
            )
            logger.info(f'SERVICE {service_record_handle:04X} attributes:')
            service_name_found = False
            for attribute in attributes:
                logger.info(f'  {attribute}')
                if attribute.id == 0x100:
                    service_name_found = True
            if service_record_handle == service_record_handles[-1]:
                assert service_name_found is False

        # Install SDP large Record
        shell.exec_command("sdp_server register_sdp_large_valid")

        logger.info("<<< 11 List all services with L2CAP protocol with valid service name")
        service_record_handles = await sdp_client.search_services([BT_L2CAP_PROTOCOL_ID])
        logger.info(f'SERVICES: {service_record_handles}')

        assert len(service_record_handles) != 0
        # For each service in the root browse group, get all its attributes
        for service_record_handle in service_record_handles:
            attributes = await sdp_client.get_attributes(
                service_record_handle, [SDP_ALL_ATTRIBUTES_RANGE]
            )
            logger.info(f'SERVICE {service_record_handle:04X} attributes:')
            service_name_found = False
            for attribute in attributes:
                logger.info(f'  {attribute}')
                if attribute.id == 0x100:
                    service_name_found = True
            if service_record_handle == service_record_handles[-1]:
                assert service_name_found is True


class TestSdpServer:
    def test_discovery_device(self, sdp_server_dut):
        """Test case to discover IUT"""
        logger.info(f'test_discovery_device {sdp_server_dut}')
        hci, iut_address = sdp_server_dut
        asyncio.run(start_discovery(hci, iut_address))

    def test_sdp_discover(self, shell: Shell, dut: DeviceAdapter, sdp_server_dut):
        """Test case to request SDP records"""
        logger.info(f'test_sdp_discover {sdp_server_dut}')
        hci, iut_address = sdp_server_dut
        asyncio.run(br_connect(hci, shell, iut_address))

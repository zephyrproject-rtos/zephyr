# Copyright 2026 Xiaomi Corporation
#
# SPDX-License-Identifier: Apache-2.0

import asyncio
import logging
import sys

from bumble.core import (
    BT_BR_EDR_TRANSPORT,
)
from bumble.device import Device
from bumble.hci import (
    HCI_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES_ERROR,
    Address,
    HCI_Write_Page_Timeout_Command,
)
from bumble.hid import Host as HIDHost
from bumble.hid import Message
from bumble.pairing import PairingConfig, PairingDelegate
from bumble.snoop import BtSnooper
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


async def wait_for_shell_response(dut, message, max_time=10):
    found = False
    lines = []
    try:
        for _ in range(0, max_time):
            if found is False:
                read_lines = dut.readlines()
                logger.info(f"{read_lines}")
                for line in read_lines:
                    if message in line:
                        found = True
                        break
                lines = lines + read_lines
                await asyncio.sleep(1)
    except Exception as e:
        logger.error(f'{e}!', exc_info=True)
        raise e
    return found, lines


async def send_cmd_to_iut(shell, dut, cmd, parse=None, max_time=10):
    found = False
    lines = shell.exec_command(cmd)
    if parse is not None:
        for line in lines:
            if parse in line:
                found = True
                break
        if found is False:
            found, lines = await wait_for_shell_response(dut, parse, max_time)
    else:
        found = True
    logger.info(f'{lines}')
    assert found is True
    return lines


async def bumble_acl_connect(shell, dut, device, target_address):
    connection = None
    try:
        connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT)
        logger.info(f'=== Connected to {connection.peer_address}!')
    except Exception as e:
        logger.error(f'Fail to connect to {target_address}!')
        raise e
    return connection


async def bumble_acl_disconnect(shell, dut, device, connection):
    await device.disconnect(
        connection, reason=HCI_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES_ERROR
    )
    found, lines = await wait_for_shell_response(dut, "Disconnected:")
    logger.info(f'lines : {lines}')
    assert found is True
    return found, lines


class Delegate(PairingDelegate):
    def __init__(self, dut, io_capability):
        super().__init__(io_capability)
        self.dut = dut

    async def confirm(self, auto: bool = False) -> bool:
        return True


async def hid_case_register(hci_port, shell, dut, address, snoop_file) -> None:
    """Test: register HID device callbacks and SDP record."""
    logger.info('<<< hid_case_register ...')

    async with await open_transport_or_link(hci_port) as _:
        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "hid_dev register", "registered")


async def hid_case_connect_host_initiated(hci_port, shell, dut, address, snoop_file) -> None:
    """Test: Bumble (Host) initiates HID connection to DUT (Device)."""
    logger.info('<<< hid_case_connect_host_initiated ...')

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        delegate = Delegate(dut, PairingDelegate.IoCapability.KEYBOARD_INPUT_ONLY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True, mitm=True, bonding=True, delegate=delegate
        )
        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

        await send_cmd_to_iut(shell, dut, "br clear all", None)

        target_address = address.split(" ")[0]
        connection = await bumble_acl_connect(shell, dut, device, target_address)
        found, _ = await wait_for_shell_response(dut, "Connected:")
        assert found is True

        await device.authenticate(connection)
        await device.encrypt(connection)
        found, _ = await wait_for_shell_response(dut, "Security changed:")
        assert found is True

        hid_host = HIDHost(device)
        await hid_host.connect_control_channel()
        await hid_host.connect_interrupt_channel()

        found, _ = await wait_for_shell_response(dut, "HID connected")
        assert found is True

        await hid_host.disconnect_interrupt_channel()
        await hid_host.disconnect_control_channel()

        found, _ = await wait_for_shell_response(dut, "HID disconnected")
        assert found is True

        await bumble_acl_disconnect(shell, dut, device, connection)


async def hid_case_get_report(hci_port, shell, dut, address, snoop_file) -> None:
    """Test: Host sends GET_REPORT, Device responds with DATA."""
    logger.info('<<< hid_case_get_report ...')

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        delegate = Delegate(dut, PairingDelegate.IoCapability.KEYBOARD_INPUT_ONLY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True, mitm=True, bonding=True, delegate=delegate
        )
        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

        await send_cmd_to_iut(shell, dut, "br clear all", None)

        target_address = address.split(" ")[0]
        connection = await bumble_acl_connect(shell, dut, device, target_address)
        found, _ = await wait_for_shell_response(dut, "Connected:")
        assert found is True

        await device.authenticate(connection)
        await device.encrypt(connection)
        found, _ = await wait_for_shell_response(dut, "Security changed:")
        assert found is True

        hid_host = HIDHost(device)
        await hid_host.connect_control_channel()
        await hid_host.connect_interrupt_channel()
        found, _ = await wait_for_shell_response(dut, "HID connected")
        assert found is True

        # Test GET_REPORT with valid report ID
        ctrl_data_received = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_CONTROL_DATA, ctrl_data_received.set_result)

        hid_host.get_report(
            report_type=Message.ReportType.INPUT_REPORT, report_id=2, buffer_size=64
        )

        found, _ = await wait_for_shell_response(dut, "get_report type 1 id 2")
        assert found is True

        response = await asyncio.wait_for(ctrl_data_received, timeout=5.0)
        logger.info(f"GET_REPORT response: {response.hex()}")
        assert len(response) > 0

        # Test GET_REPORT with invalid report ID → should get handshake error
        handshake_received = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_HANDSHAKE, handshake_received.set_result)

        hid_host.get_report(
            report_type=Message.ReportType.INPUT_REPORT, report_id=99, buffer_size=64
        )

        handshake = await asyncio.wait_for(handshake_received, timeout=5.0)
        logger.info(f"GET_REPORT invalid id handshake: {handshake}")
        assert handshake == Message.Handshake.ERR_INVALID_REPORT_ID

        await hid_host.disconnect_interrupt_channel()
        await hid_host.disconnect_control_channel()
        found, _ = await wait_for_shell_response(dut, "HID disconnected")
        assert found is True

        await bumble_acl_disconnect(shell, dut, device, connection)


async def hid_case_set_report(hci_port, shell, dut, address, snoop_file) -> None:
    """Test: Host sends SET_REPORT, Device responds with HANDSHAKE."""
    logger.info('<<< hid_case_set_report ...')

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        delegate = Delegate(dut, PairingDelegate.IoCapability.KEYBOARD_INPUT_ONLY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True, mitm=True, bonding=True, delegate=delegate
        )
        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

        await send_cmd_to_iut(shell, dut, "br clear all", None)

        target_address = address.split(" ")[0]
        connection = await bumble_acl_connect(shell, dut, device, target_address)
        found, _ = await wait_for_shell_response(dut, "Connected:")
        assert found is True

        await device.authenticate(connection)
        await device.encrypt(connection)
        found, _ = await wait_for_shell_response(dut, "Security changed:")
        assert found is True

        hid_host = HIDHost(device)
        await hid_host.connect_control_channel()
        await hid_host.connect_interrupt_channel()
        found, _ = await wait_for_shell_response(dut, "HID connected")
        assert found is True

        # SET_REPORT with valid report ID → HANDSHAKE SUCCESS
        handshake_received = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_HANDSHAKE, handshake_received.set_result)

        hid_host.set_report(
            report_type=Message.ReportType.OUTPUT_REPORT, data=bytes([0x02, 0x00, 0x05, 0x05, 0x00])
        )

        handshake = await asyncio.wait_for(handshake_received, timeout=5.0)
        logger.info(f"SET_REPORT handshake: {handshake}")
        assert handshake == Message.Handshake.SUCCESSFUL

        found, _ = await wait_for_shell_response(dut, "set_report type 2 id 2")
        assert found is True

        # SET_REPORT with invalid report ID → HANDSHAKE ERR_INVALID_REPORT_ID
        handshake_received2 = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_HANDSHAKE, handshake_received2.set_result)

        hid_host.set_report(
            report_type=Message.ReportType.OUTPUT_REPORT,
            data=bytes([0x63, 0x00]),  # report_id=99
        )

        handshake2 = await asyncio.wait_for(handshake_received2, timeout=5.0)
        logger.info(f"SET_REPORT invalid handshake: {handshake2}")
        assert handshake2 == Message.Handshake.ERR_INVALID_REPORT_ID

        await hid_host.disconnect_interrupt_channel()
        await hid_host.disconnect_control_channel()
        await bumble_acl_disconnect(shell, dut, device, connection)


async def hid_case_protocol(hci_port, shell, dut, address, snoop_file) -> None:
    """Test: GET_PROTOCOL and SET_PROTOCOL."""
    logger.info('<<< hid_case_protocol ...')

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        delegate = Delegate(dut, PairingDelegate.IoCapability.KEYBOARD_INPUT_ONLY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True, mitm=True, bonding=True, delegate=delegate
        )
        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

        await send_cmd_to_iut(shell, dut, "br clear all", None)

        target_address = address.split(" ")[0]
        connection = await bumble_acl_connect(shell, dut, device, target_address)
        found, _ = await wait_for_shell_response(dut, "Connected:")
        assert found is True

        await device.authenticate(connection)
        await device.encrypt(connection)
        found, _ = await wait_for_shell_response(dut, "Security changed:")
        assert found is True

        hid_host = HIDHost(device)
        await hid_host.connect_control_channel()
        await hid_host.connect_interrupt_channel()
        found, _ = await wait_for_shell_response(dut, "HID connected")
        assert found is True

        # GET_PROTOCOL → should return REPORT_PROTOCOL (default)
        ctrl_data_received = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_CONTROL_DATA, ctrl_data_received.set_result)

        hid_host.get_protocol()
        found, _ = await wait_for_shell_response(dut, "get_protocol")
        assert found is True

        response = await asyncio.wait_for(ctrl_data_received, timeout=5.0)
        logger.info(f"GET_PROTOCOL response: {response.hex()}")

        # SET_PROTOCOL to BOOT mode → HANDSHAKE SUCCESS
        handshake_received = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_HANDSHAKE, handshake_received.set_result)

        hid_host.set_protocol(protocol_mode=Message.ProtocolMode.BOOT_PROTOCOL)

        handshake = await asyncio.wait_for(handshake_received, timeout=5.0)
        logger.info(f"SET_PROTOCOL handshake: {handshake}")
        assert handshake == Message.Handshake.SUCCESSFUL

        found, _ = await wait_for_shell_response(dut, "set_protocol 0")
        assert found is True

        # SET_PROTOCOL back to REPORT mode
        handshake_received2 = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_HANDSHAKE, handshake_received2.set_result)

        hid_host.set_protocol(protocol_mode=Message.ProtocolMode.REPORT_PROTOCOL)

        handshake2 = await asyncio.wait_for(handshake_received2, timeout=5.0)
        assert handshake2 == Message.Handshake.SUCCESSFUL

        found, _ = await wait_for_shell_response(dut, "set_protocol 1")
        assert found is True

        await hid_host.disconnect_interrupt_channel()
        await hid_host.disconnect_control_channel()
        await bumble_acl_disconnect(shell, dut, device, connection)


async def hid_case_send_input_report(hci_port, shell, dut, address, snoop_file) -> None:
    """Test: DUT sends input report, Bumble Host receives it."""
    logger.info('<<< hid_case_send_input_report ...')

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        delegate = Delegate(dut, PairingDelegate.IoCapability.KEYBOARD_INPUT_ONLY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True, mitm=True, bonding=True, delegate=delegate
        )
        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

        await send_cmd_to_iut(shell, dut, "br clear all", None)

        target_address = address.split(" ")[0]
        connection = await bumble_acl_connect(shell, dut, device, target_address)
        found, _ = await wait_for_shell_response(dut, "Connected:")
        assert found is True

        await device.authenticate(connection)
        await device.encrypt(connection)
        found, _ = await wait_for_shell_response(dut, "Security changed:")
        assert found is True

        hid_host = HIDHost(device)
        await hid_host.connect_control_channel()
        await hid_host.connect_interrupt_channel()
        found, _ = await wait_for_shell_response(dut, "HID connected")
        assert found is True

        # Listen for interrupt data from DUT
        intr_data_received = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_INTERRUPT_DATA, intr_data_received.set_result)

        # DUT sends a mouse report: button=0, X=10, Y=20, wheel=0
        await send_cmd_to_iut(shell, dut, "hid_dev send 0 10 20 0", "sent")

        data = await asyncio.wait_for(intr_data_received, timeout=5.0)
        logger.info(f"Received input report: {data.hex()}")
        assert len(data) >= 4  # Report ID + buttons + X + Y + wheel

        await hid_host.disconnect_interrupt_channel()
        await hid_host.disconnect_control_channel()
        await bumble_acl_disconnect(shell, dut, device, connection)


async def hid_case_suspend_resume(hci_port, shell, dut, address, snoop_file) -> None:
    """Test: Host sends SUSPEND and EXIT_SUSPEND."""
    logger.info('<<< hid_case_suspend_resume ...')

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        delegate = Delegate(dut, PairingDelegate.IoCapability.KEYBOARD_INPUT_ONLY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True, mitm=True, bonding=True, delegate=delegate
        )
        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

        await send_cmd_to_iut(shell, dut, "br clear all", None)

        target_address = address.split(" ")[0]
        connection = await bumble_acl_connect(shell, dut, device, target_address)
        found, _ = await wait_for_shell_response(dut, "Connected:")
        assert found is True

        await device.authenticate(connection)
        await device.encrypt(connection)
        found, _ = await wait_for_shell_response(dut, "Security changed:")
        assert found is True

        hid_host = HIDHost(device)
        await hid_host.connect_control_channel()
        await hid_host.connect_interrupt_channel()
        found, _ = await wait_for_shell_response(dut, "HID connected")
        assert found is True

        # Send SUSPEND
        hid_host.suspend()
        found, _ = await wait_for_shell_response(dut, "suspended")
        assert found is True

        # Send EXIT_SUSPEND
        hid_host.exit_suspend()
        found, _ = await wait_for_shell_response(dut, "exit_suspend")
        assert found is True

        await hid_host.disconnect_interrupt_channel()
        await hid_host.disconnect_control_channel()
        await bumble_acl_disconnect(shell, dut, device, connection)


async def hid_case_virtual_cable_unplug(hci_port, shell, dut, address, snoop_file) -> None:
    """Test: DUT sends Virtual Cable Unplug."""
    logger.info('<<< hid_case_virtual_cable_unplug ...')

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        delegate = Delegate(dut, PairingDelegate.IoCapability.KEYBOARD_INPUT_ONLY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True, mitm=True, bonding=True, delegate=delegate
        )
        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

        await send_cmd_to_iut(shell, dut, "br clear all", None)

        target_address = address.split(" ")[0]
        connection = await bumble_acl_connect(shell, dut, device, target_address)
        found, _ = await wait_for_shell_response(dut, "Connected:")
        assert found is True

        await device.authenticate(connection)
        await device.encrypt(connection)
        found, _ = await wait_for_shell_response(dut, "Security changed:")
        assert found is True

        hid_host = HIDHost(device)
        await hid_host.connect_control_channel()
        await hid_host.connect_interrupt_channel()
        found, _ = await wait_for_shell_response(dut, "HID connected")
        assert found is True

        # Listen for VCU from device
        vcu_received = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_VIRTUAL_CABLE_UNPLUG, lambda: vcu_received.set_result(True))

        # DUT initiates VCU
        await send_cmd_to_iut(shell, dut, "hid_dev vcu", None)

        vcu = await asyncio.wait_for(vcu_received, timeout=5.0)
        assert vcu is True

        found, _ = await wait_for_shell_response(dut, "HID disconnected")
        assert found is True

        await bumble_acl_disconnect(shell, dut, device, connection)


class TestHidDevice:
    def test_hid_case_register(self, shell: Shell, dut: DeviceAdapter, hid_device_dut):
        logger.info(f'test_hid_case_register {hid_device_dut}')
        hci, iut_address = hid_device_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(hid_case_register(hci, shell, dut, iut_address, snoop_file))

    def test_hid_case_connect_host_initiated(
        self, shell: Shell, dut: DeviceAdapter, hid_device_dut
    ):
        logger.info(f'test_hid_case_connect_host_initiated {hid_device_dut}')
        hci, iut_address = hid_device_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(hid_case_connect_host_initiated(hci, shell, dut, iut_address, snoop_file))

    def test_hid_case_get_report(self, shell: Shell, dut: DeviceAdapter, hid_device_dut):
        logger.info(f'test_hid_case_get_report {hid_device_dut}')
        hci, iut_address = hid_device_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(hid_case_get_report(hci, shell, dut, iut_address, snoop_file))

    def test_hid_case_set_report(self, shell: Shell, dut: DeviceAdapter, hid_device_dut):
        logger.info(f'test_hid_case_set_report {hid_device_dut}')
        hci, iut_address = hid_device_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(hid_case_set_report(hci, shell, dut, iut_address, snoop_file))

    def test_hid_case_protocol(self, shell: Shell, dut: DeviceAdapter, hid_device_dut):
        logger.info(f'test_hid_case_protocol {hid_device_dut}')
        hci, iut_address = hid_device_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(hid_case_protocol(hci, shell, dut, iut_address, snoop_file))

    def test_hid_case_send_input_report(self, shell: Shell, dut: DeviceAdapter, hid_device_dut):
        logger.info(f'test_hid_case_send_input_report {hid_device_dut}')
        hci, iut_address = hid_device_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(hid_case_send_input_report(hci, shell, dut, iut_address, snoop_file))

    def test_hid_case_suspend_resume(self, shell: Shell, dut: DeviceAdapter, hid_device_dut):
        logger.info(f'test_hid_case_suspend_resume {hid_device_dut}')
        hci, iut_address = hid_device_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(hid_case_suspend_resume(hci, shell, dut, iut_address, snoop_file))

    def test_hid_case_virtual_cable_unplug(self, shell: Shell, dut: DeviceAdapter, hid_device_dut):
        logger.info(f'test_hid_case_virtual_cable_unplug {hid_device_dut}')
        hci, iut_address = hid_device_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(hid_case_virtual_cable_unplug(hci, shell, dut, iut_address, snoop_file))

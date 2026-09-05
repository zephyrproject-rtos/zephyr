# Copyright 2026 Xiaomi Corporation
#
# SPDX-License-Identifier: Apache-2.0

import asyncio
import contextlib
import logging

import pytest
from bumble.core import (
    BT_BR_EDR_TRANSPORT,
)
from bumble.device import Device
from bumble.hci import (
    HCI_REMOTE_USER_TERMINATED_CONNECTION_ERROR,
    Address,
    HCI_Write_Page_Timeout_Command,
)
from bumble.hid import Host as HIDHost
from bumble.hid import Message
from bumble.l2cap import ClassicChannel
from bumble.pairing import PairingConfig, PairingDelegate
from bumble.snoop import BtSnooper
from bumble.transport import open_transport_or_link
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)

# Report ID declared by the DUT mouse report descriptor
MOUSE_REPORT_ID = 2

# HIDP message headers: message type in the upper nibble, parameter
# (report type / protocol mode) in the lower nibble.
HIDP_DATA_INPUT = 0xA1
HIDP_DATA_OTHER = 0xA0

# Idle mouse report the DUT answers GET_REPORT with, prefixed by the DATA header
GET_REPORT_RSP = bytes([HIDP_DATA_INPUT, MOUSE_REPORT_ID, 0x00, 0x00, 0x00, 0x00])

# Mouse report the DUT sends for "hid_dev send 0 10 20 0"
INPUT_REPORT = bytes([HIDP_DATA_INPUT, MOUSE_REPORT_ID, 0x00, 0x0A, 0x14, 0x00])

# GET_PROTOCOL responses for Report and Boot Protocol Mode
GET_PROTOCOL_REPORT_RSP = bytes([HIDP_DATA_OTHER, Message.ProtocolMode.REPORT_PROTOCOL])
GET_PROTOCOL_BOOT_RSP = bytes([HIDP_DATA_OTHER, Message.ProtocolMode.BOOT_PROTOCOL])


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


async def wait_for_shell_responses(dut, messages, max_time=10):
    """Wait until every message in ``messages`` has been seen.

    ``wait_for_shell_response`` stops at the first match and the rest of that
    read batch is lost for the next call, so events the DUT emits back to back
    have to be awaited together.
    """
    pending = list(messages)
    lines = []
    try:
        for _ in range(0, max_time):
            read_lines = dut.readlines()
            logger.info(f"{read_lines}")
            lines = lines + read_lines
            for line in read_lines:
                for message in list(pending):
                    if message in line:
                        pending.remove(message)
            if not pending:
                break
            await asyncio.sleep(1)
    except Exception as e:
        logger.error(f'{e}!', exc_info=True)
        raise e
    return not pending, lines


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
    # HCI_Disconnect only accepts a restricted set of reason codes.
    # REMOTE_USER_TERMINATED is the portable choice; strict controllers reject
    # anything outside that list with INVALID_COMMAND_PARAMETERS.
    await device.disconnect(connection, reason=HCI_REMOTE_USER_TERMINATED_CONNECTION_ERROR)
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


async def hid_register_iut(shell, dut):
    """Make sure the DUT has the HID Device registered.

    Registration survives for the whole session, so any test may be the first
    one to run. "already registered" is therefore an acceptable answer.
    """
    lines = shell.exec_command("hid_dev register")
    logger.info(f'{lines}')
    assert any(("HID registered" in line) or ("HID already registered" in line) for line in lines)


def channel_is_open(channel) -> bool:
    """True when the L2CAP channel exists and was not closed by the peer."""
    return channel is not None and channel.state == ClassicChannel.State.OPEN


def bumble_device_new(dut, hci_transport, snoop_file):
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
    return device


@contextlib.asynccontextmanager
async def hid_host_session(hci_port, shell, dut, address, snoop_file, connect_hid=True):
    """Bring up an authenticated ACL link with Bumble acting as HID Host.

    Yields the Bumble ``HIDHost`` once both HID channels are up (unless
    ``connect_hid`` is False, in which case only the L2CAP servers are armed so
    the DUT can initiate). Whatever is left open is torn down on exit.
    """
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = bumble_device_new(dut, hci_transport, snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

        # The HID Host has to exist before the ACL link is up: it latches the
        # connection from the device event, and its L2CAP servers must already
        # be registered for a device-initiated connection to be accepted.
        hid_host = HIDHost(device)

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await hid_register_iut(shell, dut)

        target_address = address.split(" ")[0]
        connection = await bumble_acl_connect(shell, dut, device, target_address)
        found, _ = await wait_for_shell_response(dut, "Connected:")
        assert found is True

        # The DUT releases the ACL link once the last L2CAP channel is gone, so
        # the teardown below must not disconnect a link that is already down.
        acl_disconnected = asyncio.Event()
        connection.on(connection.EVENT_DISCONNECTION, lambda reason: acl_disconnected.set())

        await device.authenticate(connection)
        await device.encrypt(connection)
        found, _ = await wait_for_shell_response(dut, "Security changed:")
        assert found is True

        if connect_hid:
            await hid_host.connect_control_channel()
            await hid_host.connect_interrupt_channel()
            found, _ = await wait_for_shell_response(dut, "HID connected")
            assert found is True

        try:
            yield hid_host
        finally:
            if not acl_disconnected.is_set():
                if channel_is_open(hid_host.l2cap_intr_channel):
                    await hid_host.disconnect_interrupt_channel()
                if channel_is_open(hid_host.l2cap_ctrl_channel):
                    await hid_host.disconnect_control_channel()

            # Give the DUT the chance to release the link on its own first
            if acl_disconnected.is_set():
                logger.info('=== ACL released by the DUT')
            else:
                await bumble_acl_disconnect(shell, dut, device, connection)


async def hid_case_register(hci_port, shell, dut, address, snoop_file) -> None:
    """Register and unregister the HID Device callbacks and SDP record."""
    logger.info('<<< hid_case_register ...')

    await send_cmd_to_iut(shell, dut, "br clear all", None)

    # The DUT may already be registered by a previous test in the session
    await hid_register_iut(shell, dut)
    await send_cmd_to_iut(shell, dut, "hid_dev unregister", "HID unregistered")

    # A second unregister must be rejected instead of unregistering twice
    await send_cmd_to_iut(shell, dut, "hid_dev unregister", "HID not registered")

    # Leave the DUT registered for the remaining tests
    await send_cmd_to_iut(shell, dut, "hid_dev register", "HID registered")


async def hid_case_connect_host_initiated(hci_port, shell, dut, address, snoop_file) -> None:
    """Host opens the control and interrupt channels, then closes them."""
    logger.info('<<< hid_case_connect_host_initiated ...')

    async with hid_host_session(hci_port, shell, dut, address, snoop_file) as hid_host:
        await hid_host.disconnect_interrupt_channel()
        await hid_host.disconnect_control_channel()

        found, _ = await wait_for_shell_response(dut, "HID disconnected")
        assert found is True


async def hid_case_connect_device_initiated(hci_port, shell, dut, address, snoop_file) -> None:
    """DUT opens both HID channels, then disconnects them."""
    logger.info('<<< hid_case_connect_device_initiated ...')

    async with hid_host_session(
        hci_port, shell, dut, address, snoop_file, connect_hid=False
    ) as hid_host:
        await send_cmd_to_iut(shell, dut, "hid_dev connect", "HID connected")
        assert hid_host.l2cap_ctrl_channel is not None
        assert hid_host.l2cap_intr_channel is not None

        await send_cmd_to_iut(shell, dut, "hid_dev disconnect", "HID disconnected")


async def hid_case_get_report(hci_port, shell, dut, address, snoop_file) -> None:
    """GET_REPORT: valid request answered with DATA, invalid ones rejected."""
    logger.info('<<< hid_case_get_report ...')

    async with hid_host_session(hci_port, shell, dut, address, snoop_file) as hid_host:
        # Valid report ID: the DUT answers with the idle mouse report
        ctrl_data = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_CONTROL_DATA, ctrl_data.set_result)

        hid_host.get_report(
            report_type=Message.ReportType.INPUT_REPORT,
            report_id=MOUSE_REPORT_ID,
            buffer_size=64,
        )

        found, _ = await wait_for_shell_response(dut, f"get_report type 1 id {MOUSE_REPORT_ID}")
        assert found is True

        response = await asyncio.wait_for(ctrl_data, timeout=5.0)
        logger.info(f"GET_REPORT response: {response.hex()}")
        assert response == GET_REPORT_RSP

        # Unknown report ID: ERR_INVALID_REPORT_ID
        handshake = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_HANDSHAKE, handshake.set_result)

        hid_host.get_report(
            report_type=Message.ReportType.INPUT_REPORT, report_id=99, buffer_size=64
        )

        result = await asyncio.wait_for(handshake, timeout=5.0)
        logger.info(f"GET_REPORT invalid id handshake: {result}")
        assert result == Message.Handshake.ERR_INVALID_REPORT_ID

        # Report type the descriptor does not declare: ERR_INVALID_PARAMETER
        handshake = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_HANDSHAKE, handshake.set_result)

        hid_host.get_report(
            report_type=Message.ReportType.FEATURE_REPORT,
            report_id=MOUSE_REPORT_ID,
            buffer_size=64,
        )

        result = await asyncio.wait_for(handshake, timeout=5.0)
        logger.info(f"GET_REPORT invalid type handshake: {result}")
        assert result == Message.Handshake.ERR_INVALID_PARAMETER


async def hid_case_set_report(hci_port, shell, dut, address, snoop_file) -> None:
    """SET_REPORT: valid request acknowledged, unknown report ID rejected."""
    logger.info('<<< hid_case_set_report ...')

    async with hid_host_session(hci_port, shell, dut, address, snoop_file) as hid_host:
        handshake = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_HANDSHAKE, handshake.set_result)

        hid_host.set_report(
            report_type=Message.ReportType.OUTPUT_REPORT,
            data=bytes([MOUSE_REPORT_ID, 0x00, 0x05, 0x05, 0x00]),
        )

        result = await asyncio.wait_for(handshake, timeout=5.0)
        logger.info(f"SET_REPORT handshake: {result}")
        assert result == Message.Handshake.SUCCESSFUL

        found, _ = await wait_for_shell_response(dut, f"set_report type 2 id {MOUSE_REPORT_ID}")
        assert found is True

        # Unknown report ID: ERR_INVALID_REPORT_ID
        handshake = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_HANDSHAKE, handshake.set_result)

        hid_host.set_report(
            report_type=Message.ReportType.OUTPUT_REPORT,
            data=bytes([99, 0x00]),
        )

        result = await asyncio.wait_for(handshake, timeout=5.0)
        logger.info(f"SET_REPORT invalid id handshake: {result}")
        assert result == Message.Handshake.ERR_INVALID_REPORT_ID


async def hid_case_protocol(hci_port, shell, dut, address, snoop_file) -> None:
    """GET_PROTOCOL / SET_PROTOCOL round trip through both protocol modes."""
    logger.info('<<< hid_case_protocol ...')

    async def get_protocol(expected):
        ctrl_data = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_CONTROL_DATA, ctrl_data.set_result)
        hid_host.get_protocol()
        response = await asyncio.wait_for(ctrl_data, timeout=5.0)
        logger.info(f"GET_PROTOCOL response: {response.hex()}")
        assert response == expected

    async def set_protocol(mode):
        handshake = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_HANDSHAKE, handshake.set_result)
        hid_host.set_protocol(protocol_mode=mode)
        result = await asyncio.wait_for(handshake, timeout=5.0)
        logger.info(f"SET_PROTOCOL {mode.name} handshake: {result}")
        assert result == Message.Handshake.SUCCESSFUL
        found, _ = await wait_for_shell_response(dut, f"set_protocol {mode.value}")
        assert found is True

    async with hid_host_session(hci_port, shell, dut, address, snoop_file) as hid_host:
        # A fresh connection defaults to Report Protocol Mode. GET_PROTOCOL is
        # answered by the stack itself, so there is no DUT shell output here.
        await get_protocol(GET_PROTOCOL_REPORT_RSP)

        await set_protocol(Message.ProtocolMode.BOOT_PROTOCOL)
        await get_protocol(GET_PROTOCOL_BOOT_RSP)

        await set_protocol(Message.ProtocolMode.REPORT_PROTOCOL)
        await get_protocol(GET_PROTOCOL_REPORT_RSP)


async def hid_case_send_input_report(hci_port, shell, dut, address, snoop_file) -> None:
    """DUT sends an input report on the interrupt channel."""
    logger.info('<<< hid_case_send_input_report ...')

    async with hid_host_session(hci_port, shell, dut, address, snoop_file) as hid_host:
        intr_data = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_INTERRUPT_DATA, intr_data.set_result)

        # button=0, X=10, Y=20, wheel=0
        await send_cmd_to_iut(shell, dut, "hid_dev send 0 10 20 0", "sent")

        data = await asyncio.wait_for(intr_data, timeout=5.0)
        logger.info(f"Received input report: {data.hex()}")
        assert data == INPUT_REPORT


async def hid_case_output_report(hci_port, shell, dut, address, snoop_file) -> None:
    """Host sends an output report on the interrupt channel."""
    logger.info('<<< hid_case_output_report ...')

    async with hid_host_session(hci_port, shell, dut, address, snoop_file) as hid_host:
        hid_host.send_data(bytes([MOUSE_REPORT_ID, 0xFF]))

        found, _ = await wait_for_shell_response(dut, f"output_report id {MOUSE_REPORT_ID} len 1")
        assert found is True


async def hid_case_suspend_resume(hci_port, shell, dut, address, snoop_file) -> None:
    """Host sends SUSPEND and EXIT_SUSPEND."""
    logger.info('<<< hid_case_suspend_resume ...')

    async with hid_host_session(hci_port, shell, dut, address, snoop_file) as hid_host:
        hid_host.suspend()
        found, _ = await wait_for_shell_response(dut, "suspended")
        assert found is True

        hid_host.exit_suspend()
        found, _ = await wait_for_shell_response(dut, "exit_suspend")
        assert found is True


async def hid_case_vcu_from_device(hci_port, shell, dut, address, snoop_file) -> None:
    """DUT initiates Virtual Cable Unplug."""
    logger.info('<<< hid_case_vcu_from_device ...')

    async with hid_host_session(hci_port, shell, dut, address, snoop_file) as hid_host:
        vcu_received = asyncio.get_running_loop().create_future()
        hid_host.once(HIDHost.EVENT_VIRTUAL_CABLE_UNPLUG, lambda: vcu_received.set_result(True))

        await send_cmd_to_iut(shell, dut, "hid_dev vcu", None)

        assert await asyncio.wait_for(vcu_received, timeout=5.0) is True

        # The link is torn down after a Virtual Cable Unplug
        found, _ = await wait_for_shell_response(dut, "HID disconnected")
        assert found is True


async def hid_case_vcu_from_host(hci_port, shell, dut, address, snoop_file) -> None:
    """Host initiates Virtual Cable Unplug."""
    logger.info('<<< hid_case_vcu_from_host ...')

    async with hid_host_session(hci_port, shell, dut, address, snoop_file) as hid_host:
        hid_host.virtual_cable_unplug()

        # The recipient drops the link, so both events show up back to back
        found, _ = await wait_for_shell_responses(dut, ["virtual_cable_unplug", "HID disconnected"])
        assert found is True


class TestHidDevice:
    @pytest.fixture(autouse=True)
    def snoop_name(self, request):
        self.snoop_path = f"bumble_hci_{request.node.name}.log"

    def run_case(self, case, shell, dut, hid_device_dut):
        hci, iut_address = hid_device_dut
        logger.info(f'{case.__name__} {hid_device_dut}')
        with open(self.snoop_path, "wb") as snoop_file:
            asyncio.run(case(hci, shell, dut, iut_address, snoop_file))

    def test_hid_case_register(self, shell: Shell, dut: DeviceAdapter, hid_device_dut):
        self.run_case(hid_case_register, shell, dut, hid_device_dut)

    def test_hid_case_connect_host_initiated(
        self, shell: Shell, dut: DeviceAdapter, hid_device_dut
    ):
        self.run_case(hid_case_connect_host_initiated, shell, dut, hid_device_dut)

    def test_hid_case_connect_device_initiated(
        self, shell: Shell, dut: DeviceAdapter, hid_device_dut
    ):
        self.run_case(hid_case_connect_device_initiated, shell, dut, hid_device_dut)

    def test_hid_case_get_report(self, shell: Shell, dut: DeviceAdapter, hid_device_dut):
        self.run_case(hid_case_get_report, shell, dut, hid_device_dut)

    def test_hid_case_set_report(self, shell: Shell, dut: DeviceAdapter, hid_device_dut):
        self.run_case(hid_case_set_report, shell, dut, hid_device_dut)

    def test_hid_case_protocol(self, shell: Shell, dut: DeviceAdapter, hid_device_dut):
        self.run_case(hid_case_protocol, shell, dut, hid_device_dut)

    def test_hid_case_send_input_report(self, shell: Shell, dut: DeviceAdapter, hid_device_dut):
        self.run_case(hid_case_send_input_report, shell, dut, hid_device_dut)

    def test_hid_case_output_report(self, shell: Shell, dut: DeviceAdapter, hid_device_dut):
        self.run_case(hid_case_output_report, shell, dut, hid_device_dut)

    def test_hid_case_suspend_resume(self, shell: Shell, dut: DeviceAdapter, hid_device_dut):
        self.run_case(hid_case_suspend_resume, shell, dut, hid_device_dut)

    def test_hid_case_vcu_from_device(self, shell: Shell, dut: DeviceAdapter, hid_device_dut):
        self.run_case(hid_case_vcu_from_device, shell, dut, hid_device_dut)

    def test_hid_case_vcu_from_host(self, shell: Shell, dut: DeviceAdapter, hid_device_dut):
        self.run_case(hid_case_vcu_from_host, shell, dut, hid_device_dut)

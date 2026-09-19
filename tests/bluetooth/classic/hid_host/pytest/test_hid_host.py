# Copyright (c) 2026 Xiaomi Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""Runtime tests for the Bluetooth Classic HID Host profile.

The IUT is the Zephyr classic shell built with CONFIG_BT_HID_HOST=y, driven
through the ``hid_host`` shell commands. The peer is a Bumble HID Device
(bumble.hid.Device) answering the control-channel transactions, so both sides
run on real controllers: the IUT on an HCI User Channel adapter, the peer on a
second adapter through libusb.

Covered: host-initiated association setup, GET_REPORT with a Report ID,
GET_PROTOCOL in both protocol modes, a malformed GET_PROTOCOL reply, SET_REPORT
acknowledged by a HANDSHAKE, host- and device-initiated connection release
(HID11/HOS/HCR/BV-01-C and BV-02-C), and reading the HID service record over
SDP.
"""

import asyncio
import contextlib
import logging

from bumble.core import (
    BT_BR_EDR_TRANSPORT,
    BT_HIDP_PROTOCOL_ID,
    BT_HUMAN_INTERFACE_DEVICE_SERVICE,
    BT_L2CAP_PROTOCOL_ID,
)
from bumble.device import Device
from bumble.hci import (
    HCI_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES_ERROR,
    Address,
    HCI_Write_Page_Timeout_Command,
)
from bumble.hid import HID_CONTROL_PSM, HID_INTERRUPT_PSM, Message
from bumble.hid import Device as HIDDevice
from bumble.l2cap import ClassicChannel
from bumble.pairing import PairingConfig, PairingDelegate
from bumble.sdp import (
    SDP_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_ID,
    SDP_BROWSE_GROUP_LIST_ATTRIBUTE_ID,
    SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID,
    SDP_PUBLIC_BROWSE_ROOT,
    SDP_SERVICE_CLASS_ID_LIST_ATTRIBUTE_ID,
    SDP_SERVICE_RECORD_HANDLE_ATTRIBUTE_ID,
    DataElement,
    ServiceAttribute,
)
from bumble.snoop import BtSnooper
from bumble.transport import open_transport_or_link
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)

# HID SDP attribute IDs, HID spec v1.1.2 Section 5.3.4.
SDP_ATTR_HID_DEVICE_SUBCLASS = 0x0202
SDP_ATTR_HID_VIRTUAL_CABLE = 0x0204
SDP_ATTR_HID_RECONNECT_INITIATE = 0x0205
SDP_ATTR_HID_DESCRIPTOR_LIST = 0x0206
SDP_ATTR_HID_SUPERVISION_TIMEOUT = 0x020C
SDP_ATTR_HID_BOOT_DEVICE = 0x020E

# ClassDescriptorType of a Report descriptor, HID spec v1.1.2 Section 5.3.4.7.
HID_CLASS_DESC_TYPE_REPORT = 0x22

# A mouse report descriptor that declares a Report ID (0x85 0x02), so the IUT has
# to split the Report ID off incoming reports and add it to its requests.
HID_REPORT_DESCRIPTOR = bytes(
    [
        0x05, 0x01,  # Usage Page (Generic Desktop)
        0x09, 0x02,  # Usage (Mouse)
        0xA1, 0x01,  # Collection (Application)
        0x85, 0x02,  # Report ID (2)
        0x09, 0x01,  # Usage (Pointer)
        0xA1, 0x00,  # Collection (Physical)
        0x05, 0x09,  # Usage Page (Button)
        0x19, 0x01,  # Usage Minimum (1)
        0x29, 0x03,  # Usage Maximum (3)
        0x15, 0x00,  # Logical Minimum (0)
        0x25, 0x01,  # Logical Maximum (1)
        0x75, 0x01,  # Report Size (1)
        0x95, 0x03,  # Report Count (3)
        0x81, 0x02,  # Input (Data, Variable, Absolute)
        0x75, 0x05,  # Report Size (5)
        0x95, 0x01,  # Report Count (1)
        0x81, 0x03,  # Input (Constant)
        0x05, 0x01,  # Usage Page (Generic Desktop)
        0x09, 0x30,  # Usage (X)
        0x09, 0x31,  # Usage (Y)
        0x15, 0x81,  # Logical Minimum (-127)
        0x25, 0x7F,  # Logical Maximum (127)
        0x75, 0x08,  # Report Size (8)
        0x95, 0x02,  # Report Count (2)
        0x81, 0x06,  # Input (Data, Variable, Relative)
        0xC0,        # End Collection
        0xC0,        # End Collection
    ]
)  # fmt: skip

# Service record of the peer, holding the attributes the IUT reads back with
# ``hid_host sdp_discover``.
SDP_SERVICE_RECORDS_HID = {
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
            DataElement.sequence([DataElement.uuid(BT_HUMAN_INTERFACE_DEVICE_SERVICE)]),
        ),
        ServiceAttribute(
            SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_L2CAP_PROTOCOL_ID),
                            DataElement.unsigned_integer_16(HID_CONTROL_PSM),
                        ]
                    ),
                    DataElement.sequence([DataElement.uuid(BT_HIDP_PROTOCOL_ID)]),
                ]
            ),
        ),
        ServiceAttribute(
            SDP_BLUETOOTH_PROFILE_DESCRIPTOR_LIST_ATTRIBUTE_ID,
            DataElement.sequence(
                [
                    DataElement.sequence(
                        [
                            DataElement.uuid(BT_HUMAN_INTERFACE_DEVICE_SERVICE),
                            DataElement.unsigned_integer_16(0x0101),
                        ]
                    )
                ]
            ),
        ),
        ServiceAttribute(
            SDP_ATTR_HID_DEVICE_SUBCLASS,
            DataElement.unsigned_integer_8(0x80),
        ),
        ServiceAttribute(SDP_ATTR_HID_VIRTUAL_CABLE, DataElement.boolean(True)),
        ServiceAttribute(SDP_ATTR_HID_RECONNECT_INITIATE, DataElement.boolean(True)),
        ServiceAttribute(
            SDP_ATTR_HID_DESCRIPTOR_LIST,
            DataElement.sequence(
                [
                    DataElement.sequence(
                        [
                            DataElement.unsigned_integer_8(HID_CLASS_DESC_TYPE_REPORT),
                            DataElement.text_string(HID_REPORT_DESCRIPTOR),
                        ]
                    )
                ]
            ),
        ),
        ServiceAttribute(
            SDP_ATTR_HID_SUPERVISION_TIMEOUT,
            DataElement.unsigned_integer_16(0x0C80),
        ),
        ServiceAttribute(SDP_ATTR_HID_BOOT_DEVICE, DataElement.boolean(True)),
    ]
}


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
    for _ in range(0, max_time):
        if found:
            break
        read_lines = dut.readlines()
        logger.info(f"{read_lines}")
        for line in read_lines:
            if message in line:
                found = True
                break
        lines = lines + read_lines
        await asyncio.sleep(1)
    return found, lines


async def send_cmd_to_iut(shell, dut, cmd, parse, max_time=10):
    """Send a shell command and match ``parse`` in its output.

    ``shell.exec_command`` already drains the command's synchronous output up to
    the next prompt, so check that first; only fall back to polling ``dut`` for
    output that arrives asynchronously (e.g. connection events)."""
    found = False
    lines = shell.exec_command(cmd)
    for line in lines:
        if parse in line:
            found = True
            break
    if not found:
        found, lines = await wait_for_shell_response(dut, parse, max_time)
    logger.info(f"{lines}")
    return found, lines


class Delegate(PairingDelegate):
    async def confirm(self, auto: bool = False) -> bool:
        return True


async def setup_bumble_device(hci_transport, snoop_file, sdp_records=None):
    device = Device.with_hci(
        'Bumble',
        Address('F0:F1:F2:F3:F4:F5'),
        hci_transport.source,
        hci_transport.sink,
    )
    device.classic_enabled = True
    device.le_enabled = False
    delegate = Delegate(PairingDelegate.IoCapability.KEYBOARD_INPUT_ONLY)
    device.pairing_config_factory = lambda connection: PairingConfig(
        sc=True, mitm=True, bonding=True, delegate=delegate
    )
    device.host.snooper = BtSnooper(snoop_file)
    if sdp_records is not None:
        device.sdp_service_records = sdp_records
    await device_power_on(device)
    await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
    return device


async def iut_acl_connect(shell, dut, device, address):
    """Page the IUT from the peer and elevate security, without opening HID.

    Returns the Bumble HID Device object so the caller can register callbacks or
    interfere with the L2CAP channels while the IUT sets them up.
    """
    await device.set_discoverable(True)
    await device.set_connectable(True)

    peer = address.split(" ")[0]
    hid_dev = HIDDevice(device)

    # Session-scoped fixture shares the DUT across cases: the first case
    # registers, later ones report "already registered" - both are acceptable.
    lines = shell.exec_command("hid_host register")
    assert any(("registered" in line) for line in lines), "hid_host register failed"

    # Bumble pages the IUT; the IUT's connected callback prints "Connected:".
    connection = await device.connect(peer, transport=BT_BR_EDR_TRANSPORT)
    found, _ = await wait_for_shell_response(dut, "Connected:", max_time=20)
    assert found, "ACL connect failed"

    # Elevate security so the HID L2CAP channels are allowed to open.
    await device.authenticate(connection)
    await device.encrypt(connection)
    found, _ = await wait_for_shell_response(dut, "Security changed", max_time=20)
    assert found, "security elevation failed"

    return hid_dev


async def iut_connect(shell, dut, device, address):
    """Bring up an ACL + HID connection between the Bumble peer and the IUT.

    The ACL is paged by the Bumble peer (the reliable direction for this dongle
    pair); the IUT then drives ``hid_host connect`` so the HID L2CAP channels are
    Host-initiated, as described in HID spec v1.1.2 Section 5.2.2.
    Returns the Bumble HID Device object so the caller can register callbacks.
    """
    hid_dev = await iut_acl_connect(shell, dut, device, address)

    found, _ = await send_cmd_to_iut(
        shell, dut, "hid_host connect", "HID Host: connected", max_time=20
    )
    assert found, "HID connect failed"
    return hid_dev


async def iut_disconnect(shell, dut, device, hid_dev):
    """Tear down the HID + ACL link so the next case starts clean."""
    shell.exec_command("hid_host disconnect")
    await wait_for_shell_response(dut, "HID Host: disconnected", max_time=10)
    for connection in list(device.connections.values()):
        # The peer may already be gone: the IUT closes the ACL itself on some paths.
        with contextlib.suppress(Exception):
            await device.disconnect(
                connection, reason=HCI_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES_ERROR
            )
    await wait_for_shell_response(dut, "Disconnected", max_time=10)


async def hid_case_get_report(hci_port, shell, dut, address, snoop_file):
    """GET_REPORT round trip: IUT claims the transaction, peer answers with DATA,
    IUT dispatches to the callback and releases the lock."""
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = await setup_bumble_device(hci_transport, snoop_file)
        hid_dev = await iut_connect(shell, dut, device, address)

        # Without a parsed report descriptor the IUT treats report id 0 as "no
        # Report ID" and omits the field from the PDU (HID spec v1.1.2 3.1.2.3).
        # Bumble's handle_get_report, however, always reads pdu[1] as the report
        # id and pdu[2:4] as the buffer size, so a report-id-less PDU makes it
        # index past the end. Boot protocol always carries a report id, so drive
        # a GET_PROTOCOL that reports BOOT first: the IUT flips into boot mode and
        # then includes the report-id byte, giving Bumble a well-formed PDU.
        def get_protocol_cb():
            return HIDDevice.GetSetStatus(
                data=bytes([Message.ProtocolMode.BOOT_PROTOCOL]),
                status=HIDDevice.GetSetReturn.SUCCESS,
            )

        hid_dev.get_protocol_cb = get_protocol_cb
        found, _ = await send_cmd_to_iut(
            shell, dut, "hid_host get_protocol", "HID Host: get_protocol 0 (boot)"
        )
        assert found, "IUT did not enter boot protocol"

        def get_report_cb(report_id, report_type, buffer_size):
            # Peer echoes the requested type; payload is one report byte.
            return HIDDevice.GetSetStatus(
                data=bytes([0xAB]),
                status=HIDDevice.GetSetReturn.SUCCESS,
            )

        hid_dev.get_report_cb = get_report_cb

        # type=INPUT(1) id=2 buf_size=8. In boot mode the IUT emits the report id,
        # so the wire PDU is [header][report_id][size_lo][size_hi] as Bumble wants.
        found, _ = await send_cmd_to_iut(
            shell,
            dut,
            f"hid_host get_report {Message.ReportType.INPUT_REPORT} 2 8",
            "HID Host: get_report type 1",
        )
        assert found, "IUT did not deliver the GET_REPORT DATA reply"
        await iut_disconnect(shell, dut, device, hid_dev)


async def hid_case_get_protocol(hci_port, shell, dut, address, snoop_file):
    """GET_PROTOCOL round trip through the same transaction lock."""
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = await setup_bumble_device(hci_transport, snoop_file)
        hid_dev = await iut_connect(shell, dut, device, address)

        def get_protocol_cb():
            return HIDDevice.GetSetStatus(
                data=bytes([Message.ProtocolMode.REPORT_PROTOCOL]),
                status=HIDDevice.GetSetReturn.SUCCESS,
            )

        hid_dev.get_protocol_cb = get_protocol_cb

        found, _ = await send_cmd_to_iut(
            shell, dut, "hid_host get_protocol", "HID Host: get_protocol"
        )
        assert found, "IUT did not deliver the GET_PROTOCOL reply"
        await iut_disconnect(shell, dut, device, hid_dev)


async def hid_case_get_protocol_malformed(hci_port, shell, dut, address, snoop_file):
    """GET_PROTOCOL answered by a zero-length DATA message.

    HID spec v1.1.2 Section 3.1.2.5: the reply to GET_PROTOCOL is a DATA message
    carrying a single protocol-mode octet. A device that returns an empty DATA
    payload is malformed. The IUT consumes the outstanding transaction before
    inspecting the reply, so a malformed reply must be surfaced to the
    application as an error (get_protocol failed) rather than silently dropped,
    which would leave the app waiting for a reply that was already consumed.
    """
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = await setup_bumble_device(hci_transport, snoop_file)
        hid_dev = await iut_connect(shell, dut, device, address)

        def get_protocol_cb():
            # SUCCESS status with an empty payload -> Bumble sends a DATA message
            # whose HID payload is a bare header byte (no protocol-mode octet).
            return HIDDevice.GetSetStatus(
                data=b'',
                status=HIDDevice.GetSetReturn.SUCCESS,
            )

        hid_dev.get_protocol_cb = get_protocol_cb

        found, _ = await send_cmd_to_iut(
            shell, dut, "hid_host get_protocol", "HID Host: get_protocol failed"
        )
        assert found, "IUT did not report a malformed GET_PROTOCOL reply as an error"
        await iut_disconnect(shell, dut, device, hid_dev)


async def hid_case_set_report(hci_port, shell, dut, address, snoop_file):
    """SET_REPORT round trip: IUT sends, peer HANDSHAKEs, IUT reports the result."""
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = await setup_bumble_device(hci_transport, snoop_file)
        hid_dev = await iut_connect(shell, dut, device, address)

        def set_report_cb(report_id, report_type, report_size, report_data):
            return HIDDevice.GetSetStatus(status=HIDDevice.GetSetReturn.SUCCESS)

        hid_dev.set_report_cb = set_report_cb

        # set_report <type> <hex...> : OUTPUT(2), report id 0x02 + payload 0xAB
        found, _ = await send_cmd_to_iut(
            shell,
            dut,
            f"hid_host set_report {Message.ReportType.OUTPUT_REPORT} 02 AB",
            "HID Host: set_report done",
        )
        assert found, "IUT did not report the SET_REPORT handshake"
        await iut_disconnect(shell, dut, device, hid_dev)


async def hid_case_connection_release(hci_port, shell, dut, address, snoop_file):
    """Host-initiated connection release, HID11/HOS/HCR/BV-01-C.

    HID spec v1.1.2 Section 5.2.2: the Interrupt channel is closed before the
    Control channel. The association has to be usable again afterwards, which is
    only true if the pool entry was released once L2CAP was done with both
    channels, and the application must be told exactly once.
    """
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = await setup_bumble_device(hci_transport, snoop_file)
        hid_dev = await iut_connect(shell, dut, device, address)

        closed = []
        assert hid_dev.l2cap_ctrl_channel is not None
        assert hid_dev.l2cap_intr_channel is not None
        hid_dev.l2cap_ctrl_channel.on(
            ClassicChannel.EVENT_CLOSE, lambda: closed.append(HID_CONTROL_PSM)
        )
        hid_dev.l2cap_intr_channel.on(
            ClassicChannel.EVENT_CLOSE, lambda: closed.append(HID_INTERRUPT_PSM)
        )

        shell.exec_command("hid_host disconnect")
        found, lines = await wait_for_shell_response(dut, "HID Host: disconnected")
        assert found, "IUT did not report the release"
        # One association, one disconnected callback.
        assert sum(1 for line in lines if "HID Host: disconnected" in line) == 1

        assert closed == [HID_INTERRUPT_PSM, HID_CONTROL_PSM], (
            f"channels closed in order {closed}, expected interrupt before control"
        )

        # The pool entry has to be free again: reconnect on the same ACL.
        found, _ = await send_cmd_to_iut(
            shell, dut, "hid_host connect", "HID Host: connected", max_time=20
        )
        assert found, "reconnect after release failed"
        await iut_disconnect(shell, dut, device, hid_dev)


async def hid_case_device_initiated_release(hci_port, shell, dut, address, snoop_file):
    """Device-initiated connection release, HID11/HOS/HCR/BV-02-C.

    HID spec v1.1.2 Section 5.2.2: the Interrupt channel is closed first, so the
    peer only closes that one and the IUT is expected to close the Control
    channel itself rather than leave the association half open. The application
    is told once, and the pool entry has to be usable again afterwards.
    """
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = await setup_bumble_device(hci_transport, snoop_file)
        hid_dev = await iut_connect(shell, dut, device, address)

        closed = []
        assert hid_dev.l2cap_ctrl_channel is not None
        hid_dev.l2cap_ctrl_channel.on(
            ClassicChannel.EVENT_CLOSE, lambda: closed.append(HID_CONTROL_PSM)
        )

        # Fire and forget: if the IUT never answers, this case has to fail on the
        # missing callback rather than block on the peer.
        disconnecting = asyncio.ensure_future(hid_dev.disconnect_interrupt_channel())

        found, lines = await wait_for_shell_response(dut, "HID Host: disconnected", max_time=20)
        assert found, "IUT did not report the release"
        assert sum(1 for line in lines if "HID Host: disconnected" in line) == 1, (
            "the application was told more than once"
        )
        with contextlib.suppress(Exception):
            await asyncio.wait_for(disconnecting, timeout=5)

        assert closed == [HID_CONTROL_PSM], "IUT left the control channel open"

        # The pool entry has to be free again.
        found, _ = await send_cmd_to_iut(
            shell, dut, "hid_host connect", "HID Host: connected", max_time=20
        )
        assert found, "reconnect after the device initiated release failed"
        await iut_disconnect(shell, dut, device, hid_dev)


async def hid_case_sdp_discover(hci_port, shell, dut, address, snoop_file):
    """Reading the HID service record over SDP, twice.

    The profile does not read the record, so the shell does it: it needs the
    report descriptor to tell whether reports carry a Report ID (HID spec v1.1.2
    Section 3.1.2.3). The query is run twice on the same connection because the
    SDP client keeps the request parameters until it has called back, so a second
    query must find them released rather than still outstanding.
    """
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = await setup_bumble_device(
            hci_transport, snoop_file, sdp_records=SDP_SERVICE_RECORDS_HID
        )
        hid_dev = await iut_connect(shell, dut, device, address)

        for attempt in range(2):
            found, lines = await send_cmd_to_iut(
                shell, dut, "hid_host sdp_discover", "Report IDs in use: yes", max_time=20
            )
            assert found, f"attempt {attempt + 1}: report descriptor not read"
            assert not any("No SDP HID data" in line for line in lines), (
                f"attempt {attempt + 1}: the query was also reported as failed"
            )
            assert not any("already outstanding" in line for line in lines), (
                f"attempt {attempt + 1}: the previous query was never released"
            )

        await iut_disconnect(shell, dut, device, hid_dev)


class TestHidHost:
    def test_hid_host_get_report(self, shell: Shell, dut: DeviceAdapter, hid_host_dut):
        logger.info(f'test_hid_host_get_report {hid_host_dut}')
        hci_port, address = hid_host_dut
        with open("bumble_hid_get_report.btsnoop", "wb") as snoop_file:
            asyncio.run(hid_case_get_report(hci_port, shell, dut, address, snoop_file))

    def test_hid_host_get_protocol(self, shell: Shell, dut: DeviceAdapter, hid_host_dut):
        logger.info(f'test_hid_host_get_protocol {hid_host_dut}')
        hci_port, address = hid_host_dut
        with open("bumble_hid_get_protocol.btsnoop", "wb") as snoop_file:
            asyncio.run(hid_case_get_protocol(hci_port, shell, dut, address, snoop_file))

    def test_hid_host_get_protocol_malformed(self, shell: Shell, dut: DeviceAdapter, hid_host_dut):
        logger.info(f'test_hid_host_get_protocol_malformed {hid_host_dut}')
        hci_port, address = hid_host_dut
        with open("bumble_hid_get_protocol_malformed.btsnoop", "wb") as snoop_file:
            asyncio.run(hid_case_get_protocol_malformed(hci_port, shell, dut, address, snoop_file))

    def test_hid_host_set_report(self, shell: Shell, dut: DeviceAdapter, hid_host_dut):
        logger.info(f'test_hid_host_set_report {hid_host_dut}')
        hci_port, address = hid_host_dut
        with open("bumble_hid_set_report.btsnoop", "wb") as snoop_file:
            asyncio.run(hid_case_set_report(hci_port, shell, dut, address, snoop_file))

    def test_hid_host_connection_release(self, shell: Shell, dut: DeviceAdapter, hid_host_dut):
        logger.info(f'test_hid_host_connection_release {hid_host_dut}')
        hci_port, address = hid_host_dut
        with open("bumble_hid_connection_release.btsnoop", "wb") as snoop_file:
            asyncio.run(hid_case_connection_release(hci_port, shell, dut, address, snoop_file))

    def test_hid_host_device_initiated_release(
        self, shell: Shell, dut: DeviceAdapter, hid_host_dut
    ):
        logger.info(f'test_hid_host_device_initiated_release {hid_host_dut}')
        hci_port, address = hid_host_dut
        with open("bumble_hid_device_initiated_release.btsnoop", "wb") as snoop_file:
            asyncio.run(
                hid_case_device_initiated_release(hci_port, shell, dut, address, snoop_file)
            )

    def test_hid_host_sdp_discover(self, shell: Shell, dut: DeviceAdapter, hid_host_dut):
        logger.info(f'test_hid_host_sdp_discover {hid_host_dut}')
        hci_port, address = hid_host_dut
        with open("bumble_hid_sdp_discover.btsnoop", "wb") as snoop_file:
            asyncio.run(hid_case_sdp_discover(hci_port, shell, dut, address, snoop_file))

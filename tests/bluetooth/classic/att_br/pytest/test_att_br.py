# Copyright (c) 2026 Xiaomi Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""ATT bearer over BR/EDR (GATT over BR/EDR).

Bumble has no GATT client or server on BR/EDR, so the peer exchanges raw ATT
PDUs over an L2CAP channel on the ATT PSM.
"""

import asyncio
import contextlib
import logging
import struct
import sys

from bumble.att import (
    ATT_PDU,
    ATT_Error_Response,
    ATT_Exchange_MTU_Request,
    ATT_Find_Information_Request,
    ATT_Find_Information_Response,
    ATT_Handle_Value_Notification,
    ATT_Read_By_Group_Type_Request,
    ATT_Read_By_Group_Type_Response,
    ATT_Read_By_Type_Request,
    ATT_Read_By_Type_Response,
    ATT_Read_Request,
    ATT_Read_Response,
    ATT_Signed_Write_Command,
    ATT_Write_Request,
    ATT_Write_Response,
    ErrorCode,
    Opcode,
)
from bumble.core import BT_ATT_PROTOCOL_ID, BT_BR_EDR_TRANSPORT, BT_L2CAP_PROTOCOL_ID, UUID
from bumble.device import Device
from bumble.gatt import (
    GATT_CHARACTERISTIC_ATTRIBUTE_TYPE,
    GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_DESCRIPTOR,
    GATT_DEVICE_NAME_CHARACTERISTIC,
    GATT_GENERIC_ACCESS_SERVICE,
    GATT_GENERIC_ATTRIBUTE_SERVICE,
    GATT_PRIMARY_SERVICE_ATTRIBUTE_TYPE,
)
from bumble.hci import Address, HCI_Write_Page_Timeout_Command
from bumble.l2cap import ClassicChannelSpec
from bumble.pairing import PairingConfig, PairingDelegate
from bumble.sdp import (
    SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID,
    SDP_SERVICE_CLASS_ID_LIST_ATTRIBUTE_ID,
)
from bumble.sdp import Client as SDP_Client
from bumble.snoop import BtSnooper
from bumble.transport import open_transport_or_link
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)

ATT_PSM = 0x001F
# Local L2CAP MTU of the Bumble peer, the ATT MTU is the minimum of both sides
PEER_MTU = 100
DEFAULT_LE_ATT_MTU = 23

TEST_SVC_UUID = UUID('9B0A0000-7C1E-4C64-9A3B-5F2D1E8C0A01')
TEST_VAL_UUID = UUID('9B0A0001-7C1E-4C64-9A3B-5F2D1E8C0A01')
TEST_AUTH_UUID = UUID('9B0A0002-7C1E-4C64-9A3B-5F2D1E8C0A01')
TEST_HIDDEN_SVC_UUID = UUID('9B0A0010-7C1E-4C64-9A3B-5F2D1E8C0A01')
BATTERY_SERVICE_UUID = UUID.from_16_bits(0x180F)


async def wait_for_shell_response(dut, message, max_time=10):
    found = False
    lines = []
    for _ in range(0, max_time):
        read_lines = dut.readlines()
        lines += read_lines
        if any(message in line for line in read_lines):
            found = True
            break
        await asyncio.sleep(1)
    return found, lines


async def send_cmd_to_iut(shell, dut, cmd, parse=None, max_time=10):
    lines = shell.exec_command(cmd)
    found = True
    if parse is not None:
        found = any(parse in line for line in lines)
        if not found:
            found, more = await wait_for_shell_response(dut, parse, max_time)
            lines += more
    logger.info(f'{cmd}: {lines}')
    assert found, f'"{parse}" not found after "{cmd}"'
    return lines


async def assert_no_shell_response(dut, message, max_time=2):
    found, lines = await wait_for_shell_response(dut, message, max_time)
    assert not found, f'unexpected "{message}": {lines}'


class AttBearer:
    """ATT client on a Classic L2CAP channel, one outstanding request at a time."""

    def __init__(self, channel):
        self.channel = channel
        self.responses = asyncio.Queue()
        self.notifications = asyncio.Queue()
        channel.sink = self.on_pdu

    def on_pdu(self, pdu):
        att_pdu = ATT_PDU.from_bytes(pdu)
        logger.info(f'ATT rx {att_pdu}')
        if att_pdu.op_code == Opcode.ATT_HANDLE_VALUE_NOTIFICATION:
            self.notifications.put_nowait(att_pdu)
        else:
            self.responses.put_nowait((att_pdu, len(pdu)))

    def send(self, pdu):
        logger.info(f'ATT tx {pdu}')
        self.channel.write(bytes(pdu))

    async def request(self, pdu, timeout=5):
        self.send(pdu)
        response, _ = await asyncio.wait_for(self.responses.get(), timeout)
        return response

    async def request_with_len(self, pdu, timeout=5):
        self.send(pdu)
        return await asyncio.wait_for(self.responses.get(), timeout)

    async def primary_services(self):
        services = []
        start = 0x0001
        while start <= 0xFFFF:
            rsp = await self.request(
                ATT_Read_By_Group_Type_Request(
                    starting_handle=start,
                    ending_handle=0xFFFF,
                    attribute_group_type=GATT_PRIMARY_SERVICE_ATTRIBUTE_TYPE,
                )
            )
            if isinstance(rsp, ATT_Error_Response):
                assert rsp.error_code == ErrorCode.ATTRIBUTE_NOT_FOUND
                break
            assert isinstance(rsp, ATT_Read_By_Group_Type_Response)
            for handle, end, value in rsp.attributes:
                services.append((UUID.from_bytes(value), handle, end))
                start = end + 1
        return services

    async def characteristics(self, start, end):
        chars = []
        while start <= end:
            rsp = await self.request(
                ATT_Read_By_Type_Request(
                    starting_handle=start,
                    ending_handle=end,
                    attribute_type=GATT_CHARACTERISTIC_ATTRIBUTE_TYPE,
                )
            )
            if isinstance(rsp, ATT_Error_Response):
                assert rsp.error_code == ErrorCode.ATTRIBUTE_NOT_FOUND
                break
            assert isinstance(rsp, ATT_Read_By_Type_Response)
            for handle, value in rsp.attributes:
                properties, value_handle = struct.unpack_from('<BH', value)
                chars.append((UUID.from_bytes(value[3:]), handle, value_handle, properties))
                start = handle + 1
        return chars

    async def find_descriptor(self, start, end, uuid):
        rsp = await self.request(
            ATT_Find_Information_Request(starting_handle=start, ending_handle=end)
        )
        assert isinstance(rsp, ATT_Find_Information_Response)
        for handle, value in rsp.information:
            if UUID.from_bytes(value) == uuid:
                return handle
        return None


async def open_peer(hci_transport, snoop_file):
    device = Device.with_hci(
        'Bumble', Address('F0:F1:F2:F3:F4:F5'), hci_transport.source, hci_transport.sink
    )
    device.classic_enabled = True
    device.le_enabled = False
    delegate = PairingDelegate(PairingDelegate.IoCapability.NO_OUTPUT_NO_INPUT)
    device.pairing_config_factory = lambda connection: PairingConfig(
        sc=True, mitm=False, bonding=True, delegate=delegate
    )
    device.host.snooper = BtSnooper(snoop_file)
    while True:
        try:
            await device.power_on()
            break
        except Exception:
            continue
    await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
    return device


async def connect_encrypted(shell, dut, device, iut_address):
    await send_cmd_to_iut(shell, dut, "br clear all")
    connection = await device.connect(iut_address.split(" ")[0], transport=BT_BR_EDR_TRANSPORT)
    found, _ = await wait_for_shell_response(dut, "Connected:")
    assert found
    await device.authenticate(connection)
    await device.encrypt(connection)
    found, _ = await wait_for_shell_response(dut, "Security changed:")
    assert found
    return connection


async def disconnect(dut, device, connection):
    await device.disconnect(connection, reason=0x13)
    found, _ = await wait_for_shell_response(dut, "Disconnected:")
    assert found


async def open_att(connection):
    channel = await connection.create_l2cap_channel(
        spec=ClassicChannelSpec(psm=ATT_PSM, mtu=PEER_MTU)
    )
    return channel, AttBearer(channel)


async def find_test_char(bearer):
    services = await bearer.primary_services()
    svc = next(s for s in services if s[0] == TEST_SVC_UUID)
    chars = await bearer.characteristics(svc[1], svc[2])
    val = next(c for c in chars if c[0] == TEST_VAL_UUID)
    auth = next(c for c in chars if c[0] == TEST_AUTH_UUID)
    ccc = await bearer.find_descriptor(
        val[2] + 1, svc[2], GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_DESCRIPTOR
    )
    assert ccc is not None
    return val[2], ccc, auth[2]


async def sdp_records_match_discovery(hci_port, shell, dut, address, snoop_file):
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = await open_peer(hci_transport, snoop_file)
        connection = await connect_encrypted(shell, dut, device, address)

        sdp_client = SDP_Client(connection)
        await sdp_client.connect()
        records = await sdp_client.search_attributes(
            [BT_ATT_PROTOCOL_ID],
            [SDP_SERVICE_CLASS_ID_LIST_ATTRIBUTE_ID, SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID],
        )
        await sdp_client.disconnect()

        sdp_services = {}
        for record in records:
            attrs = {attr.id: attr.value for attr in record}
            uuid = attrs[SDP_SERVICE_CLASS_ID_LIST_ATTRIBUTE_ID].value[0].value
            l2cap, att = attrs[SDP_PROTOCOL_DESCRIPTOR_LIST_ATTRIBUTE_ID].value
            assert l2cap.value[0].value == BT_L2CAP_PROTOCOL_ID
            assert l2cap.value[1].value == ATT_PSM
            assert att.value[0].value == BT_ATT_PROTOCOL_ID
            sdp_services[uuid] = (att.value[1].value, att.value[2].value)
        logger.info(f'SDP services {sdp_services}')

        _, bearer = await open_att(connection)
        services = await bearer.primary_services()
        logger.info(f'ATT services {services}')

        # GAP, GATT and the opted-in test service are published with their handle range, the
        # other services are only found with ATT
        published = (GATT_GENERIC_ATTRIBUTE_SERVICE, GATT_GENERIC_ACCESS_SERVICE, TEST_SVC_UUID)
        att_services = {uuid: (start, end) for uuid, start, end in services}
        assert TEST_HIDDEN_SVC_UUID in att_services
        assert {uuid: att_services[uuid] for uuid in published} == sdp_services

        await disconnect(dut, device, connection)


async def server_procedures(hci_port, shell, dut, address, snoop_file):
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = await open_peer(hci_transport, snoop_file)
        connection = await connect_encrypted(shell, dut, device, address)
        channel, bearer = await open_att(connection)

        # The ATT MTU comes from the L2CAP configuration: responses are longer
        # than the default LE ATT MTU but fit the MTU of the peer.
        rsp, length = await bearer.request_with_len(
            ATT_Read_By_Type_Request(
                starting_handle=0x0001,
                ending_handle=0xFFFF,
                attribute_type=GATT_CHARACTERISTIC_ATTRIBUTE_TYPE,
            )
        )
        assert isinstance(rsp, ATT_Read_By_Type_Response)
        logger.info(f'Read By Type response of {length} octets, peer MTU {channel.peer_mtu}')
        assert DEFAULT_LE_ATT_MTU < length <= min(PEER_MTU, channel.peer_mtu)

        rsp = await bearer.request(
            ATT_Read_By_Type_Request(
                starting_handle=0x0001,
                ending_handle=0xFFFF,
                attribute_type=GATT_DEVICE_NAME_CHARACTERISTIC,
            )
        )
        assert isinstance(rsp, ATT_Read_By_Type_Response)
        assert rsp.attributes[0][1] == b'att_br'

        value_handle, _, auth_handle = await find_test_char(bearer)

        rsp = await bearer.request(ATT_Read_Request(attribute_handle=value_handle))
        assert isinstance(rsp, ATT_Read_Response)
        assert rsp.attribute_value == b'att_br'

        rsp = await bearer.request(
            ATT_Write_Request(attribute_handle=value_handle, attribute_value=b'written')
        )
        assert isinstance(rsp, ATT_Write_Response)
        rsp = await bearer.request(ATT_Read_Request(attribute_handle=value_handle))
        assert rsp.attribute_value == b'written'

        # Just Works pairing: the link is not authenticated
        rsp = await bearer.request(ATT_Read_Request(attribute_handle=auth_handle))
        assert isinstance(rsp, ATT_Error_Response)
        assert rsp.error_code == ErrorCode.INSUFFICIENT_AUTHENTICATION

        # Exchange MTU is not used on BR/EDR (Core Vol 3, Part G, 4.3.1)
        rsp = await bearer.request(ATT_Exchange_MTU_Request(client_rx_mtu=PEER_MTU))
        assert isinstance(rsp, ATT_Error_Response)
        assert rsp.request_opcode_in_error == Opcode.ATT_EXCHANGE_MTU_REQUEST
        assert rsp.error_code == ErrorCode.REQUEST_NOT_SUPPORTED

        # Signed Write is not used on BR/EDR: the command is ignored
        bearer.send(
            ATT_Signed_Write_Command(
                attribute_handle=value_handle, attribute_value=b'signed' + bytes(12)
            )
        )
        with contextlib.suppress(TimeoutError):
            unexpected = await asyncio.wait_for(bearer.responses.get(), 1)
            raise AssertionError(f'response to Signed Write: {unexpected}')
        rsp = await bearer.request(ATT_Read_Request(attribute_handle=value_handle))
        assert rsp.attribute_value == b'written'

        await disconnect(dut, device, connection)


async def notify_and_ccc_restore(hci_port, shell, dut, address, snoop_file):
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = await open_peer(hci_transport, snoop_file)
        connection = await connect_encrypted(shell, dut, device, address)
        channel, bearer = await open_att(connection)

        value_handle, ccc_handle, _ = await find_test_char(bearer)

        rsp = await bearer.request(
            ATT_Write_Request(attribute_handle=ccc_handle, attribute_value=b'\x01\x00')
        )
        assert isinstance(rsp, ATT_Write_Response)

        await send_cmd_to_iut(shell, dut, f"gatt notify {value_handle:x} 0a0b")
        ntf = await asyncio.wait_for(bearer.notifications.get(), 5)
        assert isinstance(ntf, ATT_Handle_Value_Notification)
        assert ntf.attribute_handle == value_handle
        assert ntf.attribute_value == b'\x0a\x0b'

        # Closing the ATT bearer keeps the ACL connection up
        await channel.disconnect()
        await assert_no_shell_response(dut, "Disconnected:")

        # The peer is bonded: the CCC value is kept for the next bearer
        _, bearer = await open_att(connection)
        await asyncio.sleep(1)
        await send_cmd_to_iut(shell, dut, f"gatt notify {value_handle:x} 0c0d")
        ntf = await asyncio.wait_for(bearer.notifications.get(), 5)
        assert ntf.attribute_handle == value_handle
        assert ntf.attribute_value == b'\x0c\x0d'

        await disconnect(dut, device, connection)


class AttServer:
    """Minimal ATT server on a Classic L2CAP channel with a single service."""

    def __init__(self, channel):
        self.channel = channel
        channel.sink = self.on_pdu

    def on_pdu(self, pdu):
        request = ATT_PDU.from_bytes(pdu)
        logger.info(f'ATT server rx {request}')
        if (
            isinstance(request, ATT_Read_By_Group_Type_Request)
            and request.attribute_group_type == GATT_PRIMARY_SERVICE_ATTRIBUTE_TYPE
            and request.starting_handle <= 0x0001
        ):
            data = struct.pack('<HH', 0x0001, 0x0003) + BATTERY_SERVICE_UUID.to_bytes()
            response = ATT_Read_By_Group_Type_Response(length=len(data), attribute_data_list=data)
        else:
            error = (
                ErrorCode.ATTRIBUTE_NOT_FOUND
                if isinstance(request, ATT_Read_By_Group_Type_Request)
                else ErrorCode.REQUEST_NOT_SUPPORTED
            )
            response = ATT_Error_Response(
                request_opcode_in_error=request.op_code,
                attribute_handle_in_error=0x0000,
                error_code=error,
            )
        logger.info(f'ATT server tx {response}')
        self.channel.write(bytes(response))


async def client_procedures(hci_port, shell, dut, address, snoop_file):
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = await open_peer(hci_transport, snoop_file)

        opened = asyncio.get_running_loop().create_future()
        closed = asyncio.get_running_loop().create_future()

        def on_channel(channel):
            AttServer(channel)
            channel.on(channel.EVENT_OPEN, lambda: opened.done() or opened.set_result(channel))
            channel.on(channel.EVENT_CLOSE, lambda: closed.done() or closed.set_result(None))

        device.create_l2cap_server(spec=ClassicChannelSpec(psm=ATT_PSM), handler=on_channel)

        connection = await connect_encrypted(shell, dut, device, address)

        await send_cmd_to_iut(shell, dut, "br att-connect", "ATT connection pending")
        # Shell commands block the event loop: let the L2CAP configuration
        # complete on both sides before the next one.
        await asyncio.wait_for(opened, 10)
        await asyncio.sleep(1)

        await send_cmd_to_iut(
            shell,
            dut,
            "gatt discover-primary",
            "Service 180f found: start handle 1, end_handle 3",
        )
        await send_cmd_to_iut(shell, dut, "gatt exchange-mtu", "Exchange failed (err -95)")

        # A second bearer on the same connection is rejected
        await send_cmd_to_iut(shell, dut, "br att-connect", "ATT connect failed (err -")

        await send_cmd_to_iut(shell, dut, "br att-disconnect")
        await asyncio.wait_for(closed, 10)
        await assert_no_shell_response(dut, "Disconnected:")

        await disconnect(dut, device, connection)


class TestAttBr:
    def test_sdp_records(self, shell: Shell, dut: DeviceAdapter, att_br_dut):
        """SDP records of the GAP, GATT and opted-in services match the ATT discovery."""
        hci, iut_address = att_br_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sdp_records_match_discovery(hci, shell, dut, iut_address, snoop_file))

    def test_server_procedures(self, shell: Shell, dut: DeviceAdapter, att_br_dut):
        """ATT MTU, read, write, security, Exchange MTU and Signed Write on the IUT server."""
        hci, iut_address = att_br_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(server_procedures(hci, shell, dut, iut_address, snoop_file))

    def test_notify_ccc_restore(self, shell: Shell, dut: DeviceAdapter, att_br_dut):
        """Notification, bearer close without ACL close, bonded CCC kept on reopen."""
        hci, iut_address = att_br_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(notify_and_ccc_restore(hci, shell, dut, iut_address, snoop_file))

    def test_client_procedures(self, shell: Shell, dut: DeviceAdapter, att_br_dut):
        """IUT client: connect, discover, Exchange MTU rejected, disconnect the bearer."""
        hci, iut_address = att_br_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(client_procedures(hci, shell, dut, iut_address, snoop_file))

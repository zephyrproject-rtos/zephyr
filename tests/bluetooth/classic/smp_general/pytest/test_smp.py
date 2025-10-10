# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0


import asyncio
import logging
import random
import re
import sys

from bumble.core import (
    BT_ADVANCED_AUDIO_DISTRIBUTION_SERVICE,
    BT_AUDIO_SOURCE_SERVICE,
    BT_AVDTP_PROTOCOL_ID,
    BT_BR_EDR_TRANSPORT,
    BT_L2CAP_PROTOCOL_ID,
    ProtocolError,
    UnreachableError,
)
from bumble.device import (
    Device,
    host_event_handler,
    with_connection_from_address,
)
from bumble.hci import (
    HCI_REMOTE_USER_TERMINATED_CONNECTION_ERROR,
    Address,
    HCI_User_Confirmation_Request_Negative_Reply_Command,
    HCI_User_Confirmation_Request_Reply_Command,
    HCI_Write_Page_Timeout_Command,
)
from bumble.l2cap import ClassicChannel, ClassicChannelSpec, L2CAP_Connection_Response
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
from bumble.smp import (
    SMP_DISPLAY_ONLY_IO_CAPABILITY,
    SMP_DISPLAY_YES_NO_IO_CAPABILITY,
    SMP_KEYBOARD_ONLY_IO_CAPABILITY,
    SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY,
)
from bumble.snoop import BtSnooper
from bumble.transport import open_transport_or_link
from bumble.utils import AsyncRunner
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)

l2cap_server_psm = 0x1001

JUST_WORKS = 0
PASSKEY_ENTRY = 1
PASSKEY_DISPLAY = 2
PASSKEY_CONFIRM = 3

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


# power on dongle
async def device_power_on(device) -> None:
    while True:
        try:
            await device.power_on()
            break
        except Exception:
            continue


def check_shell_response(lines: list[str], regex: str) -> bool:
    found = False
    try:
        for line in lines:
            if re.search(regex, line):
                found = True
                break
    except Exception as e:
        logger.error(f'{e}!', exc_info=True)
        raise e

    return found


def search_messages(result, messages, read_lines):
    for message in messages:
        for line in read_lines:
            if re.search(message, line):
                result[message] = True
                if False not in result.values():
                    return True
                break

    return False


async def wait_for_shell_response(dut, regex: list[str] | str, max_wait_sec=10):
    found = False
    lines = []

    logger.debug('wait_for_shell_response')

    messages = [regex] if isinstance(regex, str) else regex
    result = dict.fromkeys(messages, False)

    try:
        for _ in range(0, max_wait_sec):
            read_lines = dut.readlines(print_output=True)
            lines += read_lines
            for line in read_lines:
                logger.debug(f'DUT response: {str(line)}')

            found = search_messages(result, messages, read_lines)
            if found is True:
                break
            await asyncio.sleep(1)
    except Exception as e:
        logger.error(f'{e}!', exc_info=True)
        raise e

    for key in result:
        logger.debug(f'Expected DUT response: "{key}", Matched: {result[key]}')

    return found, lines


async def send_cmd_to_iut(shell, dut, cmd, parse=None):
    lines = shell.exec_command(cmd)
    if parse is not None:
        found, lines = await wait_for_shell_response(dut, parse)
    else:
        found = True
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


async def bumble_l2cap_br_connect(connection):
    channel = None
    chan_conn_res = 'success'
    try:
        channel = await connection.create_l2cap_channel(
            spec=ClassicChannelSpec(psm=l2cap_server_psm)
        )
    except BaseException as e:
        if isinstance(e, asyncio.exceptions.CancelledError):
            chan_conn_res = e.args[0]
            logger.info(
                f'Fail to create L2CAP channel, exception type: {type(e)}, reason: {chan_conn_res}'
            )
            await asyncio.sleep(1)  # Added to avoid transport lost with PTS dongle
        elif isinstance(e, ProtocolError):
            chan_conn_res = e.error_name
            logger.info(
                f'Fail to create L2CAP channel, exception type: {type(e)}, reason: {chan_conn_res}'
            )
        else:
            chan_conn_res = 'unexpected error'
            raise e

    return channel, chan_conn_res


# Override methods in Device class
class SmpDevice(Device):
    # [Classic only]
    @host_event_handler
    @with_connection_from_address
    def on_authentication_user_confirmation_request(self, connection, code) -> None:
        # Ask what the pairing config should be for this connection
        pairing_config = self.pairing_config_factory(connection)
        io_capability = pairing_config.delegate.classic_io_capability
        peer_io_capability = connection.peer_pairing_io_capability

        async def confirm() -> bool:
            # Ask the user to confirm the pairing, without display
            return await pairing_config.delegate.confirm()

        async def auto_confirm() -> bool:
            # Ask the user to auto-confirm the pairing, without display
            return await pairing_config.delegate.confirm(auto=True)

        async def display_confirm() -> bool:
            # Display the code and ask the user to compare
            return await pairing_config.delegate.compare_numbers(code, digits=6)

        async def display_auto_confirm() -> bool:
            # Display the code to the user and ask the delegate to auto-confirm
            await pairing_config.delegate.display_number(code, digits=6)
            return await pairing_config.delegate.confirm(auto=True)

        async def na() -> bool:
            raise UnreachableError()

        # See Bluetooth spec @ Vol 3, Part C 5.2.2.6
        methods = {
            SMP_DISPLAY_ONLY_IO_CAPABILITY: {
                SMP_DISPLAY_ONLY_IO_CAPABILITY: display_auto_confirm,
                SMP_DISPLAY_YES_NO_IO_CAPABILITY: display_confirm,
                SMP_KEYBOARD_ONLY_IO_CAPABILITY: na,
                SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY: auto_confirm,
            },
            SMP_DISPLAY_YES_NO_IO_CAPABILITY: {
                SMP_DISPLAY_ONLY_IO_CAPABILITY: display_auto_confirm,
                SMP_DISPLAY_YES_NO_IO_CAPABILITY: display_confirm,
                SMP_KEYBOARD_ONLY_IO_CAPABILITY: na,
                SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY: auto_confirm,
            },
            SMP_KEYBOARD_ONLY_IO_CAPABILITY: {
                SMP_DISPLAY_ONLY_IO_CAPABILITY: na,
                SMP_DISPLAY_YES_NO_IO_CAPABILITY: auto_confirm,
                SMP_KEYBOARD_ONLY_IO_CAPABILITY: na,
                SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY: auto_confirm,
            },
            SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY: {
                SMP_DISPLAY_ONLY_IO_CAPABILITY: confirm,
                SMP_DISPLAY_YES_NO_IO_CAPABILITY: confirm,
                SMP_KEYBOARD_ONLY_IO_CAPABILITY: auto_confirm,
                SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY: auto_confirm,
            },
        }

        method = methods[peer_io_capability][io_capability]

        async def reply() -> None:
            try:
                if await connection.abort_on('disconnection', method()):
                    await self.host.send_command(
                        HCI_User_Confirmation_Request_Reply_Command(bd_addr=connection.peer_address)
                    )
                    return
            except Exception as error:
                logger.warning(f'exception while confirming: {error}')

            await self.host.send_command(
                HCI_User_Confirmation_Request_Negative_Reply_Command(
                    bd_addr=connection.peer_address
                )
            )

        AsyncRunner.spawn(reply())

    # [Classic only]
    @host_event_handler
    @with_connection_from_address
    def on_authentication_user_passkey_notification(self, connection, passkey):
        # Ask what the pairing config should be for this connection
        pairing_config = self.pairing_config_factory(connection)

        # Show the passkey to the user
        connection.abort_on(
            'disconnection', pairing_config.delegate.display_number(passkey, digits=6)
        )


async def sm_reset_board(shell, dut):
    logger.info("Reset board...")
    command = "test_smp reboot"
    dut.write(command.encode())
    if not shell.wait_for_prompt():
        logger.error('Prompt not found')
    if dut.device_config.type == 'hardware':
        # after booting up the device, there might appear additional logs
        # after first prompt, so we need to wait and clear the buffer
        await asyncio.sleep(0.5)
        dut.clear_buffer()


def sm_get_pairing_method(local_sec, local_io_cap, remote_sec, remote_io_cap):
    pairing_method = [
        [JUST_WORKS, JUST_WORKS, PASSKEY_ENTRY, JUST_WORKS],
        [JUST_WORKS, PASSKEY_CONFIRM, PASSKEY_ENTRY, JUST_WORKS],
        [PASSKEY_DISPLAY, PASSKEY_DISPLAY, PASSKEY_ENTRY, JUST_WORKS],
        [JUST_WORKS, JUST_WORKS, JUST_WORKS, JUST_WORKS],
    ]
    method = pairing_method[remote_io_cap][local_io_cap]

    if remote_sec < 3 and local_sec < 3:
        method = JUST_WORKS

    return method


async def sm_passkey_confirm(shell, dut, number, digits):
    logger.debug(f'Passkey shown on Bumble: {number}')

    passkey = 0
    regex = r"Confirm passkey for .*?: (?P<passkey>\d{6})"
    found, lines = await wait_for_shell_response(dut, regex)
    if found is True:
        for line in lines:
            searched = re.search(regex, line)
            if searched:
                passkey = int(searched.group("passkey"))
                break

    logger.debug(f'Passkey shown on DUT: {passkey}')
    if number == passkey:
        logger.info('Passkey matched')
        shell.exec_command("bt auth-passkey-confirm")
        return True
    else:
        logger.info('Passkey not matched')
        return False


async def sm_bumble_passkey_entry(dut):
    passkey = None
    regex = r"Passkey for .*?: (?P<passkey>\d{6})"
    found, lines = await wait_for_shell_response(dut, regex)
    if found is True:
        for line in lines:
            searched = re.search(regex, line)
            if searched:
                passkey = int(searched.group("passkey"))
                break

    logger.info(f'Passkey = {passkey}')

    return passkey


async def sm_init_io_cap(
    hci_port, shell, dut, address, snoop_file, dut_sec, dut_io_cap, bumble_sec, bumble_io_cap
) -> None:
    class Delegate(PairingDelegate):
        def __init__(
            self,
            io_capability,
        ):
            super().__init__(
                io_capability,
            )
            self.reset()

        def reset(self):
            self.dut_passkey_entry = asyncio.get_running_loop().create_future()
            self.dut_passkey_confirm = asyncio.get_running_loop().create_future()
            self.bumble_passkey_entry = asyncio.get_running_loop().create_future()

        async def display_number(self, number, digits):
            """Display a number."""
            logger.info(f'Displaying number: {number}')
            if dut_method == PASSKEY_ENTRY:
                found, _ = await wait_for_shell_response(dut, "Enter passkey")
                assert found is True
                shell.exec_command(f"bt auth-passkey {number}")
                self.dut_passkey_entry.set_result(True)

        async def compare_numbers(self, number: int, digits: int) -> bool:
            """Compare two numbers."""
            logger.info('Comparing number')
            if dut_method == PASSKEY_CONFIRM:
                result = await sm_passkey_confirm(shell, dut, number, digits)
                if result is True:
                    self.dut_passkey_confirm.set_result(True)
                return result
            else:
                return True

        async def get_number(self) -> int | None:
            """
            Return an optional number as an answer to a passkey request.
            Returning `None` will result in a negative reply.
            """
            logger.info('Getting number')
            passkey = await sm_bumble_passkey_entry(dut)
            if passkey is not None:
                self.bumble_passkey_entry.set_result(True)

            return passkey

    bt_auth = {
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY: 'none',
        SMP_KEYBOARD_ONLY_IO_CAPABILITY: 'input',
        SMP_DISPLAY_ONLY_IO_CAPABILITY: 'display',
        SMP_DISPLAY_YES_NO_IO_CAPABILITY: 'yesno',
    }
    dut_method = sm_get_pairing_method(dut_sec, dut_io_cap, bumble_sec, bumble_io_cap)
    bumble_method = sm_get_pairing_method(bumble_sec, bumble_io_cap, dut_sec, dut_io_cap)

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = bool(dut_sec != 3)

        delegate = Delegate(bumble_io_cap)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=bool(dut_sec != 3),
            mitm=bool(dut_sec > 2),
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, f"bt auth {bt_auth[dut_io_cap]}", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        lines = shell.exec_command(
            f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec {dut_sec}"
        )

        if dut_method == PASSKEY_ENTRY:
            await asyncio.wait_for(delegate.dut_passkey_entry, timeout=5.0)
        elif dut_method == PASSKEY_CONFIRM:
            await asyncio.wait_for(delegate.dut_passkey_confirm, timeout=5.0)
        elif bumble_method == PASSKEY_ENTRY:
            await asyncio.wait_for(delegate.bumble_passkey_entry, timeout=5.0)

        # If dut_sec is 1, the security level will be 2 when both devices support SSP
        dut_sec = 2 if dut_sec == 1 else dut_sec

        if dut_io_cap == SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY and dut_sec > 2:
            assert check_shell_response(lines, r"Channel \w+ disconnected")
            assert check_shell_response(lines, f"Unable to connect to psm {l2cap_server_psm}")

            shell.exec_command("bt disconnect")
            found, _ = await wait_for_shell_response(dut, f"Disconnected: {bumble_address}")
            assert found is True

        elif dut_method == JUST_WORKS and dut_sec > 2:
            found, _ = await wait_for_shell_response(
                dut,
                [
                    f"Pairing failed with {bumble_address} reason: Authentication failure",
                    r"Channel \w+ disconnected",
                    f"Disconnected: {bumble_address}",
                ],
            )
            assert found is True
            await asyncio.sleep(1)  # Added to avoid transport lost with PTS dongle

        else:
            found, _ = await wait_for_shell_response(
                dut,
                [
                    f"Bonded with {bumble_address}",
                    f"Security changed: {bumble_address} level {dut_sec}",
                    r"Channel \w+ connected",
                ],
            )
            assert found is True

            lines = shell.exec_command("test_smp security_info")
            assert check_shell_response(lines, "Encryption key size: 16")

            shell.exec_command("bt disconnect")
            found, _ = await wait_for_shell_response(
                dut, [r"Channel \w+ disconnected", f"Disconnected: {bumble_address}"]
            )
            assert found is True


async def sm_init_001(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_init_io_cap(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        2,
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY,
        2,
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY,
    )


async def sm_init_002(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_init_io_cap(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        3,
        SMP_DISPLAY_ONLY_IO_CAPABILITY,
        3,
        SMP_KEYBOARD_ONLY_IO_CAPABILITY,
    )


async def sm_init_003(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_init_io_cap(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        4,
        SMP_KEYBOARD_ONLY_IO_CAPABILITY,
        4,
        SMP_DISPLAY_ONLY_IO_CAPABILITY,
    )


async def sm_init_005(hci_port, shell, dut, address, snoop_file) -> None:
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

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = True

        delegate = PairingDelegate(SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True,
            mitm=False,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        await send_cmd_to_iut(shell, dut, f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec 2")

        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Bonded with {bumble_address}",
                f"Security changed: {bumble_address} level 2",
                r"Channel \w+ disconnected",
            ],
        )
        assert found is True

        await send_cmd_to_iut(shell, dut, "bt disconnect", f"Disconnected: {bumble_address}")

    # Restore the origin method
    ClassicChannel.on_connection_request = on_connection_request


async def sm_init_006(hci_port, shell, dut, address, snoop_file) -> None:
    class Delegate(PairingDelegate):
        async def compare_numbers(self, number: int, digits: int) -> bool:
            """Compare two numbers."""
            logger.info('Comparing number')
            return await sm_passkey_confirm(shell, dut, number, digits)

    def on_app_connection(connection):
        global br_conn
        logger.info(f'BR/EDR connected {connection}')
        br_conn = connection

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = True

        delegate = Delegate(SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True,
            mitm=False,
            bonding=True,
            delegate=delegate,
        )

        dut_sec = 2
        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]
        device.on('connection', on_app_connection)

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br register {format(l2cap_server_psm, 'x')}", None
        )
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br security {format(l2cap_server_psm, 'x')} {dut_sec}", None
        )
        await send_cmd_to_iut(shell, dut, f"bt security {dut_sec}", None)
        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Bonded with {bumble_address}",
                f"Security changed: {bumble_address} level {dut_sec}",
            ],
        )
        assert found is True

        dut_sec = 4
        delegate = Delegate(SMP_DISPLAY_YES_NO_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True,
            mitm=True,
            bonding=True,
            delegate=delegate,
        )

        await send_cmd_to_iut(shell, dut, "bt auth yesno", None)
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br security {format(l2cap_server_psm, 'x')} {dut_sec}", None
        )

        logger.info('Creating L2CAP channel...')
        await device.create_l2cap_channel(
            connection=br_conn, spec=ClassicChannelSpec(psm=l2cap_server_psm)
        )
        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Bonded with {bumble_address}",
                f"Security changed: {bumble_address} level {dut_sec}",
            ],
        )
        assert found is True

        shell.exec_command("bt disconnect")
        found, _ = await wait_for_shell_response(
            dut, [r"Channel \w+ disconnected", f"Disconnected: {bumble_address}"]
        )
        assert found is True


async def sm_init_008(hci_port, shell, dut, address, snoop_file) -> None:
    class Delegate(PairingDelegate):
        async def compare_numbers(self, number: int, digits: int) -> bool:
            """Compare two numbers."""
            logger.info('Force ACL disconnection during pairing')
            await device.disconnect(br_conn, HCI_REMOTE_USER_TERMINATED_CONNECTION_ERROR)
            return True

    def on_app_connection(connection):
        global br_conn
        logger.info(f'BR/EDR connected {connection}')
        br_conn = connection

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = True

        delegate = Delegate(SMP_DISPLAY_YES_NO_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True,
            mitm=False,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]
        device.on('connection', on_app_connection)

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt auth yesno", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        await send_cmd_to_iut(shell, dut, f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec 2")

        found, _ = await wait_for_shell_response(
            dut,
            [r"Channel \w+ disconnected", f"Disconnected: {bumble_address}"],
        )
        assert found is True


async def sm_init_011(hci_port, shell, dut, address, snoop_file) -> None:
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = True

        delegate = PairingDelegate(SMP_DISPLAY_ONLY_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True,
            mitm=False,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt auth display", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        await send_cmd_to_iut(shell, dut, f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec 2")

        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Bonded with {bumble_address}",
                f"Security changed: {bumble_address} level 2",
                r"Channel \w+ connected",
            ],
        )
        assert found is True

        shell.exec_command("bt disconnect")
        found, _ = await wait_for_shell_response(
            dut, [r"Channel \w+ disconnected", f"Disconnected: {bumble_address}"]
        )
        assert found is True

        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        await send_cmd_to_iut(shell, dut, f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec 2")

        found, lines = await wait_for_shell_response(
            dut,
            [f"Security changed: {bumble_address} level 2", r"Channel \w+ connected"],
        )
        assert found is True

        found = check_shell_response(lines, f"Bonded with {bumble_address}")
        assert found is False

        shell.exec_command("bt disconnect")
        found, _ = await wait_for_shell_response(
            dut, [r"Channel \w+ disconnected", f"Disconnected: {bumble_address}"]
        )
        assert found is True


async def sm_init_012(hci_port, shell, dut, address, snoop_file) -> None:
    class Delegate(PairingDelegate):
        def __init__(
            self,
            io_capability,
        ):
            super().__init__(
                io_capability,
            )
            self.reset()

        def reset(self):
            self.dut_passkey_confirm = asyncio.get_running_loop().create_future()

        async def compare_numbers(self, number: int, digits: int) -> bool:
            """Compare two numbers."""
            logger.info('Comparing number')
            if dut_sec > 2:
                result = await sm_passkey_confirm(shell, dut, number, digits)
                if result is True:
                    self.dut_passkey_confirm.set_result(True)
                return result
            else:
                return True

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = False

        delegate = Delegate(SMP_DISPLAY_YES_NO_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=False,
            mitm=False,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        dut_sec = 1
        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt auth yesno", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec {dut_sec}"
        )

        # If dut_sec is 1, the security level will be 2 when both devices support SSP
        dut_sec = 2
        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Bonded with {bumble_address}",
                f"Security changed: {bumble_address} level {dut_sec}",
                r"Channel \w+ connected",
            ],
        )
        assert found is True

        dut_sec = 3
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=False,
            mitm=True,
            bonding=True,
            delegate=delegate,
        )
        await send_cmd_to_iut(shell, dut, f"bt security {dut_sec}")
        await asyncio.wait_for(delegate.dut_passkey_confirm, timeout=5.0)

        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Bonded with {bumble_address}",
                f"Security changed: {bumble_address} level {dut_sec}",
            ],
        )
        assert found is True

        shell.exec_command("bt disconnect")
        found, _ = await wait_for_shell_response(
            dut, [r"Channel \w+ disconnected", f"Disconnected: {bumble_address}"]
        )
        assert found is True


async def sm_init_013(hci_port, shell, dut, address, snoop_file) -> None:
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = True

        delegate = PairingDelegate(SMP_DISPLAY_YES_NO_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True,
            mitm=False,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))
        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm + 2, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm + 2))
        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm + 4, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm + 4))

        dut_sec = 2
        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt auth yesno", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )

        await send_cmd_to_iut(shell, dut, f"bt security {dut_sec}", None)
        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Bonded with {bumble_address}",
                f"Security changed: {bumble_address} level {dut_sec}",
            ],
        )
        assert found is True

        await send_cmd_to_iut(
            shell,
            dut,
            f"l2cap_br connect {format(l2cap_server_psm, 'x')}",
            r"Channel \w+ connected",
        )
        await send_cmd_to_iut(
            shell,
            dut,
            f"l2cap_br connect {format(l2cap_server_psm + 2, 'x')}",
            r"Channel \w+ connected",
        )
        await send_cmd_to_iut(
            shell,
            dut,
            f"l2cap_br connect {format(l2cap_server_psm + 4, 'x')}",
            r"Channel \w+ connected",
        )
        await send_cmd_to_iut(shell, dut, "l2cap_br disconnect 0", r"Channel 0 disconnected")
        await send_cmd_to_iut(shell, dut, "l2cap_br disconnect 1", r"Channel 1 disconnected")
        await send_cmd_to_iut(shell, dut, "l2cap_br disconnect 2", r"Channel 2 disconnected")
        await send_cmd_to_iut(shell, dut, "bt disconnect", f"Disconnected: {bumble_address}")


async def sm_init_017(hci_port, shell, dut, address, snoop_file) -> None:
    class Delegate(PairingDelegate):
        async def get_number(self) -> int | None:
            """
            Return an optional number as an answer to a passkey request.
            Returning `None` will result in a negative reply.
            """
            logger.info('Getting number')
            logger.info('Force to reject pairing')
            return None

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = False

        delegate = Delegate(SMP_KEYBOARD_ONLY_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=False,
            mitm=True,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt auth display", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        await send_cmd_to_iut(shell, dut, f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec 3")

        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Pairing failed with {bumble_address} reason: Authentication failure",
                r"Channel \w+ disconnected",
                f"Disconnected: {bumble_address}",
            ],
        )
        assert found is True


async def sm_init_018(hci_port, shell, dut, address, snoop_file) -> None:
    class Delegate(PairingDelegate):
        async def compare_numbers(self, number: int, digits: int) -> bool:
            """Compare two numbers."""
            logger.info('Force timeout during pairing')
            await asyncio.sleep(40)
            return True

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = True

        delegate = Delegate(SMP_DISPLAY_YES_NO_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True,
            mitm=False,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt auth yesno", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        await send_cmd_to_iut(shell, dut, f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec 2")

        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Pairing failed with {bumble_address} reason: Authentication failure",
                r"Channel \w+ disconnected",
                f"Disconnected: {bumble_address}",
            ],
            max_wait_sec=60,
        )
        assert found is True


async def sm_init_021(hci_port, shell, dut, address, snoop_file) -> None:
    class Delegate(PairingDelegate):
        def __init__(
            self,
            io_capability,
        ):
            super().__init__(
                io_capability,
            )
            self.reset()

        def reset(self):
            self.dut_passkey_entry = asyncio.get_running_loop().create_future()

        async def display_number(self, number, digits):
            """Display a number."""
            logger.info(f'Displaying number: {number}')
            found, _ = await wait_for_shell_response(dut, "Enter passkey")
            assert found is True
            logger.info('Enter incorrect passkey to simulate authentication failure')
            shell.exec_command(f"bt auth-passkey {number + 1}")
            self.dut_passkey_entry.set_result(True)

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = False

        delegate = Delegate(SMP_DISPLAY_ONLY_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=False,
            mitm=True,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt auth input", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        await send_cmd_to_iut(shell, dut, f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec 3")
        await asyncio.wait_for(delegate.dut_passkey_entry, timeout=5.0)

        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Pairing failed with {bumble_address} reason: Authentication failure",
                r"Channel \w+ disconnected",
                f"Disconnected: {bumble_address}",
            ],
        )
        assert found is True


async def sm_init_022(hci_port, shell, dut, address, snoop_file) -> None:
    class Delegate(PairingDelegate):
        async def get_string(self, max_length: int) -> str | None:
            """
            Return a string whose utf-8 encoding is up to max_length bytes.
            """
            logger.info('Enter PIN code on Bumble')
            return pin_code

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = False
        device.classic_ssp_enabled = False

        delegate = Delegate(SMP_KEYBOARD_ONLY_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=False,
            mitm=False,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        pin_code = '123456'
        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt auth input", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        lines = shell.exec_command(f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec 2")
        found = check_shell_response(lines, f"Enter PIN code for {bumble_address}")
        assert found is True

        await send_cmd_to_iut(shell, dut, f"br auth-pincode {pin_code}", None)
        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Security changed: {bumble_address} level 2",
                r"Channel \w+ connected",
            ],
        )
        assert found is True

        lines = shell.exec_command("br bonds")
        found = check_shell_response(lines, f"Remote Identity: {bumble_address}")
        assert found is True

        shell.exec_command("bt disconnect")
        found, _ = await wait_for_shell_response(
            dut, [r"Channel \w+ disconnected", f"Disconnected: {bumble_address}"]
        )
        assert found is True


async def sm_init_025(hci_port, shell, dut, address, snoop_file) -> None:
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = True

        delegate = PairingDelegate(SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True,
            mitm=False,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        lines = shell.exec_command(f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec 4")
        assert check_shell_response(lines, r"Channel \w+ disconnected")
        assert check_shell_response(lines, f"Unable to connect to psm {l2cap_server_psm}")
        await send_cmd_to_iut(shell, dut, "bt disconnect", f"Disconnected: {bumble_address}")


async def sm_init_028(hci_port, shell, dut, address, snoop_file) -> None:
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = True

        delegate = PairingDelegate(SMP_DISPLAY_ONLY_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True,
            mitm=False,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        dut_sec = 2
        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt auth display", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        shell.exec_command(f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec {dut_sec}")
        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Bonded with {bumble_address}",
                f"Security changed: {bumble_address} level {dut_sec}",
                r"Channel \w+ connected",
            ],
        )
        assert found is True

        shell.exec_command("bt disconnect")
        found, _ = await wait_for_shell_response(
            dut, [r"Channel \w+ disconnected", f"Disconnected: {bumble_address}"]
        )
        assert found is True

        # Reconnection
        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        shell.exec_command(f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec {dut_sec}")
        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Bonded with {bumble_address}",
                f"Security changed: {bumble_address} level {dut_sec}",
                r"Channel \w+ connected",
            ],
        )
        assert found is True

        shell.exec_command("bt disconnect")
        found, _ = await wait_for_shell_response(
            dut, [r"Channel \w+ disconnected", f"Disconnected: {bumble_address}"]
        )
        assert found is True


async def sm_init_029(hci_port, shell, dut, address, snoop_file) -> None:
    class Delegate(PairingDelegate):
        def __init__(
            self,
            io_capability,
        ):
            super().__init__(
                io_capability,
            )
            self.reset()

        def reset(self):
            self.dut_passkey_confirm = asyncio.get_running_loop().create_future()

        async def compare_numbers(self, number: int, digits: int) -> bool:
            """Compare two numbers."""
            logger.info('Comparing number')
            logger.debug(f'Passkey shown on Bumble: {number}')
            passkey = 0
            regex = r"Confirm passkey for .*?: (?P<passkey>\d{6})"
            found, lines = await wait_for_shell_response(dut, regex)
            if found is True:
                for line in lines:
                    searched = re.search(regex, line)
                    if searched:
                        passkey = int(searched.group("passkey"))
                        break

            logger.debug(f'Passkey shown on DUT: {passkey}')
            if number == passkey:
                logger.info('Passkey matched')
                logger.info('Introduce delays during pairing')
                await asyncio.sleep(10)

                shell.exec_command("bt auth-passkey-confirm")
                self.dut_passkey_confirm.set_result(True)
                return True
            else:
                logger.info('Passkey not matched')
                return False

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = False

        delegate = Delegate(SMP_DISPLAY_YES_NO_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=False,
            mitm=True,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt auth yesno", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        await send_cmd_to_iut(shell, dut, f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec 3")
        await asyncio.wait_for(delegate.dut_passkey_confirm, timeout=20)

        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Bonded with {bumble_address}",
                f"Security changed: {bumble_address} level 3",
                r"Channel \w+ connected",
            ],
        )
        assert found is True

        shell.exec_command("bt disconnect")
        found, _ = await wait_for_shell_response(
            dut, [r"Channel \w+ disconnected", f"Disconnected: {bumble_address}"]
        )
        assert found is True


async def sm_init_030(hci_port, shell, dut, address, snoop_file) -> None:
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = True

        delegate = PairingDelegate(SMP_DISPLAY_ONLY_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True,
            mitm=True,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt auth display", None)

        for _ in range(25):
            await send_cmd_to_iut(shell, dut, "br clear all", None)
            await send_cmd_to_iut(shell, dut, "bt clear all", None)
            await send_cmd_to_iut(
                shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
            )
            await send_cmd_to_iut(
                shell, dut, f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec 2"
            )

            found, _ = await wait_for_shell_response(
                dut,
                [
                    f"Bonded with {bumble_address}",
                    f"Security changed: {bumble_address} level 2",
                    r"Channel \w+ connected",
                ],
            )
            assert found is True

            shell.exec_command("bt disconnect")
            found, _ = await wait_for_shell_response(
                dut, [r"Channel \w+ disconnected", f"Disconnected: {bumble_address}"]
            )
            assert found is True

            timeout = random.uniform(0, 3)
            logger.info(f'Wait a short random period: {timeout}s')
            await asyncio.sleep(timeout)


async def sm_init_032(hci_port, shell, dut, address, snoop_file) -> None:
    class Delegate(PairingDelegate):
        def __init__(
            self,
            io_capability,
        ):
            super().__init__(
                io_capability,
            )
            self.reset()

        def reset(self):
            self.dut_passkey_entry = asyncio.get_running_loop().create_future()

        async def display_number(self, number, digits):
            """Display a number."""
            logger.info(f'Displaying number: {number}')
            found, _ = await wait_for_shell_response(dut, "Enter passkey")
            assert found is True
            shell.exec_command(f"bt auth-passkey {number}")
            self.dut_passkey_entry.set_result(True)

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = False

        delegate = Delegate(SMP_DISPLAY_ONLY_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=False,
            mitm=True,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt auth input", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        await send_cmd_to_iut(shell, dut, f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec 3")
        await asyncio.wait_for(delegate.dut_passkey_entry, timeout=5.0)

        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Bonded with {bumble_address}",
                f"Security changed: {bumble_address} level 3",
                r"Channel \w+ connected",
            ],
        )
        assert found is True

        await sm_reset_board(shell, dut)

        await send_cmd_to_iut(shell, dut, "bt init", "Settings Loaded")
        await send_cmd_to_iut(shell, dut, "br pscan on", None)
        await send_cmd_to_iut(shell, dut, "br iscan on", None)
        await send_cmd_to_iut(shell, dut, "bt auth input", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        await send_cmd_to_iut(shell, dut, f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec 3")
        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Security changed: {bumble_address} level 3",
                r"Channel \w+ connected",
            ],
        )
        assert found is True

        shell.exec_command("bt disconnect")
        found, _ = await wait_for_shell_response(
            dut, [r"Channel \w+ disconnected", f"Disconnected: {bumble_address}"]
        )
        assert found is True


async def sm_init_034(hci_port, shell, dut, address, snoop_file) -> None:
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = True
        device.sdp_service_records = SDP_SERVICE_RECORDS_A2DP

        delegate = PairingDelegate(SMP_DISPLAY_YES_NO_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True,
            mitm=False,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt auth yesno", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )

        pairing_info = []
        for _ in range(3):
            # Discover SDP Record
            shell.exec_command(f"sdp_client ssa_discovery {BT_L2CAP_PROTOCOL_ID.to_hex_str()}")
            if pairing_info == []:
                await send_cmd_to_iut(shell, dut, "bt security 2")
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
            pairing_info.extend(lines)

        assert check_shell_response(pairing_info, f"Bonded with {bumble_address}")
        assert check_shell_response(pairing_info, f"Security changed: {bumble_address} level 2")
        await send_cmd_to_iut(shell, dut, "bt disconnect", f"Disconnected: {bumble_address}")


async def sm_init_035(hci_port, shell, dut, address, snoop_file) -> None:
    class Delegate(PairingDelegate):
        async def get_string(self, max_length: int) -> str | None:
            """
            Return a string whose utf-8 encoding is up to max_length bytes.
            """
            logger.info('Enter PIN code on Bumble')
            return pin_code

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = False
        device.classic_ssp_enabled = False

        delegate = Delegate(SMP_KEYBOARD_ONLY_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=False,
            mitm=True,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        pin_code = '0123456789ABCDEF'
        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt auth input", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        lines = shell.exec_command(f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec 3")
        found = check_shell_response(lines, f"Enter 16 digits wide PIN code for {bumble_address}")
        assert found is True

        await send_cmd_to_iut(shell, dut, f"br auth-pincode {pin_code}", None)
        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Security changed: {bumble_address} level 3",
                r"Channel \w+ connected",
            ],
        )
        assert found is True

        lines = shell.exec_command("br bonds")
        found = check_shell_response(lines, f"Remote Identity: {bumble_address}")
        assert found is True

        shell.exec_command("bt disconnect")
        found, _ = await wait_for_shell_response(
            dut, [r"Channel \w+ disconnected", f"Disconnected: {bumble_address}"]
        )
        assert found is True


async def sm_init_037(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_init_io_cap(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        4,
        SMP_DISPLAY_ONLY_IO_CAPABILITY,
        4,
        SMP_KEYBOARD_ONLY_IO_CAPABILITY,
    )


async def sm_init_039(hci_port, shell, dut, address, snoop_file) -> None:
    class Delegate(PairingDelegate):
        def __init__(
            self,
            io_capability,
        ):
            super().__init__(
                io_capability,
            )
            self.reset()

        def reset(self):
            self.dut_passkey_confirm = asyncio.get_running_loop().create_future()

        async def compare_numbers(self, number: int, digits: int) -> bool:
            """Compare two numbers."""
            logger.info('Comparing number')
            if dut_sec > 2:
                result = await sm_passkey_confirm(shell, dut, number, digits)
                if result is True:
                    self.dut_passkey_confirm.set_result(True)
                return result
            else:
                return True

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = True

        delegate = Delegate(SMP_DISPLAY_YES_NO_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True,
            mitm=True,
            bonding=True,
            delegate=delegate,
        )

        logger.info(f'Bumble registers l2cap server, psm: {format(l2cap_server_psm, "x")}')
        device.create_l2cap_server(spec=ClassicChannelSpec(psm=l2cap_server_psm))

        dut_sec = 4
        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt auth yesno", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        await send_cmd_to_iut(shell, dut, f"bt security {dut_sec}")
        await asyncio.wait_for(delegate.dut_passkey_confirm, timeout=5.0)

        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Bonded with {bumble_address}",
                f"Security changed: {bumble_address} level {dut_sec}",
            ],
        )
        assert found is True

        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True,
            mitm=False,
            bonding=True,
            delegate=delegate,
        )
        await send_cmd_to_iut(shell, dut, "bt security 2")
        lines = shell.exec_command("test_smp security_info")
        assert check_shell_response(lines, f"Security level: {dut_sec}")

        await send_cmd_to_iut(
            shell,
            dut,
            f"l2cap_br connect {format(l2cap_server_psm, 'x')}",
            r"Channel \w+ connected",
        )

        shell.exec_command("bt disconnect")
        found, _ = await wait_for_shell_response(
            dut, [r"Channel \w+ disconnected", f"Disconnected: {bumble_address}"]
        )
        assert found is True


async def sm_rsp_io_cap_with_bonding_flag(
    hci_port,
    shell,
    dut,
    address,
    snoop_file,
    dut_sec,
    dut_io_cap,
    bumble_sec,
    bumble_io_cap,
    bonding=True,
) -> None:
    class Delegate(PairingDelegate):
        async def confirm(self, auto: bool = False) -> bool:
            """Confirming pairing."""
            logger.info('Confirming pairing')
            if dut_method == JUST_WORKS and dut_io_cap != SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY:
                found, _ = await wait_for_shell_response(dut, "Confirm pairing")
                assert found is True
                await send_cmd_to_iut(shell, dut, "bt auth-pairing-confirm", None)
            return True

        async def display_number(self, number, digits):
            """Display a number."""
            logger.info(f'Displaying number: {number}')
            if dut_method == PASSKEY_ENTRY:
                found, _ = await wait_for_shell_response(dut, "Enter passkey")
                assert found is True
                shell.exec_command(f"bt auth-passkey {number}")

        async def compare_numbers(self, number: int, digits: int) -> bool:
            """Compare two numbers."""
            logger.info('Comparing number')
            if dut_method == PASSKEY_CONFIRM:
                return await sm_passkey_confirm(shell, dut, number, digits)
            elif dut_io_cap != SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY:
                found, _ = await wait_for_shell_response(dut, "Confirm pairing")
                assert found is True
                await send_cmd_to_iut(shell, dut, "bt auth-pairing-confirm", None)
                return True
            else:
                return True

        async def get_number(self) -> int | None:
            """
            Return an optional number as an answer to a passkey request.
            Returning `None` will result in a negative reply.
            """
            logger.info('Getting number')
            return await sm_bumble_passkey_entry(dut)

    bt_auth = {
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY: 'none',
        SMP_KEYBOARD_ONLY_IO_CAPABILITY: 'input',
        SMP_DISPLAY_ONLY_IO_CAPABILITY: 'display',
        SMP_DISPLAY_YES_NO_IO_CAPABILITY: 'yesno',
    }
    dut_method = sm_get_pairing_method(dut_sec, dut_io_cap, bumble_sec, bumble_io_cap)

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = bool(dut_sec != 3)

        delegate = Delegate(SMP_DISPLAY_YES_NO_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=bool(dut_sec != 3),
            mitm=bool(dut_sec > 2),
            bonding=bonding,
            delegate=delegate,
        )

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]
        iut_address = address.split(" ")[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, f"bt auth {bt_auth[dut_io_cap]}", None)
        await send_cmd_to_iut(shell, dut, f"bt bondable {'on' if bonding else 'off'}", None)
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br register {format(l2cap_server_psm, 'x')}", None
        )
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br security {format(l2cap_server_psm, 'x')} {dut_sec}", None
        )
        connection = await bumble_acl_connect(shell, dut, device, iut_address)

        logger.info('Authenticating...')
        await device.authenticate(connection)

        logger.info('Enabling encryption...')
        await device.encrypt(connection)

        # For JUST_WORKS, the security level will be 2 when both devices support SSP.
        sec_check = 2 if dut_method == JUST_WORKS else dut_sec

        expected = [f"Security changed: {bumble_address} level {sec_check}"]
        if bonding is True:
            expected.append(f"Bonded with {bumble_address}")
        found, _ = await wait_for_shell_response(dut, expected)
        assert found is True

        logger.info('Creating L2CAP channel...')
        channel, chan_conn_res = await bumble_l2cap_br_connect(connection)

        if dut_io_cap == SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY and dut_sec > 2:
            # For no input/output, the security level will be 2 and L2CAP connection will fail.
            assert (chan_conn_res == "CONNECTION_REFUSED_SECURITY_BLOCK") is True
            found, _ = await wait_for_shell_response(
                dut,
                [
                    r"Channel \w+ disconnected",
                    f"Disconnected: {bumble_address}",
                ],
            )
            assert found is True
        elif dut_method == JUST_WORKS and dut_sec > 2:
            # For JUST_WORKS, the security level will be 2 and L2CAP connection will fail.
            assert (chan_conn_res == "abort: disconnection event occurred.") is True
            found, _ = await wait_for_shell_response(
                dut,
                [
                    r"Channel \w+ disconnected",
                    f"Disconnected: {bumble_address}",
                ],
            )
            assert found is True
        else:
            found, _ = await wait_for_shell_response(dut, r"Channel \w+ connected")
            assert found is True

            await channel.disconnect()
            found, _ = await wait_for_shell_response(dut, r"Channel \w+ disconnected")
            assert found is True

            await device.disconnect(connection, HCI_REMOTE_USER_TERMINATED_CONNECTION_ERROR)
            found, _ = await wait_for_shell_response(dut, f"Disconnected: {bumble_address}")
            assert found is True


async def sm_rsp_001(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_rsp_io_cap_with_bonding_flag(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        1,
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY,
        1,
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY,
        False,
    )


async def sm_rsp_002(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_rsp_io_cap_with_bonding_flag(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        2,
        SMP_DISPLAY_ONLY_IO_CAPABILITY,
        2,
        SMP_KEYBOARD_ONLY_IO_CAPABILITY,
    )


async def sm_rsp_003(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_rsp_io_cap_with_bonding_flag(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        3,
        SMP_KEYBOARD_ONLY_IO_CAPABILITY,
        3,
        SMP_DISPLAY_ONLY_IO_CAPABILITY,
    )


async def sm_rsp_006(hci_port, shell, dut, address, snoop_file) -> None:
    class Delegate(PairingDelegate):
        async def compare_numbers(self, number: int, digits: int) -> bool:
            """Compare two numbers."""
            logger.info('Comparing number')
            if dut_sec > 2:
                return await sm_passkey_confirm(shell, dut, number, digits)
            else:
                return True

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = True

        delegate = Delegate(SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True,
            mitm=False,
            bonding=True,
            delegate=delegate,
        )

        dut_sec = 4
        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]
        iut_address = address.split(" ")[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br register {format(l2cap_server_psm, 'x')}", None
        )
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br security {format(l2cap_server_psm, 'x')} {dut_sec}", None
        )
        connection = await bumble_acl_connect(shell, dut, device, iut_address)

        logger.info('Authenticating...')
        await device.authenticate(connection)
        logger.info('Enabling encryption...')
        await device.encrypt(connection)

        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Bonded with {bumble_address}",
                f"Security changed: {bumble_address} level 2",
            ],
        )
        assert found is True

        logger.info('Creating L2CAP channel...')
        channel, chan_conn_res = await bumble_l2cap_br_connect(connection)

        # For no input/output, the security level will be 2 and L2CAP connection will fail.
        assert (chan_conn_res == "CONNECTION_REFUSED_SECURITY_BLOCK") is True
        found, _ = await wait_for_shell_response(
            dut,
            [
                r"Channel \w+ disconnected",
                f"Disconnected: {bumble_address}",
            ],
        )
        assert found is True

        delegate = Delegate(SMP_DISPLAY_YES_NO_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True,
            mitm=True,
            bonding=True,
            delegate=delegate,
        )
        await device.keystore.delete_all()
        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth yesno", None)

        connection = await bumble_acl_connect(shell, dut, device, iut_address)
        logger.info('Authenticating...')
        await device.authenticate(connection)
        logger.info('Enabling encryption...')
        await device.encrypt(connection)
        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Bonded with {bumble_address}",
                f"Security changed: {bumble_address} level {dut_sec}",
            ],
        )
        assert found is True

        logger.info('Creating L2CAP channel...')
        channel, _ = await bumble_l2cap_br_connect(connection)

        found, _ = await wait_for_shell_response(dut, r"Channel \w+ connected")
        assert found is True

        await channel.disconnect()
        found, _ = await wait_for_shell_response(dut, r"Channel \w+ disconnected")
        assert found is True

        await device.disconnect(connection, HCI_REMOTE_USER_TERMINATED_CONNECTION_ERROR)
        found, _ = await wait_for_shell_response(dut, f"Disconnected: {bumble_address}")
        assert found is True


async def sm_rsp_sec_level(hci_port, shell, dut, address, snoop_file, dut_sec, dut_io_cap) -> None:
    class Delegate(PairingDelegate):
        async def confirm(self, auto: bool = False) -> bool:
            """Confirming pairing."""
            logger.info('Confirming pairing')
            found, _ = await wait_for_shell_response(dut, "Confirm pairing")
            assert found is True
            await send_cmd_to_iut(shell, dut, "bt auth-pairing-confirm", None)
            return True

        async def compare_numbers(self, number: int, digits: int) -> bool:
            """Compare two numbers."""
            logger.info('Comparing number')
            found, _ = await wait_for_shell_response(dut, "Confirm pairing")
            assert found is True
            await send_cmd_to_iut(shell, dut, "bt auth-pairing-confirm", None)
            return True

    sdu_sent_cnt = 2
    sdu_sent_len = 10
    sdu_rcvd_cnt = 0
    sdu_rcvd_evt = asyncio.get_running_loop().create_future()

    def on_channel_sdu(sdu):
        nonlocal sdu_sent_cnt
        nonlocal sdu_sent_len
        nonlocal sdu_rcvd_evt
        nonlocal sdu_rcvd_cnt

        print(f'Recv L2CAP SDU: {len(sdu)} bytes, {sdu}')

        if sdu_rcvd_cnt >= sdu_sent_cnt:
            return

        byte = sdu_sent_cnt - sdu_rcvd_cnt - 1
        if sdu == bytes([byte for _ in range(sdu_sent_len)]):
            sdu_rcvd_cnt += 1
            if sdu_rcvd_cnt == sdu_sent_cnt:
                sdu_rcvd_evt.set_result(True)

    bt_auth = {
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY: 'none',
        SMP_KEYBOARD_ONLY_IO_CAPABILITY: 'input',
        SMP_DISPLAY_ONLY_IO_CAPABILITY: 'display',
        SMP_DISPLAY_YES_NO_IO_CAPABILITY: 'yesno',
    }

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = SmpDevice.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = True

        delegate = Delegate(SMP_DISPLAY_YES_NO_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True,
            mitm=False,
            bonding=True,
            delegate=delegate,
        )

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]
        iut_address = address.split(" ")[0]

        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, f"bt auth {bt_auth[dut_io_cap]}", None)
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br register {format(l2cap_server_psm, 'x')}", None
        )
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br security {format(l2cap_server_psm, 'x')} {dut_sec}", None
        )
        connection = await bumble_acl_connect(shell, dut, device, iut_address)

        logger.info('Authenticating...')
        await device.authenticate(connection)
        logger.info('Enabling encryption...')
        await device.encrypt(connection)
        dut_sec = 2 if dut_sec == 1 else dut_sec
        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Bonded with {bumble_address}",
                f"Security changed: {bumble_address} level {dut_sec}",
            ],
        )
        assert found is True

        logger.info('Creating L2CAP channel...')
        channel = await device.create_l2cap_channel(
            connection=connection, spec=ClassicChannelSpec(psm=l2cap_server_psm)
        )
        found, _ = await wait_for_shell_response(dut, r"Channel \w+ connected")
        assert found is True

        logger.info('Sending L2CAP Data...')
        channel.sink = on_channel_sdu
        await send_cmd_to_iut(shell, dut, f"l2cap_br send 0 {sdu_sent_cnt} {sdu_sent_len}", None)
        await asyncio.wait_for(sdu_rcvd_evt, timeout=5.0)

        await channel.disconnect()
        found, _ = await wait_for_shell_response(dut, r"Channel \w+ disconnected")
        assert found is True

        await device.disconnect(connection, HCI_REMOTE_USER_TERMINATED_CONNECTION_ERROR)
        found, _ = await wait_for_shell_response(dut, f"Disconnected: {bumble_address}")
        assert found is True


async def sm_rsp_008(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_rsp_sec_level(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        1,
        SMP_KEYBOARD_ONLY_IO_CAPABILITY,
    )


async def sm_rsp_009(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_rsp_sec_level(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        2,
        SMP_DISPLAY_YES_NO_IO_CAPABILITY,
    )


class TestSmpServer:
    def test_sm_init_001(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that an initiator can successfully establish a secure connection with
        a peripheral device when using "No Input No Output" IO capability."""
        logger.info(f'test_sm_init_001 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_001(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_002(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that an initiator can successfully establish a secure connection with
        a peripheral device when using "Display Only" IO capability."""
        logger.info(f'test_sm_init_002 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_002(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_003(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that an initiator can successfully establish a secure connection with
        a peripheral device when using "Keyboard Only" IO capability."""
        logger.info(f'test_sm_init_003 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_003(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_005(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that the initiator handles L2CAP connection failures correctly after
        successful pairing."""
        logger.info(f'test_sm_init_005 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_005(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_006(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that an initiator can successfully establish a secure connection when
        configured as an L2CAP server."""
        logger.info(f'test_sm_init_006 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_006(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_008(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that the initiator handles ACL disconnection correctly during
        the pairing process."""
        logger.info(f'test_sm_init_008 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_008(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_011(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that an initiator can successfully re-establish a secure connection
        with a previously paired device."""
        logger.info(f'test_sm_init_011 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_011(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_012(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that an initiator can escalate security level on an already
        encrypted connection."""
        logger.info(f'test_sm_init_012 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_012(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_013(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that an initiator can establish multiple secure L2CAP channels
        after a single pairing."""
        logger.info(f'test_sm_init_013 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_013(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_017(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that an initiator handles pairing rejection correctly."""
        logger.info(f'test_sm_init_017 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_017(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_018(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that an initiator handles pairing timeout correctly."""
        logger.info(f'test_sm_init_018 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_018(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_021(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that an initiator handles authentication failures correctly
        during the pairing process."""
        logger.info(f'test_sm_init_021 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_021(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_022(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that an initiator can fallback to legacy pairing when "Secure connection"
        is selected (not "Secure connection Only") and the peripheral doesn't support
        Secure Connections and SSP."""
        logger.info(f'test_sm_init_022 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_022(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_025(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that the initiator rejects weak pairing methods when high
        security level is required."""
        logger.info(f'test_sm_init_025 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_025(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_028(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that the initiator handles reconnection correctly after
        previously stored keys are deleted."""
        logger.info(f'test_sm_init_028 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_028(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_029(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that the initiator handles delayed responses during the pairing
        process correctly."""
        logger.info(f'test_sm_init_029 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_029(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_030(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify the robustness of the SM initiator by performing multiple connection,
        pairing, and disconnection cycles."""
        logger.info(f'test_sm_init_030 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_030(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_032(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that the initiator can recover correctly after power loss during
        an established connection."""
        logger.info(f'test_sm_init_032 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_032(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_034(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that the initiator can successfully initiate and complete pairing
        while actively transferring data."""
        logger.info(f'test_sm_init_034 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_034(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_035(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that the initiator correctly handles pairing with lengthy
        authentication credentials."""
        logger.info(f'test_sm_init_035 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_035(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_037(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that the initiator correctly handles peripherals requesting
        maximum encryption key size."""
        logger.info(f'test_sm_init_037 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_037(hci, shell, dut, iut_address, snoop_file))

    def test_sm_init_039(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that the initiator correctly handles attempted security level
        downgrades during the pairing process."""
        logger.info(f'test_sm_init_039 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_init_039(hci, shell, dut, iut_address, snoop_file))

    def test_sm_rsp_001(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify basic SM responder functionality with Non-bondable pairing and
        No Input No Output capability."""
        logger.info(f'test_sm_rsp_001 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_rsp_001(hci, shell, dut, iut_address, snoop_file))

    def test_sm_rsp_002(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify SM responder functionality with Display Only capability and general bonding."""
        logger.info(f'test_sm_rsp_002 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_rsp_002(hci, shell, dut, iut_address, snoop_file))

    def test_sm_rsp_003(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify SM responder functionality with Keyboard Only capability and dedicated bonding."""
        logger.info(f'test_sm_rsp_003 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_rsp_003(hci, shell, dut, iut_address, snoop_file))

    def test_sm_rsp_006(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify SM responder enforces the configured security level."""
        logger.info(f'test_sm_rsp_006 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_rsp_006(hci, shell, dut, iut_address, snoop_file))

    def test_sm_rsp_008(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify SM responder correctly handles L2CAP connection with Security Level 1."""
        logger.info(f'test_sm_rsp_008 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_rsp_008(hci, shell, dut, iut_address, snoop_file))

    def test_sm_rsp_009(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify SM responder correctly handles L2CAP server with Security Level 2."""
        logger.info(f'test_sm_rsp_009 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_rsp_009(hci, shell, dut, iut_address, snoop_file))

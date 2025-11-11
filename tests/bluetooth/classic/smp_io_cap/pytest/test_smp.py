# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0


import asyncio
import logging
import re
import sys

from bumble.core import (
    BT_BR_EDR_TRANSPORT,
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
from bumble.l2cap import ClassicChannelSpec
from bumble.pairing import PairingConfig, PairingDelegate
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
    shell.exec_command(cmd)
    if parse is not None:
        found, lines = await wait_for_shell_response(dut, parse)
    else:
        found = True
        lines = []
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


async def sm_io_cap_init(hci_port, shell, dut, address, snoop_file, dut_sec, dut_io_cap) -> None:
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
            self.passkey_entry = asyncio.get_running_loop().create_future()
            self.passkey_confirm = asyncio.get_running_loop().create_future()

        async def display_number(self, number, digits):
            """Display a number."""
            logger.info(f'Displaying number: {number}')
            if dut_method == PASSKEY_ENTRY:
                found, _ = await wait_for_shell_response(dut, "Enter passkey")
                assert found is True
                shell.exec_command(f"bt auth-passkey {number}")
                self.passkey_entry.set_result(True)

        async def compare_numbers(self, number: int, digits: int) -> bool:
            """Compare two numbers."""
            logger.info('Comparing number')
            if dut_method == PASSKEY_CONFIRM:
                result = await sm_passkey_confirm(shell, dut, number, digits)
                if result is True:
                    self.passkey_confirm.set_result(True)
                return result
            else:
                return True

    bt_auth = {
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY: 'none',
        SMP_KEYBOARD_ONLY_IO_CAPABILITY: 'input',
        SMP_DISPLAY_ONLY_IO_CAPABILITY: 'display',
        SMP_DISPLAY_YES_NO_IO_CAPABILITY: 'yesno',
    }

    bumble_sec = dut_sec
    bumble_io_cap = SMP_DISPLAY_YES_NO_IO_CAPABILITY
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
            await asyncio.wait_for(delegate.passkey_entry, timeout=5.0)
        elif dut_method == PASSKEY_CONFIRM:
            await asyncio.wait_for(delegate.passkey_confirm, timeout=5.0)

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
            if (
                dut_sec == 1
            ):  # If dut_sec is 1, the security level will be 2 when both devices support SSP
                dut_sec += 1

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


async def sm_io_cap_init_001(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        1,
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY,
    )


async def sm_io_cap_init_002(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        2,
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY,
    )


async def sm_io_cap_init_003(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        3,
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY,
    )


async def sm_io_cap_init_004(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        4,
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY,
    )


async def sm_io_cap_init_005(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        1,
        SMP_DISPLAY_ONLY_IO_CAPABILITY,
    )


async def sm_io_cap_init_006(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        2,
        SMP_DISPLAY_ONLY_IO_CAPABILITY,
    )


async def sm_io_cap_init_007(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        3,
        SMP_DISPLAY_ONLY_IO_CAPABILITY,
    )


async def sm_io_cap_init_008(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        4,
        SMP_DISPLAY_ONLY_IO_CAPABILITY,
    )


async def sm_io_cap_init_009(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        1,
        SMP_KEYBOARD_ONLY_IO_CAPABILITY,
    )


async def sm_io_cap_init_010(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        2,
        SMP_KEYBOARD_ONLY_IO_CAPABILITY,
    )


async def sm_io_cap_init_011(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        3,
        SMP_KEYBOARD_ONLY_IO_CAPABILITY,
    )


async def sm_io_cap_init_012(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        4,
        SMP_KEYBOARD_ONLY_IO_CAPABILITY,
    )


async def sm_io_cap_init_013(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        1,
        SMP_DISPLAY_YES_NO_IO_CAPABILITY,
    )


async def sm_io_cap_init_014(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        2,
        SMP_DISPLAY_YES_NO_IO_CAPABILITY,
    )


async def sm_io_cap_init_015(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        3,
        SMP_DISPLAY_YES_NO_IO_CAPABILITY,
    )


async def sm_io_cap_init_016(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        4,
        SMP_DISPLAY_YES_NO_IO_CAPABILITY,
    )


async def sm_io_cap_rsp(hci_port, shell, dut, address, snoop_file, dut_sec, dut_io_cap) -> None:
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
            elif dut_io_cap != SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY and not encrption_on:
                found, _ = await wait_for_shell_response(dut, "Confirm pairing")
                assert found is True
                await send_cmd_to_iut(shell, dut, "bt auth-pairing-confirm", None)
                return True
            else:
                return True

    encrption_on = False

    bt_auth = {
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY: 'none',
        SMP_KEYBOARD_ONLY_IO_CAPABILITY: 'input',
        SMP_DISPLAY_ONLY_IO_CAPABILITY: 'display',
        SMP_DISPLAY_YES_NO_IO_CAPABILITY: 'yesno',
    }

    bumble_sec = dut_sec
    bumble_io_cap = SMP_DISPLAY_YES_NO_IO_CAPABILITY
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

        delegate = Delegate(bumble_io_cap)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=bool(dut_sec != 3),
            mitm=bool(dut_sec > 2),
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
        encrption_on = True

        # For JUST_WORKS, the security level will be 2 when both devices support SSP.
        sec_check = 2 if dut_method == JUST_WORKS else dut_sec

        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Bonded with {bumble_address}",
                f"Security changed: {bumble_address} level {sec_check}",
            ],
        )
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
            # For display only with display yes/no, the security level will be 2
            # and L2CAP connection will fail.
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


async def sm_io_cap_rsp_001(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        1,
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY,
    )


async def sm_io_cap_rsp_002(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        2,
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY,
    )


async def sm_io_cap_rsp_003(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        3,
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY,
    )


async def sm_io_cap_rsp_004(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        4,
        SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY,
    )


async def sm_io_cap_rsp_005(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        1,
        SMP_DISPLAY_ONLY_IO_CAPABILITY,
    )


async def sm_io_cap_rsp_006(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        2,
        SMP_DISPLAY_ONLY_IO_CAPABILITY,
    )


async def sm_io_cap_rsp_007(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        3,
        SMP_DISPLAY_ONLY_IO_CAPABILITY,
    )


async def sm_io_cap_rsp_008(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        4,
        SMP_DISPLAY_ONLY_IO_CAPABILITY,
    )


async def sm_io_cap_rsp_009(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        1,
        SMP_KEYBOARD_ONLY_IO_CAPABILITY,
    )


async def sm_io_cap_rsp_010(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        2,
        SMP_KEYBOARD_ONLY_IO_CAPABILITY,
    )


async def sm_io_cap_rsp_011(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        3,
        SMP_KEYBOARD_ONLY_IO_CAPABILITY,
    )


async def sm_io_cap_rsp_012(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        4,
        SMP_KEYBOARD_ONLY_IO_CAPABILITY,
    )


async def sm_io_cap_rsp_013(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        1,
        SMP_DISPLAY_YES_NO_IO_CAPABILITY,
    )


async def sm_io_cap_rsp_014(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        2,
        SMP_DISPLAY_YES_NO_IO_CAPABILITY,
    )


async def sm_io_cap_rsp_015(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        3,
        SMP_DISPLAY_YES_NO_IO_CAPABILITY,
    )


async def sm_io_cap_rsp_016(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_io_cap_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        4,
        SMP_DISPLAY_YES_NO_IO_CAPABILITY,
    )


class TestSmpServer:
    def test_sm_io_cap_init_001(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "No Input No Output" IO capability can
        connect to a peer with "Display YesNo" capability using Security Level 1."""
        logger.info(f'test_sm_io_cap_init_001 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_init_001(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_init_002(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "No Input No Output" IO capability can
        connect to a peer with "Display YesNo" capability using Security Level 2."""
        logger.info(f'test_sm_io_cap_init_002 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_init_002(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_init_003(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "No Input No Output" IO capability cannot
        connect to a peer with "Display YesNo" capability using Security Level 3."""
        logger.info(f'test_sm_io_cap_init_003 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_init_003(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_init_004(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "No Input No Output" IO capability cannot
        connect to a peer with "Display YesNo" capability using Security Level 4."""
        logger.info(f'test_sm_io_cap_init_004 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_init_004(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_init_005(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Display Only" IO capability can
        connect to a peer with "Display YesNo" capability using Security Level 1."""
        logger.info(f'test_sm_io_cap_init_005 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_init_005(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_init_006(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Display Only" IO capability can
        connect to a peer with "Display YesNo" capability using Security Level 2."""
        logger.info(f'test_sm_io_cap_init_006 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_init_006(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_init_007(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Display Only" IO capability cannot
        connect to a peer with "Display YesNo" capability using Security Level 3."""
        logger.info(f'test_sm_io_cap_init_007 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_init_007(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_init_008(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Display Only" IO capability cannot
        connect to a peer with "Display YesNo" capability using Security Level 4."""
        logger.info(f'test_sm_io_cap_init_008 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_init_008(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_init_009(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Keyboard Only" IO capability can
        connect to a peer with "Display YesNo" capability using Security Level 1."""
        logger.info(f'test_sm_io_cap_init_009 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_init_009(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_init_010(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Keyboard Only" IO capability can
        connect to a peer with "Display YesNo" capability using Security Level 2."""
        logger.info(f'test_sm_io_cap_init_010 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_init_010(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_init_011(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Keyboard Only" IO capability can
        connect to a peer with "Display YesNo" capability using Security Level 3."""
        logger.info(f'test_sm_io_cap_init_011 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_init_011(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_init_012(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Keyboard Only" IO capability can
        connect to a peer with "Display YesNo" capability using Security Level 4."""
        logger.info(f'test_sm_io_cap_init_012 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_init_012(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_init_013(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Display YesNo" IO capability can
        connect to a peer with "Display YesNo" capability using Security Level 1."""
        logger.info(f'test_sm_io_cap_init_013 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_init_013(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_init_014(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Display YesNo" IO capability can
        connect to a peer with "Display YesNo" capability using Security Level 2."""
        logger.info(f'test_sm_io_cap_init_014 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_init_014(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_init_015(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Display YesNo" IO capability can
        connect to a peer with "Display YesNo" capability using Security Level 3."""
        logger.info(f'test_sm_io_cap_init_015 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_init_015(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_init_016(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Display YesNo" IO capability can
        connect to a peer with "Display YesNo" capability using Security Level 4."""
        logger.info(f'test_sm_io_cap_init_016 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_init_016(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_rsp_001(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "No input No output" IO capability can establish a
        connection at Security Level 1 with a peer device having "Display YesOrNo" capability."""
        logger.info(f'test_sm_io_cap_rsp_001 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_rsp_001(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_rsp_002(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "No input No output" IO capability can establish a
        connection at Security Level 2 with a peer device having "Display YesOrNo" capability."""
        logger.info(f'test_sm_io_cap_rsp_002 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_rsp_002(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_rsp_003(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "No input No output" IO capability cannot establish a
        connection at Security Level 3 with a peer device having "Display YesOrNo" capability."""
        logger.info(f'test_sm_io_cap_rsp_003 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_rsp_003(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_rsp_004(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "No input No output" IO capability cannot establish a
        connection at Security Level 4 with a peer device having "Display YesOrNo" capability."""
        logger.info(f'test_sm_io_cap_rsp_004 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_rsp_004(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_rsp_005(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Display Only" IO capability can establish a
        connection at Security Level 1 with a peer device having "Display YesOrNo" capability."""
        logger.info(f'test_sm_io_cap_rsp_005 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_rsp_005(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_rsp_006(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Display Only" IO capability can establish a
        connection at Security Level 2 with a peer device having "Display YesOrNo" capability."""
        logger.info(f'test_sm_io_cap_rsp_006 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_rsp_006(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_rsp_007(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Display Only" IO capability cannot establish a
        connection at Security Level 3 with a peer device having "Display YesOrNo" capability."""
        logger.info(f'test_sm_io_cap_rsp_007 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_rsp_007(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_rsp_008(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Display Only" IO capability cannot establish a
        connection at Security Level 4 with a peer device having "Display YesOrNo" capability."""
        logger.info(f'test_sm_io_cap_rsp_008 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_rsp_008(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_rsp_009(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Keyboard Only" IO capability can establish a
        connection at Security Level 1 with a peer device having "Display YesOrNo" capability."""
        logger.info(f'test_sm_io_cap_rsp_009 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_rsp_009(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_rsp_010(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Keyboard Only" IO capability can establish a
        connection at Security Level 2 with a peer device having "Display YesOrNo" capability."""
        logger.info(f'test_sm_io_cap_rsp_010 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_rsp_010(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_rsp_011(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Keyboard Only" IO capability can establish a
        connection at Security Level 3 with a peer device having "Display YesOrNo" capability."""
        logger.info(f'test_sm_io_cap_rsp_011 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_rsp_011(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_rsp_012(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Keyboard Only" IO capability can establish a
        connection at Security Level 4 with a peer device having "Display YesOrNo" capability."""
        logger.info(f'test_sm_io_cap_rsp_012 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_rsp_012(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_rsp_013(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Display YesOrNo" IO capability can establish a
        connection at Security Level 1 with a peer device having "Display YesOrNo" capability."""
        logger.info(f'test_sm_io_cap_rsp_013 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_rsp_013(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_rsp_014(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Display YesOrNo" IO capability can establish a
        connection at Security Level 2 with a peer device having "Display YesOrNo" capability."""
        logger.info(f'test_sm_io_cap_rsp_014 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_rsp_014(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_rsp_015(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Display YesOrNo" IO capability can establish a
        connection at Security Level 3 with a peer device having "Display YesOrNo" capability."""
        logger.info(f'test_sm_io_cap_rsp_015 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_rsp_015(hci, shell, dut, iut_address, snoop_file))

    def test_sm_io_cap_rsp_016(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that a device with "Display YesOrNo" IO capability can establish a
        connection at Security Level 4 with a peer device having "Display YesOrNo" capability."""
        logger.info(f'test_sm_io_cap_rsp_016 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_io_cap_rsp_016(hci, shell, dut, iut_address, snoop_file))

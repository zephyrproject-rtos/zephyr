# Copyright 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import asyncio
import logging
import re

from bumble import hci
from bumble.core import BT_BR_EDR_TRANSPORT, ProtocolError, UnreachableError
from bumble.device import (
    DEVICE_DEFAULT_INQUIRY_LENGTH,
    Device,
    host_event_handler,
    with_connection_from_address,
)
from bumble.hci import (
    HCI_User_Confirmation_Request_Negative_Reply_Command,
    HCI_User_Confirmation_Request_Reply_Command,
)
from bumble.l2cap import ClassicChannelSpec
from bumble.smp import (
    SMP_DISPLAY_ONLY_IO_CAPABILITY,
    SMP_DISPLAY_YES_NO_IO_CAPABILITY,
    SMP_KEYBOARD_ONLY_IO_CAPABILITY,
    SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY,
)
from bumble.utils import AsyncRunner

logger = logging.getLogger(__name__)


async def device_power_on(device, retries: int = 10, delay: float = 0.5) -> None:
    for attempt in range(1, retries + 1):
        try:
            await device.power_on()
            return
        except Exception:
            logger.exception('device.power_on() failed (attempt %d/%d)', attempt, retries)
            await asyncio.sleep(delay)

    raise RuntimeError(f'device.power_on() failed after {retries} attempts')


l2cap_server_psm = 0x1001


def search_messages(result, messages, read_lines):
    for message in messages:
        for line in read_lines:
            if re.search(message, line):
                result[message] = True
                if False not in result.values():
                    return True
                break

    return False


async def set_limited_discoverable(device, discoverable=True):
    # Read current class of device
    response = await device.send_command(
        hci.HCI_Command(
            op_code=0x0C23,  # Read Class of Device
            parameters=b'',
        )
    )
    current_cod = response.return_parameters.class_of_device

    if discoverable:
        # set Limited Discoverable Mode (bit 13)
        new_cod = (current_cod | 0x2000).to_bytes(3, byteorder='little')
        # Limited Inquiry Access Code(LIAC) = 0x9E8B00
        iac = hci.HCI_LIMITED_DEDICATED_INQUIRY_LAP.to_bytes(3, byteorder='little')
    else:
        mask = ~0x2000
        new_cod = (current_cod & mask).to_bytes(3, byteorder='little')
        # General Inquiry Access Code(GIAC) = 0x9E8B33
        iac = hci.HCI_GENERAL_INQUIRY_LAP.to_bytes(3, byteorder='little')

    await device.send_command(
        hci.HCI_Command(
            op_code=0x0C24,  # Write Class of Device
            parameters=new_cod,
        )
    )

    await device.send_command(
        hci.HCI_Command(
            op_code=0x0C3A,  # Write Current IAC LAP
            parameters=bytes([0x01]) + iac,  # num_current_iac=1, iac_lap
        )
    )

    device.discoverable = discoverable


async def start_limited_discovery(device):
    await device.send_command(
        hci.HCI_Write_Inquiry_Mode_Command(inquiry_mode=hci.HCI_EXTENDED_INQUIRY_MODE),
        check_result=True,
    )

    response = await device.send_command(
        hci.HCI_Inquiry_Command(
            lap=hci.HCI_LIMITED_DEDICATED_INQUIRY_LAP,
            inquiry_length=DEVICE_DEFAULT_INQUIRY_LENGTH,
            num_responses=0,  # Unlimited number of responses.
        )
    )
    if response.status != hci.HCI_Command_Status_Event.PENDING:
        device.discovering = False
        raise hci.HCI_StatusError(response)

    device.auto_restart_inquiry = False
    device.discovering = True


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
            except Exception:
                logger.exception('exception while confirming')

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

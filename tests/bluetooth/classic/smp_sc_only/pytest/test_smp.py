# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0


import asyncio
import logging
import re
import sys

from bumble.core import (
    BT_BR_EDR_TRANSPORT,
)
from bumble.device import (
    Device,
)
from bumble.hci import (
    HCI_REMOTE_USER_TERMINATED_CONNECTION_ERROR,
    Address,
    HCI_Error,
    HCI_Write_Page_Timeout_Command,
)
from bumble.l2cap import ClassicChannelSpec
from bumble.pairing import PairingConfig, PairingDelegate
from bumble.smp import (
    SMP_DISPLAY_YES_NO_IO_CAPABILITY,
    SMP_KEYBOARD_ONLY_IO_CAPABILITY,
)
from bumble.snoop import BtSnooper
from bumble.transport import open_transport_or_link
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)

l2cap_server_psm = 0x1001


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


async def sm_sc_only_init(hci_port, shell, dut, address, snoop_file, dut_sec) -> None:
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
            self.passkey_confirm = asyncio.get_running_loop().create_future()

        async def compare_numbers(self, number: int, digits: int) -> bool:
            """Compare two numbers."""
            logger.info('Comparing number')
            result = await sm_passkey_confirm(shell, dut, number, digits)
            if result is True:
                self.passkey_confirm.set_result(True)
            return result

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
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
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec {dut_sec}", None
        )

        await asyncio.wait_for(delegate.passkey_confirm, timeout=5.0)

        if dut_sec == 3:
            found, _ = await wait_for_shell_response(
                dut,
                [
                    f"Pairing failed with {bumble_address} reason: Authentication failure",
                    r"Channel \w+ disconnected",
                    f"Disconnected: {bumble_address}",
                ],
            )
            assert found is True
        else:
            # In SC only mode, the security level will be overwritten as 4 on the DUT side when
            # creating L2CAP connection with the security level 1-3, so the final security level
            # is 4.
            found, _ = await wait_for_shell_response(
                dut,
                [
                    f"Bonded with {bumble_address}",
                    f"Security changed: {bumble_address} level 4",
                    r"Channel \w+ connected",
                ],
            )
            assert found is True

            shell.exec_command("bt disconnect")
            found, _ = await wait_for_shell_response(
                dut, [r"Channel \w+ disconnected", f"Disconnected: {bumble_address}"]
            )
            assert found is True


async def sm_sc_init_005(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_sc_only_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        4,
    )


async def sm_sc_init_006(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_sc_only_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        1,
    )


async def sm_sc_init_007(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_sc_only_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        2,
    )


async def sm_sc_init_008(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_sc_only_init(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        3,
    )


async def sm_sc_init_009(hci_port, shell, dut, address, snoop_file) -> None:
    class Delegate(PairingDelegate):
        async def get_string(self, max_length: int) -> str | None:
            """
            Return a string whose utf-8 encoding is up to max_length bytes.
            """
            logger.info('Enter PIN code on Bumble')
            return pin_code

    dut_sec = 3
    pin_code = '0123456789ABCDEF'
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = bool(dut_sec != 3)
        device.classic_ssp_enabled = False

        delegate = Delegate(SMP_KEYBOARD_ONLY_IO_CAPABILITY)
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
        await send_cmd_to_iut(shell, dut, "bt auth input", None)
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )
        lines = shell.exec_command(
            f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec {dut_sec}"
        )
        assert check_shell_response(lines, f"Enter.*?PIN code for {bumble_address}")

        await send_cmd_to_iut(shell, dut, f"br auth-pincode {pin_code}", None)
        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Pairing failed with {bumble_address} reason: Authentication failure",
                r"Channel \w+ disconnected",
                f"Disconnected: {bumble_address}",
            ],
        )
        assert found is True


async def sm_sc_only_rsp(hci_port, shell, dut, address, snoop_file, dut_sec) -> None:
    class Delegate(PairingDelegate):
        async def compare_numbers(self, number: int, digits: int) -> bool:
            """Compare two numbers."""
            logger.info('Comparing number')
            if dut_sec > 2:
                return await sm_passkey_confirm(shell, dut, number, digits)
            else:
                found, _ = await wait_for_shell_response(dut, "Confirm pairing")
                assert found is True
                await send_cmd_to_iut(shell, dut, "bt auth-pairing-confirm", None)
                return True

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
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
        await send_cmd_to_iut(shell, dut, "bt auth yesno", None)
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br register {format(l2cap_server_psm, 'x')}", None
        )
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br security {format(l2cap_server_psm, 'x')} {dut_sec}", None
        )
        connection = await bumble_acl_connect(shell, dut, device, iut_address)

        logger.info('Authenticating...')
        await device.authenticate(connection)

        try:
            logger.info('Enabling encryption...')
            await device.encrypt(connection)
        except BaseException as e:
            if isinstance(e, HCI_Error):
                logger.info(f'Fail to encrypt, reason: {e.error_name}')
            else:
                raise e

        if dut_sec != 4:
            found, _ = await wait_for_shell_response(
                dut,
                [
                    f"Pairing failed with {bumble_address} reason: Authentication failure",
                    f"Disconnected: {bumble_address}",
                ],
            )
            assert found is True
        else:
            found, _ = await wait_for_shell_response(
                dut,
                [
                    f"Bonded with {bumble_address}",
                    f"Security changed: {bumble_address} level 4",
                ],
            )
            assert found is True

            logger.info('Creating L2CAP channel...')
            channel = await device.create_l2cap_channel(
                connection=connection, spec=ClassicChannelSpec(psm=l2cap_server_psm)
            )

            found, _ = await wait_for_shell_response(dut, r"Channel \w+ connected")
            assert found is True

            await channel.disconnect()
            found, _ = await wait_for_shell_response(dut, r"Channel \w+ disconnected")
            assert found is True

            await device.disconnect(connection, HCI_REMOTE_USER_TERMINATED_CONNECTION_ERROR)
            found, _ = await wait_for_shell_response(dut, f"Disconnected: {bumble_address}")
            assert found is True


async def sm_sc_rsp_005(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_sc_only_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        1,
    )


async def sm_sc_rsp_006(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_sc_only_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        2,
    )


async def sm_sc_rsp_007(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_sc_only_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        3,
    )


async def sm_sc_rsp_008(hci_port, shell, dut, address, snoop_file) -> None:
    await sm_sc_only_rsp(
        hci_port,
        shell,
        dut,
        address,
        snoop_file,
        4,
    )


async def sm_sc_rsp_009(hci_port, shell, dut, address, snoop_file) -> None:
    async def enter_pin_code(shell, dut):
        found, _ = await wait_for_shell_response(dut, f"Enter.*?PIN code for {bumble_address}")
        assert found is True
        await send_cmd_to_iut(shell, dut, f"br auth-pincode {pin_code}", None)

    class Delegate(PairingDelegate):
        async def get_string(self, max_length: int) -> str | None:
            """
            Return a string whose utf-8 encoding is up to max_length bytes.
            """
            logger.info('Enter PIN code on Bumble')
            asyncio.create_task(enter_pin_code(shell, dut))
            return pin_code

    dut_sec = 3
    pin_code = '0123456789ABCDEF'
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = bool(dut_sec != 3)
        device.classic_ssp_enabled = False

        delegate = Delegate(SMP_KEYBOARD_ONLY_IO_CAPABILITY)
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
        await send_cmd_to_iut(shell, dut, "bt auth input", None)
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br register {format(l2cap_server_psm, 'x')}", None
        )
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br security {format(l2cap_server_psm, 'x')} {dut_sec}", None
        )
        connection = await bumble_acl_connect(shell, dut, device, iut_address)

        logger.info('Authenticating...')
        await device.authenticate(connection)

        try:
            logger.info('Enabling encryption...')
            await device.encrypt(connection)
        except BaseException as e:
            if isinstance(e, HCI_Error):
                logger.info(f'Fail to encrypt, reason: {e.error_name}')
            else:
                raise e

        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Pairing failed with {bumble_address} reason: Authentication failure",
                f"Disconnected: {bumble_address}",
            ],
        )
        assert found is True


class TestSmpServer:
    def test_sm_sc_init_005(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that devices with "Secure Connection Only" support can
        establish a connection with Security Level 4."""
        logger.info(f'test_sm_sc_init_005 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_sc_init_005(hci, shell, dut, iut_address, snoop_file))

    def test_sm_sc_init_006(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that devices with "Secure Connection Only" support reject
        connection attempts with Security Level 1."""
        logger.info(f'test_sm_sc_init_006 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_sc_init_006(hci, shell, dut, iut_address, snoop_file))

    def test_sm_sc_init_007(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that devices with "Secure Connection Only" support reject
        connection attempts with Security Level 2."""
        logger.info(f'test_sm_sc_init_007 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_sc_init_007(hci, shell, dut, iut_address, snoop_file))

    def test_sm_sc_init_008(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that devices with "Secure Connection Only" support reject
        connection attempts with Security Level 3."""
        logger.info(f'test_sm_sc_init_008 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_sc_init_008(hci, shell, dut, iut_address, snoop_file))

    def test_sm_sc_init_009(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that an initiator cannot fallback to legacy pairing when "Secure connection Only"
        is selected and the responder doesn't support Secure Connections and SSP."""
        logger.info(f'test_sm_sc_init_009 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_sc_init_009(hci, shell, dut, iut_address, snoop_file))

    def test_sm_sc_rsp_005(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """To verify that Security Level 1 cannot be established when the
        device is configured to support only secure connection pairing methods."""
        logger.info(f'test_sm_sc_rsp_005 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_sc_rsp_005(hci, shell, dut, iut_address, snoop_file))

    def test_sm_sc_rsp_006(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """To verify that Security Level 2 cannot be established when the
        device is configured to support only secure connection pairing methods."""
        logger.info(f'test_sm_sc_rsp_006 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_sc_rsp_006(hci, shell, dut, iut_address, snoop_file))

    def test_sm_sc_rsp_007(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """To verify that Security Level 3 cannot be established when the
        device is configured to support only secure connection pairing methods."""
        logger.info(f'test_sm_sc_rsp_007 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_sc_rsp_007(hci, shell, dut, iut_address, snoop_file))

    def test_sm_sc_rsp_008(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """To verify that Security Level 4 can be successfully established when the
        device is configured to support only secure connection pairing methods."""
        logger.info(f'test_sm_sc_rsp_008 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_sc_rsp_008(hci, shell, dut, iut_address, snoop_file))

    def test_sm_sc_rsp_009(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify that an responder cannot fallback to legacy pairing when "Secure connection Only"
        is selected and the initiator doesn't support Secure Connections and SSP."""
        logger.info(f'test_sm_sc_rsp_009 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_sc_rsp_009(hci, shell, dut, iut_address, snoop_file))

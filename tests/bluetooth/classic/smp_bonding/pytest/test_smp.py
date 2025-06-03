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
from bumble.device import Device
from bumble.hci import Address, HCI_Write_Page_Timeout_Command
from bumble.l2cap import ClassicChannelSpec
from bumble.pairing import PairingConfig, PairingDelegate
from bumble.smp import (
    SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY,
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


async def sm_bonding_init_001(hci_port, shell, dut, address, snoop_file) -> None:
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = False

        delegate = PairingDelegate(SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True, mitm=False, bonding=False, delegate=delegate
        )

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        # Initiator as l2cap client: non-bondable
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt bondable off", None)

        # Responder as l2cap server: non-bondable
        server = device.l2cap_channel_manager.create_classic_server(
            spec=ClassicChannelSpec(psm=l2cap_server_psm)
        )
        assert server is not None

        # Initiator ceate connection
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )

        # Initiator l2cap connect
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec 2", None
        )

        # Initiator check pairing success
        found, lines = await wait_for_shell_response(
            dut,
            [f"Paired with {bumble_address}", f"Security changed: {bumble_address} level 2"],
        )
        assert found is True

        # Initiator check l2cap connection
        found = check_shell_response(lines, r"Channel \w+ connected")
        if not found:
            found, _ = await wait_for_shell_response(dut, [r"Channel \w+ connected"])
        assert found is True

        # Initiator disconnect
        await send_cmd_to_iut(shell, dut, "bt disconnect", f"Disconnected: {bumble_address}")

        # Initiator check bond removal
        lines = shell.exec_command("br bonds")
        assert check_shell_response(lines, r"Total 0")


async def sm_bonding_init_005(hci_port, shell, dut, address, snoop_file) -> None:
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = False

        delegate = PairingDelegate(SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True, mitm=False, bonding=True, delegate=delegate
        )

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        # Initiator as l2cap client: General Bonding
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt bondable on", None)

        # Responder as l2cap server: General Bonding
        server = device.l2cap_channel_manager.create_classic_server(
            spec=ClassicChannelSpec(psm=l2cap_server_psm)
        )
        assert server is not None

        # Initiator as l2cap client: General Bonding
        await send_cmd_to_iut(
            shell, dut, f"br connect {bumble_address}", f"Connected: {bumble_address}"
        )

        # Initiator l2cap connect
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br connect {format(l2cap_server_psm, 'x')} sec 2", None
        )

        # Initiator check bonding success
        found, lines = await wait_for_shell_response(
            dut,
            [f"Bonded with {bumble_address}", f"Security changed: {bumble_address} level 2"],
        )
        assert found is True

        # Initiator check l2cap connection
        found = check_shell_response(lines, r"Channel \w+ connected")
        if not found:
            found, _ = await wait_for_shell_response(dut, [r"Channel \w+ connected"])
        assert found is True

        # Initiator disconnect
        await send_cmd_to_iut(shell, dut, "bt disconnect", f"Disconnected: {bumble_address}")

        # Initiator check bonded
        lines = shell.exec_command("br bonds")
        assert check_shell_response(lines, f"Remote Identity: {bumble_address}")


async def sm_bonding_rsp_010(hci_port, shell, dut, address, snoop_file) -> None:
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = False

        delegate = PairingDelegate(SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True, mitm=False, bonding=False, delegate=delegate
        )

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        iut_address = address.split(" ")[0]

        # Responder as l2cap server: non-bondable
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt bondable off", None)
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br register {format(l2cap_server_psm, 'x')}", None
        )
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br security {format(l2cap_server_psm, 'x')} 2", None
        )

        # Initiator as l2cap client: non-bondable
        # Initiator create connection
        connection = await device.connect(iut_address, transport=BT_BR_EDR_TRANSPORT)
        # Responder check connection
        found, _ = await wait_for_shell_response(dut, [f"Connected: {bumble_address}"])
        assert found is True

        # Initiator init authentication and encryption
        await device.authenticate(connection)
        await device.encrypt(connection)

        # Responder check pairing success
        found, _ = await wait_for_shell_response(
            dut,
            [f"Paired with {bumble_address}", f"Security changed: {bumble_address} level 2"],
        )
        assert found is True

        # Initiator create l2cap connection
        await device.l2cap_channel_manager.create_classic_channel(
            connection=connection, spec=ClassicChannelSpec(psm=l2cap_server_psm)
        )

        # Responder check l2cap channel connection
        found, _ = await wait_for_shell_response(dut, [r"Channel \w+ connected"])
        assert found is True

        # Responder disconnect
        await send_cmd_to_iut(shell, dut, "bt disconnect", f"Disconnected: {bumble_address}")

        # Responder check no bonds
        lines = shell.exec_command("br bonds")
        assert check_shell_response(lines, r"Total 0")


async def sm_bonding_rsp_011(hci_port, shell, dut, address, snoop_file) -> None:
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = False

        delegate = PairingDelegate(SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True, mitm=False, bonding=True, delegate=delegate
        )

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        iut_address = address.split(" ")[0]

        # Responder as l2cap server: non-bondable
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt bondable off", None)
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br register {format(l2cap_server_psm, 'x')}", None
        )
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br security {format(l2cap_server_psm, 'x')} 2", None
        )

        # Initiator as l2cap client: General Bonding
        # Initiator create connection
        connection = await device.connect(iut_address, transport=BT_BR_EDR_TRANSPORT)
        found, _ = await wait_for_shell_response(dut, [f"Connected: {bumble_address}"])
        assert found is True

        # Initiator init authentication and encryption
        try:
            await device.authenticate(connection)
            await device.encrypt(connection)
        except Exception as e:
            logger.error(f"Authentication or encryption failed: {e}")

        # Responder check pairing failure
        found, _ = await wait_for_shell_response(
            dut,
            [
                f"Pairing failed with {bumble_address} reason: Authentication failure",
                rf"Disconnected: {bumble_address} \(reason 0x16\)",
            ],
        )
        assert found is True


async def sm_bonding_rsp_014(hci_port, shell, dut, address, snoop_file) -> None:
    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )

        device.classic_enabled = True
        device.le_enabled = False
        device.classic_sc_enabled = False

        delegate = PairingDelegate(SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY)
        device.pairing_config_factory = lambda connection: PairingConfig(
            sc=True, mitm=False, bonding=True, delegate=delegate
        )

        device.host.snooper = BtSnooper(snoop_file)
        await device_power_on(device)
        await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
        bumble_address = device.public_address.to_string().split('/P')[0]

        iut_address = address.split(" ")[0]

        # Responder as l2cap server: General Bonding
        await send_cmd_to_iut(shell, dut, "bt clear all", None)
        await send_cmd_to_iut(shell, dut, "br clear all", None)
        await send_cmd_to_iut(shell, dut, "bt auth none", None)
        await send_cmd_to_iut(shell, dut, "bt bondable on", None)
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br register {format(l2cap_server_psm, 'x')}", None
        )
        await send_cmd_to_iut(
            shell, dut, f"l2cap_br security {format(l2cap_server_psm, 'x')} 2", None
        )

        # Initiator as l2cap client: General Bonding
        # Initiator create connection
        connection = await device.connect(iut_address, transport=BT_BR_EDR_TRANSPORT)
        # Responder check connection
        found, _ = await wait_for_shell_response(dut, [f"Connected: {bumble_address}"])
        assert found is True

        # Initiator init authentication and encryption
        await device.authenticate(connection)
        await device.encrypt(connection)

        # Responder check bonding success
        found, _ = await wait_for_shell_response(
            dut,
            [f"Bonded with {bumble_address}", f"Security changed: {bumble_address} level 2"],
        )
        assert found is True

        # Initiator create l2cap connection
        await device.l2cap_channel_manager.create_classic_channel(
            connection=connection, spec=ClassicChannelSpec(psm=l2cap_server_psm)
        )

        # Responder check l2cap channel connection
        found, _ = await wait_for_shell_response(dut, [r"Channel \w+ connected"])
        assert found is True

        # Responder disconnect
        await send_cmd_to_iut(shell, dut, "bt disconnect", f"Disconnected: {bumble_address}")

        # Responder check bonds
        lines = shell.exec_command("br bonds")
        assert check_shell_response(lines, f"Remote Identity: {bumble_address}")


class TestSmpServer:
    def test_sm_bonding_init_001(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify pairing can be established when both devices are in Non-bondable mode with
        local device as L2CAP client."""
        logger.info(f'test_sm_bonding_init_001 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_bonding_init_001(hci, shell, dut, iut_address, snoop_file))

    def test_sm_bonding_init_005(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """Verify pairing and bonding can be established when both devices are
        in General Bonding mode."""
        logger.info(f'test_sm_bonding_init_005 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_bonding_init_005(hci, shell, dut, iut_address, snoop_file))

    def test_sm_bonding_rsp_010(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """To verify that pairing succeeds when both local and peer devices are configured
        as Non-bondable and the local device acts as an L2CAP server."""
        logger.info(f'test_sm_bonding_rsp_010 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_bonding_rsp_010(hci, shell, dut, iut_address, snoop_file))

    def test_sm_bonding_rsp_011(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """To verify that pairing fails when local device is Non-bondable and peer device
        is General Bonding, with the local device acting as an L2CAP server."""
        logger.info(f'test_sm_bonding_rsp_011 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_bonding_rsp_011(hci, shell, dut, iut_address, snoop_file))

    def test_sm_bonding_rsp_014(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """To verify that pairing succeeds when both local and peer devices are configured
        as General Bonding, with the local device acting as an L2CAP server."""
        logger.info(f'test_sm_bonding_rsp_014 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            asyncio.run(sm_bonding_rsp_014(hci, shell, dut, iut_address, snoop_file))

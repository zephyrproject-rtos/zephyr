# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0


import asyncio
import contextlib
import logging
import re
import sys
import time

from bumble.core import (
    BT_BR_EDR_TRANSPORT,
)
from bumble.device import (
    Device,
)
from bumble.hci import (
    Address,
    HCI_Error,
    HCI_Write_Page_Timeout_Command,
)
from bumble.pairing import PairingConfig, PairingDelegate
from bumble.smp import (
    SMP_NO_INPUT_NO_OUTPUT_IO_CAPABILITY,
)
from bumble.snoop import BtSnooper
from bumble.transport import open_transport_or_link
from twister_harness import DeviceAdapter, Shell

logger = logging.getLogger(__name__)


def app_handle_device_output(self) -> None:
    """
    This method is dedicated to run it in separate thread to read output
    from device and put them into internal queue and save to log file.
    """
    with open(self.handler_log_path, 'a+') as log_file:
        while self.is_device_running():
            if self.is_device_connected():
                output = self._read_device_output().decode(errors='replace').rstrip("\r\n")
                if output:
                    self._device_read_queue.put(output)
                    logger.debug(f'{output}\n')
                    try:
                        log_file.write(f'{output}\n')
                    except Exception:
                        contextlib.suppress(Exception)
                    log_file.flush()
            else:
                # ignore output from device
                self._flush_device_output()
                time.sleep(0.1)


# After reboot, there may be gbk character in the console, so replace _handle_device_output to
# handle the exception.
DeviceAdapter._handle_device_output = app_handle_device_output


# power on dongle
async def device_power_on(device) -> None:
    while True:
        try:
            await device.power_on()
            break
        except Exception:
            continue


async def wait_for_shell_response(dut, regex: list[str] | str, max_wait_sec=10):
    found = False
    lines = []
    dct = {}

    logger.debug('wait_for_shell_response')

    if isinstance(regex, str):
        messages = [regex]
    else:
        messages = regex

    for message in messages:
        dct[message] = False

    try:
        for _ in range(0, max_wait_sec):
            read_lines = dut.readlines(print_output=True)
            for line in read_lines:
                logger.debug(f'DUT response: {str(line)}')
            lines += read_lines
            for message in messages:
                for line in read_lines:
                    if re.search(message, line):
                        dct[message] = True
                        for key in dct:
                            if dct[key] is False:
                                found = False
                                break
                        else:
                            found = True
                        break
            if found is True:
                break
            await asyncio.sleep(1)
    except Exception as e:
        logger.error(f'{e}!', exc_info=True)
        raise e

    for key in dct:
        logger.debug(f'Expected DUT response: "{key}", Matched: {dct[key]}')

    return found, lines


# interact between script and DUT
async def send_cmd_to_iut(shell, dut, cmd, response=None, max_wait_sec=20):
    found = False
    lines = []
    exec_cmd_lines = shell.exec_command(cmd, timeout=20)
    if exec_cmd_lines is not None:
        lines += exec_cmd_lines

    if response is not None:
        if exec_cmd_lines is not None:
            for line in exec_cmd_lines:
                if response in line:
                    found = True
                    break
        if not found:
            found, wait_lines = await wait_for_shell_response(dut, response, max_wait_sec)
            lines += wait_lines
    else:
        found = True
    assert found is True
    return lines


async def bumble_acl_connect(shell, dut, device, target_address):
    connection = None
    try:
        connection = await device.connect(target_address, transport=BT_BR_EDR_TRANSPORT, timeout=30)
        logger.info(f'=== Connected to {connection.peer_address}!')
    except BaseException as e:
        logger.error(f'Fail to connect to {target_address}!')
        raise e
    return connection


async def sm_test_initial_connect(device, shell, dut, bumble_address, iut_address) -> None:
    # Delete BR/EDR bonding information
    await device.keystore.delete(iut_address + '/P')
    await send_cmd_to_iut(shell, dut, "br clear all")
    await send_cmd_to_iut(shell, dut, "bt auth none")
    await send_cmd_to_iut(shell, dut, "bt auth status")
    connection = await bumble_acl_connect(shell, dut, device, iut_address)

    logger.info('Authenticating...')
    await device.authenticate(connection)

    logger.info('Enabling encryption...')
    await device.encrypt(connection)

    found, _ = await wait_for_shell_response(
        dut,
        [
            f"Bonded with {bumble_address}",
            f"Security changed: {bumble_address} level",
        ],
    )
    assert found is True
    return connection


async def sm_test_initial_disconnect(dut, connection) -> None:
    logger.info('disconnect...')
    await connection.disconnect()
    found, _ = await wait_for_shell_response(
        dut,
        [
            "Disconnected",
        ],
    )
    assert found is True


async def sm_test_reboot(shell, dut) -> None:
    logger.info('reboot DUT...')
    command = "test_smp reboot"
    command_ext = f'{command}\n'
    dut.clear_buffer()
    dut.write(command_ext.encode())

    found, _ = await wait_for_shell_response(
        dut,
        [
            "PROJECT EXECUTION SUCCESSFUL",
        ],
    )
    assert found is True

    await send_cmd_to_iut(shell, dut, "bt init", "Settings Loaded", max_wait_sec=30)
    await send_cmd_to_iut(shell, dut, "br pscan on")
    await send_cmd_to_iut(shell, dut, "br iscan on")
    logger.info('reboot initialized')


async def sm_test_reconnect_no_pair(device, shell, dut, bumble_address, iut_address) -> None:
    logger.info('connect again...')
    connection = await bumble_acl_connect(shell, dut, device, iut_address)

    logger.info('Authenticating...')
    await device.authenticate(connection)

    logger.info('Enabling encryption...')
    await device.encrypt(connection)

    found, log_lines = await wait_for_shell_response(
        dut, f"Security changed: {bumble_address} level"
    )
    assert found is True

    for line in log_lines:
        if 'Remote pairing' in line:
            raise AssertionError('re-pairing is seen')

    return connection


async def sm_test_reconnect_key_miss(device, shell, dut, iut_address) -> None:
    logger.info('connect again...')
    connection = await bumble_acl_connect(shell, dut, device, iut_address)
    logger.info('Authenticating...')
    try:
        await device.authenticate(connection)
    except BaseException as e:
        if isinstance(e, HCI_Error) and e.error_code == 0x6:  # HCI_PIN_OR_KEY_MISSING_ERROR
            logger.info('HCI_PIN_OR_KEY_MISSING_ERROR mean success')
            return

    raise AssertionError('no HCI_PIN_OR_KEY_MISSING_ERROR')


async def sm_key_persist_001(hci_port, shell, dut, address) -> None:
    log_name = sys._getframe().f_code.co_name

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
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
            mitm=True,
            bonding=True,
            delegate=delegate,
        )

        with open(f"bumble_hci_{log_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
            bumble_address = device.public_address.to_string().split('/P')[0]
            iut_address = address.split(" ")[0]

            connection = await sm_test_initial_connect(
                device, shell, dut, bumble_address, iut_address
            )
            await sm_test_initial_disconnect(dut, connection)

            connection = await sm_test_reconnect_no_pair(
                device, shell, dut, bumble_address, iut_address
            )

            logger.info('disconnect for next test')
            await sm_test_initial_disconnect(dut, connection)


async def sm_key_persist_002(hci_port, shell, dut, address) -> None:
    log_name = sys._getframe().f_code.co_name

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
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
            mitm=True,
            bonding=True,
            delegate=delegate,
        )

        with open(f"bumble_hci_{log_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
            bumble_address = device.public_address.to_string().split('/P')[0]
            iut_address = address.split(" ")[0]

            connection = await sm_test_initial_connect(
                device, shell, dut, bumble_address, iut_address
            )
            await sm_test_initial_disconnect(dut, connection)

            await sm_test_reboot(shell, dut)

            connection = await sm_test_reconnect_no_pair(
                device, shell, dut, bumble_address, iut_address
            )

            logger.info('disconnect for next test')
            await sm_test_initial_disconnect(dut, connection)


async def sm_key_persist_003(hci_port, shell, dut, address) -> None:
    log_name = sys._getframe().f_code.co_name

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
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
            mitm=True,
            bonding=True,
            delegate=delegate,
        )

        with open(f"bumble_hci_{log_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
            bumble_address = device.public_address.to_string().split('/P')[0]
            iut_address = address.split(" ")[0]

            connection = await sm_test_initial_connect(
                device, shell, dut, bumble_address, iut_address
            )
            await sm_test_initial_disconnect(dut, connection)

            await send_cmd_to_iut(shell, dut, "br clear all", "Pairings successfully cleared")

            await sm_test_reboot(shell, dut)

            await sm_test_reconnect_key_miss(device, shell, dut, iut_address)


async def sm_key_persist_004(hci_port, shell, dut, address) -> None:
    log_name = sys._getframe().f_code.co_name

    async with await open_transport_or_link(hci_port) as hci_transport:
        device = Device.with_hci(
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
            mitm=True,
            bonding=True,
            delegate=delegate,
        )

        with open(f"bumble_hci_{log_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))
            bumble_address = device.public_address.to_string().split('/P')[0]
            iut_address = address.split(" ")[0]

            connection = await sm_test_initial_connect(
                device, shell, dut, bumble_address, iut_address
            )

            await send_cmd_to_iut(shell, dut, "br clear all", "Pairings successfully cleared")

            await sm_test_initial_disconnect(dut, connection)

            await sm_test_reboot(shell, dut)

            await sm_test_reconnect_key_miss(device, shell, dut, iut_address)


class TestSmpServer:
    def test_sm_key_persist_001(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """
        Verify that the DUT correctly stores and retrieves Bluetooth BR/EDR link keys after
        a successful pairing, allowing subsequent connections without repeating the pairing process.
        """
        logger.info(f'test_sm_key_persist_001 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        asyncio.run(sm_key_persist_001(hci, shell, dut, iut_address))

    def test_sm_key_persist_002(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """
        Verify that the DUT correctly maintains Bluetooth BR/EDR link keys in non-volatile storage
        and can retrieve them after a complete power cycle.
        """
        logger.info(f'test_sm_key_persist_002 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        asyncio.run(sm_key_persist_002(hci, shell, dut, iut_address))

    def test_sm_key_persist_003(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """
        Verify that the DUT correctly deletes stored Bluetooth BR/EDR link keys (with disconnected)
        when requested through the appropriate API calls.
        """
        logger.info(f'test_sm_key_persist_003 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        asyncio.run(sm_key_persist_003(hci, shell, dut, iut_address))

    def test_sm_key_persist_004(self, shell: Shell, dut: DeviceAdapter, smp_initiator_dut):
        """
        Verify that the DUT correctly deletes stored Bluetooth BR/EDR link keys (with connected)
        when requested through the appropriate API calls.
        """
        logger.info(f'test_sm_key_persist_004 {smp_initiator_dut}')
        hci, iut_address = smp_initiator_dut
        asyncio.run(sm_key_persist_004(hci, shell, dut, iut_address))

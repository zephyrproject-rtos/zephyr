# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
import asyncio
import logging
import sys

from bumble import hci
from bumble.core import BT_BR_EDR_TRANSPORT, DeviceClass
from bumble.device import DEVICE_DEFAULT_INQUIRY_LENGTH, Device
from bumble.hci import Address, HCI_Write_Page_Timeout_Command
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


# wait for shell response
async def _wait_for_shell_response(dut, response, max_wait_sec=20):
    """
    _wait_for_shell_response() is used to wait for shell response.
    It will return after finding a specific 'response' or waiting long enough.
    :param dut:
    :param response: shell response that you want to monitor.
    :param max_wait_sec: maximum waiting time
    :return: found: whether the 'response' is found; lines: DUT shell response
    """
    found = False
    lines = []
    try:
        for _ in range(0, max_wait_sec):
            read_lines = dut.readlines()
            for line in read_lines:
                if response in line:
                    found = True
                    break
            lines = lines + read_lines
            await asyncio.sleep(1)
        logger.info(f'{str(lines)}')
    except Exception as e:
        logger.error(f'{e}!', exc_info=True)
        raise e
    return found, lines


# interact between script and DUT
async def send_cmd_to_iut(
    shell, dut, cmd, response=None, expect_to_find_resp=True, max_wait_sec=20
):
    """
    send_cmd_to_iut() is used to send shell cmd to DUT and monitor the response.
    It can choose whether to monitor the shell response of DUT.
    Use 'expect_to_find_resp' to set whether to expect the response to contain certain 'response'.
    'max_wait_sec' indicates the maximum waiting time.
    For 'expect_to_find_resp=False', this is useful
    because we need to wait long enough to get enough response
    to more accurately judge that the response does not contain specific characters.

    :param shell:
    :param dut:
    :param cmd: shell cmd sent to DUT
    :param response: shell response that you want to monitor.
                     'None' means not to monitor any response.
    :param expect_to_find_resp: set whether to expect the response to contain certain 'response'
    :param max_wait_sec: maximum monitoring time
    :return: DUT shell response
    """
    shell.exec_command(cmd)
    if response is not None:
        found, lines = await _wait_for_shell_response(dut, response, max_wait_sec)
    else:
        found = True
        lines = ''
    assert found is expect_to_find_resp
    return lines


# dongle limited discovery
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


# device listener for receiving scan results
class DiscoveryListener(Device.Listener):
    def __init__(self):
        self.discovered_addresses = set()

    def on_inquiry_result(self, address, class_of_device, data, rssi):
        DeviceClass.split_class_of_device(class_of_device)
        found_address = str(address).replace(r'/P', '')
        logger.info(f'Found addr: {found_address}')
        self.discovered_addresses.add(found_address)

    def has_found_target_addr(self, target_addr):
        logger.info(f'Target addr: {target_addr} ...')
        return str(target_addr).upper() in self.discovered_addresses


async def tc_gap_s_1(hci_port, shell, dut, address) -> None:
    """Non-connectable Mode Testing"""
    case_name = 'GAP-C-1: Non-connectable Mode Testing'
    logger.info(f'<<< Start {case_name} ...')
    dut_address = address.split(" ")[0]

    async with await open_transport_or_link(hci_port) as hci_transport:
        # init PC bluetooth env
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.listener = DiscoveryListener()

        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Start of Initial Condition
            await send_cmd_to_iut(shell, dut, "br pscan off")
            await send_cmd_to_iut(shell, dut, "br iscan off")
            # End of Initial Condition

            # Test Start
            logger.info('Step 1: Configure the DUT to operate in non-connectable mode')
            await send_cmd_to_iut(shell, dut, "br pscan off")

            logger.info('Step 2: Configure the DUT to operate in non-discoverable mode')
            await send_cmd_to_iut(shell, dut, "br iscan off")

            logger.info('Step 3: Tester searches for DUT advertisements')
            # Try general inquiry
            await device.start_discovery()
            await asyncio.sleep(10)
            await device.stop_discovery()
            assert not device.listener.has_found_target_addr(
                dut_address
            ), "DUT should not be visible in Tester's scan results"

            # Try limited inquiry
            await start_limited_discovery(device)
            await asyncio.sleep(10)
            await device.stop_discovery()

            # Verify DUT is not visible
            assert not device.listener.has_found_target_addr(
                dut_address
            ), "DUT should not be visible in Tester's scan results"

            logger.info('Step 4: Tester attempts to establish connection with DUT')
            try:
                await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT, timeout=10)
                # If we reach here, connection was successful, which is not expected
                logger.info("DUT should not accept any connection requests from Tester")
            except Exception:
                # Connection failure is expected
                logger.info('Expected connection failure in non-connectable mode')

            # Verify there was no connection established
            await asyncio.sleep(2)
            found, _ = await _wait_for_shell_response(dut, "Connected", max_wait_sec=5)
            assert not found, "DUT should not have established connection"


async def tc_gap_s_2(hci_port, shell, dut, address) -> None:
    """Connectable Non-discoverable Mode with Active Disconnection"""
    case_name = 'GAP-C-2: Connectable Non-discoverable Mode with Active Disconnection'
    logger.info(f'<<< Start {case_name} ...')
    dut_address = address.split(" ")[0]

    async with await open_transport_or_link(hci_port) as hci_transport:
        # init PC bluetooth env
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.listener = DiscoveryListener()

        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Start of Initial Condition
            await send_cmd_to_iut(shell, dut, "br pscan off")
            await send_cmd_to_iut(shell, dut, "br iscan off")
            # End of Initial Condition

            logger.info('Step 1: Configure the DUT to operate in connectable mode')
            await send_cmd_to_iut(shell, dut, "br pscan on")

            logger.info('Step 2: Configure the DUT to operate in non-discoverable mode')
            await send_cmd_to_iut(shell, dut, "br iscan off")

            logger.info('Step 3: Verify DUT cannot be discovered')
            await device.start_discovery()
            await asyncio.sleep(10)
            await device.stop_discovery()
            assert not device.listener.has_found_target_addr(
                dut_address
            ), "Device was discovered but should be non-discoverable"
            await start_limited_discovery(device)
            await asyncio.sleep(10)
            await device.stop_discovery()
            assert not device.listener.has_found_target_addr(
                dut_address
            ), "Device was discovered but should be non-discoverable"

            logger.info(
                'Step 4: Tester attempts to establish connection with DUT using known address'
            )
            await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT)

            logger.info('Step 5: DUT accepts connection request')
            # passive

            logger.info('Step 6: DUT initiates disconnection')
            await send_cmd_to_iut(shell, dut, "bt disconnect", "Disconnected")


async def tc_gap_s_3(hci_port, shell, dut, address) -> None:
    """Connectable Non-discoverable Mode with Passive Disconnection"""
    case_name = 'GAP-C-3: Connectable Non-discoverable Mode with Passive Disconnection'
    logger.info(f'<<< Start {case_name} ...')
    dut_address = address.split(" ")[0]

    async with await open_transport_or_link(hci_port) as hci_transport:
        # init PC bluetooth env
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.listener = DiscoveryListener()

        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Start of Initial Condition
            await send_cmd_to_iut(shell, dut, "br pscan off")
            await send_cmd_to_iut(shell, dut, "br iscan off")
            # End of Initial Condition

            # Test Start
            logger.info('Step 1: Configure the DUT to operate in connectable mode')
            await send_cmd_to_iut(shell, dut, "br pscan on")

            logger.info('Step 2: Configure the DUT to operate in non-discoverable mode')
            await send_cmd_to_iut(shell, dut, "br iscan off")

            logger.info('Step 3: Verify DUT cannot be discovered')
            await device.start_discovery()
            await asyncio.sleep(10)
            await device.stop_discovery()
            assert not device.listener.has_found_target_addr(
                dut_address
            ), "Device was discovered but should be non-discoverable"

            logger.info(
                'Step 4: Tester attempts to establish connection with DUT using known address'
            )
            connection = await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT)

            logger.info('Step 5: DUT accepts connection request')
            found, _ = await _wait_for_shell_response(dut, "Connected", max_wait_sec=5)
            assert found, "DUT should accept connection request"

            logger.info('Step 6: Tester initiates disconnection')
            await connection.disconnect()

            logger.info('Step 7: Verify disconnection is complete')
            found, _ = await _wait_for_shell_response(dut, "Disconnected", max_wait_sec=5)
            assert found, "DUT should properly handle disconnection initiated by Tester"


async def tc_gap_s_4(hci_port, shell, dut, address) -> None:
    """Connectable Non-discoverable Mode with Connection Rejection"""
    case_name = 'GAP-C-4: Connectable Non-discoverable Mode with Connection Rejection'
    logger.info(f'<<< Start {case_name} ...')
    dut_address = address.split(" ")[0]

    async with await open_transport_or_link(hci_port) as hci_transport:
        # init PC bluetooth env
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.listener = DiscoveryListener()

        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Start of Initial Condition
            await send_cmd_to_iut(shell, dut, "br pscan off")
            await send_cmd_to_iut(shell, dut, "br iscan off")
            # End of Initial Condition

            # Test Start
            logger.info('Step 1: Configure the DUT to operate in connectable mode')
            await send_cmd_to_iut(shell, dut, "br pscan on")

            logger.info('Step 2: Configure the DUT to operate in non-discoverable mode')
            await send_cmd_to_iut(shell, dut, "br iscan off")

            logger.info('Step 3: Verify DUT cannot be discovered')
            await device.start_discovery()
            await asyncio.sleep(10)
            await device.stop_discovery()
            assert not device.listener.has_found_target_addr(
                dut_address
            ), "Device was discovered but should be non-discoverable"

            logger.info('Step 4: Configure the DUT to reject connection requests')
            # Make the DUT non-connectable to simulate connection rejection
            await send_cmd_to_iut(shell, dut, "br pscan off")

            logger.info(
                'Step 5: Tester attempts to establish connection with DUT using known address'
            )
            try:
                await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT, timeout=10)
                logger.info("Connection should be rejected")
            except Exception:
                logger.info('Expected connection failure when connection is rejected')

            logger.info('Step 6: Verify connection was rejected')
            found, _ = await _wait_for_shell_response(dut, "Connected", max_wait_sec=5)
            assert not found, "DUT should reject connection request"


async def tc_gap_s_5(hci_port, shell, dut, address) -> None:
    """Limited Discoverable Mode with Active Disconnection"""
    case_name = 'GAP-C-5: Limited Discoverable Mode with Active Disconnection'
    logger.info(f'<<< Start {case_name} ...')
    dut_address = address.split(" ")[0]

    async with await open_transport_or_link(hci_port) as hci_transport:
        # init PC bluetooth env
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.listener = DiscoveryListener()

        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Start of Initial Condition
            await send_cmd_to_iut(shell, dut, "br pscan off")
            await send_cmd_to_iut(shell, dut, "br iscan off")
            # End of Initial Condition

            # Test Start
            logger.info('Step 1: Configure the DUT to operate in connectable mode')
            await send_cmd_to_iut(shell, dut, "br pscan on")

            logger.info('Step 2: Configure the DUT to operate in limited discoverable mode')
            await send_cmd_to_iut(shell, dut, "br iscan off")
            await send_cmd_to_iut(shell, dut, "br iscan on limited")

            logger.info('Step 3: Tester performs limited discovery procedure')
            await start_limited_discovery(device)
            await asyncio.sleep(10)
            await device.stop_discovery()

            logger.info('Step 4: Verify DUT is discovered in limited discovery')
            assert device.listener.has_found_target_addr(
                dut_address
            ), "DUT should be discoverable in limited discovery procedure"

            logger.info('Step 5: Tester attempts to establish connection with discovered DUT')
            await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT)

            logger.info('Step 6: DUT accepts connection request')
            found, _ = await _wait_for_shell_response(dut, "Connected", max_wait_sec=5)
            assert found, "DUT should accept connection request"

            logger.info('Step 7: DUT initiates disconnection')
            await send_cmd_to_iut(shell, dut, "bt disconnect", "Disconnected")

            logger.info('Step 8: Verify disconnection is complete')
            # Already verified by response check in previous step


async def tc_gap_s_6(hci_port, shell, dut, address) -> None:
    """Limited Discoverable Mode with Passive Disconnection"""
    case_name = 'GAP-C-6: Limited Discoverable Mode with Passive Disconnection'
    logger.info(f'<<< Start {case_name} ...')
    dut_address = address.split(" ")[0]

    async with await open_transport_or_link(hci_port) as hci_transport:
        # init PC bluetooth env
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.listener = DiscoveryListener()

        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Start of Initial Condition
            await send_cmd_to_iut(shell, dut, "br pscan off")
            await send_cmd_to_iut(shell, dut, "br iscan off")
            # End of Initial Condition

            # Test Start
            logger.info('Step 1: Configure the DUT to operate in connectable mode')
            await send_cmd_to_iut(shell, dut, "br pscan on")

            logger.info('Step 2: Configure the DUT to operate in limited discoverable mode')
            await send_cmd_to_iut(shell, dut, "br iscan off")
            await send_cmd_to_iut(shell, dut, "br iscan on limited")

            logger.info('Step 3: Tester performs limited discovery procedure')
            await start_limited_discovery(device)
            await asyncio.sleep(10)
            await device.stop_discovery()

            logger.info('Step 4: Verify DUT is discovered in limited discovery')
            assert device.listener.has_found_target_addr(
                dut_address
            ), "DUT should be discoverable in limited discovery procedure"

            logger.info('Step 5: Tester attempts to establish connection with discovered DUT')
            connection = await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT)

            logger.info('Step 6: DUT accepts connection request')
            found, _ = await _wait_for_shell_response(dut, "Connected", max_wait_sec=5)
            assert found, "DUT should accept connection request"

            logger.info('Step 7: Tester initiates disconnection')
            await connection.disconnect()

            logger.info('Step 8: Verify disconnection is complete')
            found, _ = await _wait_for_shell_response(dut, "Disconnected", max_wait_sec=5)
            assert found, "DUT should properly handle disconnection initiated by Tester"


async def tc_gap_s_7(hci_port, shell, dut, address) -> None:
    """Limited Discoverable Mode with Connection Rejection"""
    case_name = 'GAP-C-7: Limited Discoverable Mode with Connection Rejection'
    logger.info(f'<<< Start {case_name} ...')
    dut_address = address.split(" ")[0]

    async with await open_transport_or_link(hci_port) as hci_transport:
        # init PC bluetooth env
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.listener = DiscoveryListener()

        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Start of Initial Condition
            await send_cmd_to_iut(shell, dut, "br pscan off")
            await send_cmd_to_iut(shell, dut, "br iscan off")
            # End of Initial Condition

            # Test Start
            logger.info('Step 1: Configure the DUT to operate in connectable mode')
            await send_cmd_to_iut(shell, dut, "br pscan on")

            logger.info('Step 2: Configure the DUT to operate in limited discoverable mode')
            await send_cmd_to_iut(shell, dut, "br iscan off")
            await send_cmd_to_iut(shell, dut, "br iscan on limited")

            logger.info('Step 3: Tester performs limited discovery procedure')
            await start_limited_discovery(device)
            await asyncio.sleep(10)
            await device.stop_discovery()

            logger.info('Step 4: Verify DUT is discovered in limited discovery')
            assert device.listener.has_found_target_addr(
                dut_address
            ), "DUT should be discoverable in limited discovery procedure"

            logger.info('Step 5: Configure the DUT to reject connection requests')
            # Make the DUT non-connectable to simulate connection rejection
            await send_cmd_to_iut(shell, dut, "br pscan off")

            logger.info('Step 6: Tester attempts to establish connection with discovered DUT')
            try:
                await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT, timeout=10)
                logger.info("Connection should be rejected")
            except Exception:
                logger.info('Expected connection failure when connection is rejected')

            logger.info('Step 7: Verify connection was rejected')
            found, _ = await _wait_for_shell_response(dut, "Connected", max_wait_sec=5)
            assert not found, "DUT should reject connection request"


async def tc_gap_s_8(hci_port, shell, dut, address) -> None:
    """General Discoverable Mode with Active Disconnection"""
    case_name = 'GAP-C-8: General Discoverable Mode with Active Disconnection'
    logger.info(f'<<< Start {case_name} ...')
    dut_address = address.split(" ")[0]

    async with await open_transport_or_link(hci_port) as hci_transport:
        # init PC bluetooth env
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.listener = DiscoveryListener()

        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Start of Initial Condition
            await send_cmd_to_iut(shell, dut, "br pscan off")
            await send_cmd_to_iut(shell, dut, "br iscan off")
            # End of Initial Condition

            # Test Start
            logger.info('Step 1: Configure the DUT to operate in connectable mode')
            await send_cmd_to_iut(shell, dut, "br pscan on")

            logger.info('Step 2: Configure the DUT to operate in general discoverable mode')
            await send_cmd_to_iut(shell, dut, "br iscan on")

            logger.info('Step 3: Tester performs general discovery procedure')
            await device.start_discovery()
            await asyncio.sleep(10)
            await device.stop_discovery()

            logger.info('Step 4: Verify DUT is discovered in general discovery')
            assert device.listener.has_found_target_addr(
                dut_address
            ), "DUT should be discoverable in general discovery procedure"

            logger.info('Step 5: Tester attempts to establish connection with discovered DUT')
            await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT)

            logger.info('Step 6: DUT accepts connection request')
            found, _ = await _wait_for_shell_response(dut, "Connected", max_wait_sec=5)
            assert found, "DUT should accept connection request"

            logger.info('Step 7: DUT initiates disconnection')
            await send_cmd_to_iut(shell, dut, "bt disconnect", "Disconnected")

            logger.info('Step 8: Verify disconnection is complete')
            # Already verified by response check in previous step


async def tc_gap_s_9(hci_port, shell, dut, address) -> None:
    """General Discoverable Mode with Passive Disconnection"""
    case_name = 'GAP-C-9: General Discoverable Mode with Passive Disconnection'
    logger.info(f'<<< Start {case_name} ...')
    dut_address = address.split(" ")[0]

    async with await open_transport_or_link(hci_port) as hci_transport:
        # init PC bluetooth env
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.listener = DiscoveryListener()

        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Start of Initial Condition
            await send_cmd_to_iut(shell, dut, "br pscan off")
            await send_cmd_to_iut(shell, dut, "br iscan off")
            # End of Initial Condition

            # Test Start
            logger.info('Step 1: Configure the DUT to operate in connectable mode')
            await send_cmd_to_iut(shell, dut, "br pscan on")

            logger.info('Step 2: Configure the DUT to operate in general discoverable mode')
            await send_cmd_to_iut(shell, dut, "br iscan on")

            logger.info('Step 3: Tester performs general discovery procedure')
            await device.start_discovery()
            await asyncio.sleep(10)
            await device.stop_discovery()

            logger.info('Step 4: Verify DUT is discovered in general discovery')
            assert device.listener.has_found_target_addr(
                dut_address
            ), "DUT should be discoverable in general discovery procedure"

            logger.info('Step 5: Tester attempts to establish connection with discovered DUT')
            connection = await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT)

            logger.info('Step 6: DUT accepts connection request')
            found, _ = await _wait_for_shell_response(dut, "Connected", max_wait_sec=5)
            assert found, "DUT should accept connection request"

            logger.info('Step 7: Tester initiates disconnection')
            await connection.disconnect()

            logger.info('Step 8: Verify disconnection is complete')
            found, _ = await _wait_for_shell_response(dut, "Disconnected", max_wait_sec=5)
            assert found, "DUT should properly handle disconnection initiated by Tester"


async def tc_gap_s_10(hci_port, shell, dut, address) -> None:
    """General Discoverable Mode with Connection Rejection"""
    case_name = 'GAP-C-10: General Discoverable Mode with Connection Rejection'
    logger.info(f'<<< Start {case_name} ...')
    dut_address = address.split(" ")[0]

    async with await open_transport_or_link(hci_port) as hci_transport:
        # init PC bluetooth env
        device = Device.with_hci(
            'Bumble',
            Address('F0:F1:F2:F3:F4:F5'),
            hci_transport.source,
            hci_transport.sink,
        )
        device.classic_enabled = True
        device.le_enabled = False
        device.listener = DiscoveryListener()

        with open(f"bumble_hci_{sys._getframe().f_code.co_name}.log", "wb") as snoop_file:
            device.host.snooper = BtSnooper(snoop_file)
            await device_power_on(device)
            await device.send_command(HCI_Write_Page_Timeout_Command(page_timeout=0xFFFF))

            # Start of Initial Condition
            await send_cmd_to_iut(shell, dut, "br pscan off")
            await send_cmd_to_iut(shell, dut, "br iscan off")
            # End of Initial Condition

            # Test Start
            logger.info('Step 1: Configure the DUT to operate in connectable mode')
            await send_cmd_to_iut(shell, dut, "br pscan on")

            logger.info('Step 2: Configure the DUT to operate in general discoverable mode')
            await send_cmd_to_iut(shell, dut, "br iscan on")

            logger.info('Step 3: Tester performs general discovery procedure')
            await device.start_discovery()
            await asyncio.sleep(10)
            await device.stop_discovery()

            logger.info('Step 4: Verify DUT is discovered in general discovery')
            assert device.listener.has_found_target_addr(
                dut_address
            ), "DUT should be discoverable in general discovery procedure"

            logger.info('Step 5: Configure the DUT to reject connection requests')
            # Make the DUT non-connectable to simulate connection rejection
            await send_cmd_to_iut(shell, dut, "br pscan off")

            logger.info('Step 6: Tester attempts to establish connection with discovered DUT')
            try:
                await device.connect(dut_address, transport=BT_BR_EDR_TRANSPORT, timeout=10)
                logger.info("Connection should be rejected")
            except Exception:
                logger.info('Expected connection failure when connection is rejected')

            logger.info('Step 7: Verify connection was rejected')
            found, _ = await _wait_for_shell_response(dut, "Connected", max_wait_sec=5)
            assert not found, "DUT should reject connection request"


class TestGAPPeripheral:
    def test_gap_s_1(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """GAP-C-1: Non-connectable Mode Testing."""
        logger.info(f'Running test_gap_s_1 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_gap_s_1(hci, shell, dut, iut_address))

    def test_gap_s_2(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """GAP-C-2: Connectable Non-discoverable Mode with Active Disconnection."""
        logger.info(f'Running test_gap_s_2 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_gap_s_2(hci, shell, dut, iut_address))

    def test_gap_s_3(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """GAP-C-3: Connectable Non-discoverable Mode with Passive Disconnection."""
        logger.info(f'Running test_gap_s_3 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_gap_s_3(hci, shell, dut, iut_address))

    def test_gap_s_4(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """GAP-C-4: Connectable Non-discoverable Mode with Connection Rejection."""
        logger.info(f'Running test_gap_s_4 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_gap_s_4(hci, shell, dut, iut_address))

    def test_gap_s_5(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """GAP-C-5: Limited Discoverable Mode with Active Disconnection."""
        logger.info(f'Running test_gap_s_5 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_gap_s_5(hci, shell, dut, iut_address))

    def test_gap_s_6(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """GAP-C-6: Limited Discoverable Mode with Passive Disconnection."""
        logger.info(f'Running test_gap_s_6 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_gap_s_6(hci, shell, dut, iut_address))

    def test_gap_s_7(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """GAP-C-7: Limited Discoverable Mode with Connection Rejection."""
        logger.info(f'Running test_gap_s_7 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_gap_s_7(hci, shell, dut, iut_address))

    def test_gap_s_8(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """GAP-C-8: General Discoverable Mode with Active Disconnection."""
        logger.info(f'Running test_gap_s_8 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_gap_s_8(hci, shell, dut, iut_address))

    def test_gap_s_9(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """GAP-C-9: General Discoverable Mode with Passive Disconnection."""
        logger.info(f'Running test_gap_s_9 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_gap_s_9(hci, shell, dut, iut_address))

    def test_gap_s_10(self, shell: Shell, dut: DeviceAdapter, device_under_test):
        """GAP-C-10: General Discoverable Mode with Connection Rejection."""
        logger.info(f'Running test_gap_s_10 {device_under_test}')
        hci, iut_address = device_under_test
        asyncio.run(tc_gap_s_10(hci, shell, dut, iut_address))

# Copyright 2021-2022 Google LLC
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import asyncio
import logging
import secrets
import sys

import bumble.logging
from bt_sim_controller import bt_sim_controller as Controller
from bt_sim_link import bt_sim_local_link
from bumble.transport import open_transport

logger = logging.getLogger(__name__)


def generate_unique_bd_address(existing_addresses: set) -> str:
    """Generate a unique Bluetooth device address.

    Args:
        existing_addresses: Set of already allocated BD addresses

    Returns:
        A unique BD address string in format XX:XX:XX:XX:XX:XX
    """
    prefix = '00:00:01'
    max_attempts = 1000

    for _ in range(max_attempts):
        suffix = ':'.join([f'{secrets.randbelow(256):02X}' for _ in range(3)])
        public_address = f'{prefix}:{suffix}'
        if public_address not in existing_addresses:
            existing_addresses.add(public_address)
            return public_address

    raise RuntimeError("Failed to generate unique BD address after maximum attempts")


def validate_bd_address(bd_address: str) -> bool:
    """Validate BD address format.

    Args:
        bd_address: BD address string to validate

    Returns:
        True if valid, False otherwise
    """
    parts = bd_address.split(':')
    if len(parts) != 6:
        return False

    for part in parts:
        if len(part) != 2:
            return False
        try:
            int(part, 16)
        except ValueError:
            return False

    return True


async def create_controller(
    index: int, transport_name: str, link, bd_addresses: set, custom_bd_address: str = None
):
    """Create and configure a single controller.

    Args:
        index: Controller index number
        transport_name: Transport connection string
        link: Shared link object for controllers
        bd_addresses: Set of allocated BD addresses

    Returns:
        Tuple of (transport, controller)
    """
    transport = await open_transport(transport_name)

    if custom_bd_address:
        if not validate_bd_address(custom_bd_address):
            raise ValueError(f"Invalid BD address format: {custom_bd_address}")
        if custom_bd_address in bd_addresses:
            raise ValueError(f"Duplicate BD address: {custom_bd_address}")
        public_address = custom_bd_address
        bd_addresses.add(public_address)
    else:
        public_address = generate_unique_bd_address(bd_addresses)

    controller = Controller(
        f'HCI{index}',
        host_source=transport.source,
        host_sink=transport.sink,
        link=link,
        public_address=public_address,
    )

    port = transport_name.split(":")[-1]
    logger.info(f"HCI{index} BD Address {public_address} port {port}")

    return transport, controller


async def async_sim_controllers():
    """Initialize and run Bluetooth simulation controllers."""
    if len(sys.argv) < 2:
        logger.error(
            "No transport provided. Usage: controllers.py <transport1@bd_address1> \
                     [transport2@bd_address2] ..."
        )
        return

    link = bt_sim_local_link()
    transports: list = []
    controllers: list = []
    bd_addresses: set = set()

    try:
        # Create all controllers
        for index, arg in enumerate(sys.argv[1:]):
            parts = arg.rsplit('@', 1)
            transport_name = parts[0]
            custom_bd_address = parts[1] if len(parts) > 1 else None

            transport, controller = await create_controller(
                index, transport_name, link, bd_addresses, custom_bd_address
            )
            transports.append(transport)
            controllers.append(controller)

        logger.info(f"Successfully initialized {len(controllers)} controller(s)")

        # Keep running until interrupted
        await asyncio.get_running_loop().create_future()

    except Exception as e:
        logger.error(f"Error during controller initialization: {e}", exc_info=True)
    finally:
        # Cleanup transports
        for transport in transports:
            try:
                transport.close()
            except Exception as e:
                logger.warning(f"Error closing transport: {e}")


def main():
    """Main entry point for the Bluetooth controller simulator."""
    bumble.logging.setup_basic_logging(default_level="DEBUG")

    try:
        asyncio.run(async_sim_controllers())
    except KeyboardInterrupt:
        logger.info("Shutting down controllers...")
    except Exception as e:
        logger.error(f"Fatal error: {e}", exc_info=True)
        sys.exit(1)


if __name__ == '__main__':
    main()

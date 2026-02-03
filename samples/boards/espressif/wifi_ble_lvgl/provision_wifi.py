#!/usr/bin/env python3
# Copyright (c) 2026 NotioNext Ltd.
# SPDX-License-Identifier: Apache-2.0

"""
WiFi BLE Provisioning Script for ESP32 with LVGL UI

This script connects to the ESP32 device via BLE and sends WiFi credentials
for provisioning. The device should be advertising as "ESP32_S3_BOX3_BLE".

Requirements:
    pip install bleak

Usage:
    python provision_wifi.py --ssid "YourWiFiNetwork" --password "YourPassword"
"""

import asyncio
import json
import argparse
import sys
from bleak import BleakClient, BleakScanner

# Service and characteristic UUIDs
WIFI_SERVICE_UUID = "12345678-1234-1234-1234-123456789abc"
WIFI_CREDS_CHAR_UUID = "12345678-1234-1234-1234-123456789abd"
WIFI_STATUS_CHAR_UUID = "12345678-1234-1234-1234-123456789abe"

# Device name to look for
DEVICE_NAME = "ESP32_S3_BOX3_BLE"

class WiFiProvisioner:
    def __init__(self):
        self.client = None
        self.device = None

    async def scan_for_device(self):
        """Scan for the ESP32 provisioning device"""
        print(f"Scanning for {DEVICE_NAME}...")

        async with asyncio.timeout(10):
            devices = await BleakScanner.discover()

            for device in devices:
                if device.name == DEVICE_NAME:
                    print(f"Found device: {device.name} ({device.address})")
                    self.device = device
                    return True

            print(f"Device {DEVICE_NAME} not found")
            return False

    async def connect(self):
        """Connect to the ESP32 device"""
        if not self.device:
            return False

        try:
            print(f"Connecting to {self.device.address}...")
            self.client = BleakClient(self.device)
            await self.client.connect()
            print("Connected successfully!")

            # Test GATT operations by reading status first
            print("Testing GATT operations...")
            try:
                status = await self.read_status()
                print(f"Initial status read successful: {status}")
            except Exception as e:
                print(f"Warning: Could not read initial status: {e}")

            return True
        except Exception as e:
            print(f"Connection failed: {e}")
            return False

    async def disconnect(self):
        """Disconnect from the device"""
        if self.client and self.client.is_connected:
            await self.client.disconnect()
            print("Disconnected")

    def _prepare_credentials_json(self, ssid, password, save_credentials):
        """Prepare credentials JSON data"""
        credentials = {
            "ssid": ssid,
            "password": password,
            "save_credentials": save_credentials
        }
        json_data = json.dumps(credentials)
        # Log credentials with masked password for security
        safe_credentials = {
            "ssid": ssid,
            "password": "***REDACTED***",
            "save_credentials": save_credentials
        }
        print(f"Sending credentials: {json.dumps(safe_credentials)}")
        print(f"Data length: {len(json_data)} bytes")
        return json_data

    def _find_wifi_service(self, services):
        """Find WiFi service in available services"""
        for service in services:
            if service.uuid.lower() == WIFI_SERVICE_UUID.lower():
                print(f"Found WiFi service: {service.uuid}")
                return service

        print("ERROR: WiFi service not found!")
        print("Available services:")
        for service in services:
            print(f"  Service: {service.uuid}")
        return None

    def _find_credentials_characteristic(self, wifi_service):
        """Find credentials characteristic in WiFi service"""
        for char in wifi_service.characteristics:
            print(f"Found characteristic: {char.uuid} (properties: {char.properties})")
            if char.uuid.lower() == WIFI_CREDS_CHAR_UUID.lower():
                print(f"Found credentials characteristic with properties: {char.properties}")
                return char

        print("ERROR: Credentials characteristic not found!")
        return None

    async def send_credentials(self, ssid, password, save_credentials=True):
        """Send WiFi credentials to the device"""
        if not self.client or not self.client.is_connected:
            print("Not connected to device")
            return False

        try:
            json_data = self._prepare_credentials_json(ssid, password, save_credentials)

            wifi_service = self._find_wifi_service(self.client.services)
            if not wifi_service:
                return False

            creds_char = self._find_credentials_characteristic(wifi_service)
            if not creds_char:
                return False

            print("Attempting to write credentials...")
            await self.client.write_gatt_char(
                WIFI_CREDS_CHAR_UUID,
                json_data.encode('utf-8'),
                response=False
            )

            print("Credentials sent successfully!")
            print("Check the device display for connection status")
            return True

        except Exception as e:
            print(f"Failed to send credentials: {e}")
            import traceback
            traceback.print_exc()
            return False

    async def read_status(self):
        """Read WiFi status from the device"""
        if not self.client or not self.client.is_connected:
            print("Not connected to device")
            return None

        try:
            status_data = await self.client.read_gatt_char(WIFI_STATUS_CHAR_UUID)
            status_json = status_data.decode('utf-8')
            status = json.loads(status_json)
            return status
        except Exception as e:
            print(f"Failed to read status: {e}")
            return None

    async def _should_retry(self, attempt, max_retries, message):
        """Check if should retry and sleep if needed"""
        if attempt >= max_retries - 1:
            return False
        else:
            print(f"Retrying {message}...")
            await asyncio.sleep(2)
            return True

    async def _scan_with_retry(self, attempt, max_retries):
        """Scan for device with retry logic"""
        if await self.scan_for_device():
            return True
        return await self._should_retry(attempt, max_retries, "scan")

    async def _connect_with_retry(self, attempt, max_retries):
        """Connect to device with retry logic"""
        if await self.connect():
            return True
        return await self._should_retry(attempt, max_retries, "connection")

    def _check_status_result(self, status):
        """Check if status indicates successful connection"""
        if not status:
            return True

        print(f"Device status: {status}")
        is_connected = status.get('status') == 'connected'

        if is_connected:
            print("✅ WiFi provisioning successful!")
        else:
            print("❌ WiFi connection failed")

        return is_connected

    async def _verify_connection_status(self):
        """Verify WiFi connection status after provisioning"""
        print("\nWaiting for connection result...")
        await asyncio.sleep(8)

        status = await self.read_status()
        return self._check_status_result(status)

    async def _handle_provisioning_attempt(self, ssid, password, save_credentials, attempt, max_retries):
        """Handle a single provisioning attempt"""
        credentials_sent = await self.send_credentials(ssid, password, save_credentials)

        if not credentials_sent:
            await self._should_retry(attempt, max_retries, "send credentials")
            return False

        connection_verified = await self._verify_connection_status()

        if not connection_verified:
            await self._should_retry(attempt, max_retries, "provisioning")

        return connection_verified

    async def provision(self, ssid, password, save_credentials=True):
        """Complete provisioning process with retry logic

        Args:
            ssid: WiFi network SSID
            password: WiFi password (never logged in clear text)
            save_credentials: Whether to save credentials on device

        Note: The password is required for provisioning but is masked in all log outputs.
        """
        max_retries = 3

        for attempt in range(max_retries):
            print(f"\n--- Provisioning Attempt {attempt + 1}/{max_retries} ---")

            if not await self._scan_with_retry(attempt, max_retries):
                continue

            if not await self._connect_with_retry(attempt, max_retries):
                continue

            try:
                if await self._handle_provisioning_attempt(ssid, password, save_credentials, attempt, max_retries):
                    return True
            finally:
                await self.disconnect()

        print(f"❌ Provisioning failed after {max_retries} attempts")
        return False

async def main():
    parser = argparse.ArgumentParser(description='WiFi BLE Provisioning for ESP32')
    parser.add_argument('--ssid', required=True, help='WiFi SSID')
    parser.add_argument('--password', required=True, help='WiFi password')
    parser.add_argument('--no-save', action='store_true',
                       help='Do not save credentials to device storage')
    parser.add_argument('--scan-only', action='store_true',
                       help='Only scan for devices, do not provision')

    args = parser.parse_args()

    provisioner = WiFiProvisioner()

    if args.scan_only:
        await provisioner.scan_for_device()
        return

    print("ESP32 WiFi BLE Provisioning Tool")
    print("=" * 40)
    print(f"SSID: {args.ssid}")
    print(f"Password: {'*' * len(args.password)}")
    print(f"Save credentials: {not args.no_save}")
    print()

    success = await provisioner.provision(
        args.ssid,
        args.password,
        save_credentials=not args.no_save
    )

    if success:
        print("\n✅ Provisioning completed!")
        print("Check the device display for the final status and IP address.")
    else:
        print("\n❌ Provisioning failed!")
        sys.exit(1)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nProvisioning cancelled by user")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

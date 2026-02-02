#!/usr/bin/env python3
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
        
    async def scan_for_device(self, timeout=10):
        """Scan for the ESP32 provisioning device"""
        print(f"Scanning for {DEVICE_NAME}...")
        
        devices = await BleakScanner.discover(timeout=timeout)
        
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
    
    async def send_credentials(self, ssid, password, save_credentials=True):
        """Send WiFi credentials to the device"""
        if not self.client or not self.client.is_connected:
            print("Not connected to device")
            return False
            
        try:
            # Prepare credentials JSON
            credentials = {
                "ssid": ssid,
                "password": password,
                "save_credentials": save_credentials
            }
            
            json_data = json.dumps(credentials)
            print(f"Sending credentials: {json_data}")
            print(f"Data length: {len(json_data)} bytes")
            
            # Check if the characteristic exists
            services = self.client.services
            wifi_service = None
            creds_char = None
            
            for service in services:
                if service.uuid.lower() == WIFI_SERVICE_UUID.lower():
                    wifi_service = service
                    print(f"Found WiFi service: {service.uuid}")
                    break
            
            if not wifi_service:
                print("ERROR: WiFi service not found!")
                # List all available services for debugging
                print("Available services:")
                for service in services:
                    print(f"  Service: {service.uuid}")
                return False
            
            for char in wifi_service.characteristics:
                print(f"Found characteristic: {char.uuid} (properties: {char.properties})")
                if char.uuid.lower() == WIFI_CREDS_CHAR_UUID.lower():
                    creds_char = char
                    print(f"Found credentials characteristic with properties: {char.properties}")
                    break
            
            if not creds_char:
                print("ERROR: Credentials characteristic not found!")
                return False
            
            # Send credentials to the device using write without response
            print("Attempting to write credentials...")
            await self.client.write_gatt_char(
                WIFI_CREDS_CHAR_UUID, 
                json_data.encode('utf-8'),
                response=False  # Use write without response
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
    
    async def provision(self, ssid, password, save_credentials=True):
        """Complete provisioning process with retry logic"""
        max_retries = 3
        
        for attempt in range(max_retries):
            print(f"\n--- Provisioning Attempt {attempt + 1}/{max_retries} ---")
            
            # Scan for device
            if not await self.scan_for_device():
                if attempt < max_retries - 1:
                    print("Retrying scan...")
                    await asyncio.sleep(2)
                    continue
                return False
            
            # Connect to device
            if not await self.connect():
                if attempt < max_retries - 1:
                    print("Retrying connection...")
                    await asyncio.sleep(2)
                    continue
                return False
            
            try:
                # Send credentials
                if await self.send_credentials(ssid, password, save_credentials):
                    print("\nWaiting for connection result...")
                    
                    # Wait a bit for connection attempt
                    await asyncio.sleep(8)  # Increased wait time
                    
                    # Read status
                    status = await self.read_status()
                    if status:
                        print(f"Device status: {status}")
                        if status.get('status') == 'connected':
                            print("✅ WiFi provisioning successful!")
                            return True
                        else:
                            print("❌ WiFi connection failed")
                            if attempt < max_retries - 1:
                                print("Retrying provisioning...")
                                await self.disconnect()
                                await asyncio.sleep(3)
                                continue
                    
                    return True
                else:
                    if attempt < max_retries - 1:
                        print("Failed to send credentials, retrying...")
                        await self.disconnect()
                        await asyncio.sleep(3)
                        continue
                    return False
                    
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

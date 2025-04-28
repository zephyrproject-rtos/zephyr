#!/usr/bin/env python3

import usb.core
import usb.util
import argparse
import time
import sys
from typing import Optional, List

class PrinterTester:
    # USB Printer Class specific constants
    PRINTER_INTERFACE = 0
    OUT_EP = 0x01
    IN_EP = 0x82
    
    def __init__(self):
        self.device = None
        self.interface = None
        self.timeout = 1000  # ms

    def find_printer(self) -> bool:
        """Find and initialize USB printer device."""
        # Find printer device (adjust VID/PID as needed)
        self.device = usb.core.find(idVendor=0x2FE3, idProduct=0x0100)
        
        if self.device is None:
            print("Error: Printer device not found")
            return False
            
        # Set configuration and claim interface
        self.device.set_configuration()
        self.interface = self.device[0][(0,0)]
        
        return True

    def send_data(self, data: bytes) -> int:
        """Send data to printer, return bytes sent."""
        return self.device.write(self.OUT_EP, data, self.timeout)

    def read_status(self) -> bytes:
        """Read printer status."""
        return self.device.read(self.IN_EP, 64, self.timeout)

    def test_basic_functionality(self) -> bool:
        """Basic printer functionality tests."""
        print("\nRunning basic functionality tests...")
        
        try:
            # Test 1: Simple text printing
            text = b"Hello, Printer Test!\r\n"
            sent = self.send_data(text)
            if sent != len(text):
                print(f"Error: Sent {sent} bytes, expected {len(text)}")
                return False
                
            # Test 2: Status check
            status = self.read_status()
            if not status or len(status) == 0:
                print("Error: Could not read printer status")
                return False
                
            print("Basic functionality tests passed")
            return True
            
        except Exception as e:
            print(f"Error in basic tests: {str(e)}")
            return False

    def test_pcl_commands(self) -> bool:
        """Test PCL command processing."""
        print("\nRunning PCL command tests...")
        
        try:
            # Test font selection
            pcl_cmd = b"\x1B(s10H" # Select 10-point Helvetica
            sent = self.send_data(pcl_cmd)
            if sent != len(pcl_cmd):
                return False
                
            # Test orientation
            pcl_cmd = b"\x1B&l1O" # Landscape orientation
            sent = self.send_data(pcl_cmd)
            if sent != len(pcl_cmd):
                return False
                
            print("PCL command tests passed")
            return True
            
        except Exception as e:
            print(f"Error in PCL tests: {str(e)}")
            return False

    def test_pjl_commands(self) -> bool:
        """Test PJL command processing."""
        print("\nRunning PJL command tests...")
        
        try:
            # Start job
            pjl_cmd = b"@PJL JOB NAME=\"Test Job\"\r\n"
            sent = self.send_data(pjl_cmd)
            if sent != len(pjl_cmd):
                return False
                
            # Get status
            pjl_cmd = b"@PJL INFO STATUS\r\n"
            sent = self.send_data(pjl_cmd)
            if sent != len(pjl_cmd):
                return False
                
            # End job
            pjl_cmd = b"@PJL EOJ\r\n"
            sent = self.send_data(pjl_cmd)
            if sent != len(pjl_cmd):
                return False
                
            print("PJL command tests passed")
            return True
            
        except Exception as e:
            print(f"Error in PJL tests: {str(e)}")
            return False

    def test_error_handling(self) -> bool:
        """Test error detection and handling."""
        print("\nRunning error handling tests...")
        
        try:
            # Test invalid PCL command
            invalid_cmd = b"\x1BINVALID"
            self.send_data(invalid_cmd)
            status = self.read_status()
            
            # Should indicate error in status
            if status[0] & 0x20 == 0:  # Check error bit
                print("Error: Device failed to detect invalid command")
                return False
                
            # Test buffer overflow
            large_data = b"X" * 16384  # Larger than buffer
            try:
                self.send_data(large_data)
                status = self.read_status()
                if status[0] & 0x20 == 0:
                    print("Error: No overflow detection")
                    return False
            except usb.core.USBError:
                # Expected behavior
                pass
                
            print("Error handling tests passed")
            return True
            
        except Exception as e:
            print(f"Error in error handling tests: {str(e)}")
            return False

def main():
    parser = argparse.ArgumentParser(description="USB Printer Test Suite")
    parser.add_argument("--all", action="store_true", help="Run all tests")
    parser.add_argument("--basic", action="store_true", help="Run basic functionality tests")
    parser.add_argument("--pcl", action="store_true", help="Run PCL command tests")
    parser.add_argument("--pjl", action="store_true", help="Run PJL command tests")
    parser.add_argument("--error", action="store_true", help="Run error handling tests")
    
    args = parser.parse_args()
    
    tester = PrinterTester()
    if not tester.find_printer():
        sys.exit(1)
        
    success = True
    
    if args.all or args.basic:
        success &= tester.test_basic_functionality()
        
    if args.all or args.pcl:
        success &= tester.test_pcl_commands()
        
    if args.all or args.pjl:
        success &= tester.test_pjl_commands()
        
    if args.all or args.error:
        success &= tester.test_error_handling()
        
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
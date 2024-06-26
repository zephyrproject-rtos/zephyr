# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing with svl'''

import subprocess
import os
import glob
from runners.core import ZephyrBinaryRunner, RunnerCaps

class PortNotFoundError(Exception):
    """Exception raised when no serial ports are found."""
    pass

class ArtemisBinaryRunner(ZephyrBinaryRunner):
    '''Runner for Artemis Nano.'''
    def __init__(self, cfg, serial_port, baud_rate):
        super().__init__(cfg)
        
        self.bin_file = cfg.bin_file
        self.serial_port = serial_port
        self.baud_rate = baud_rate

        self.svl_script = self._find_in_path("artemis_svl.py")
        if self.svl_script is None:
            raise FileNotFoundError("Script artemis_svl.py not found in PATH")
        

    @classmethod
    def do_create(cls, cfg, args):
        return ArtemisBinaryRunner(cfg=cfg,
                                   serial_port=args.serial_port,
                                   baud_rate=args.baud_rate)
    
    @classmethod
    def name(cls):
        return "sp_artemis"
    
    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash'})

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument(
            "--serial-port", required=True, help="Serial port for flashing"
        )
        parser.add_argument(
            "--baud-rate",
            type=int,
            default=115200,
            help="Specify the communication speed in baud (default is 115200)",
        )

    def do_run(self, command, **kwargs):
        if command == "flash":
            self.do_flash(**kwargs)

    def do_flash(self, **kwargs):
        print("Flashing ...")
        print(f"serial port is {self.serial_port}")
        print(f"baud rate is {self.baud_rate}")
        
        if not self._does_serial_port_exist():
            print(f"{self.serial_port} not found!!!")
            return

        if self.bin_file is not None:
            # Check if bin file actually exists
            if not os.path.isfile(self.bin_file):
                err = "Cannot flash; file ({}) not found"
                raise ValueError(err.format(self.bin_file))

            # Pad the binary file to make its size divisible by 4
            self._pad_binary_file()

            # Check if artemis_svl.py exists
            if not os.path.isfile(self.svl_script):
                raise FileNotFoundError(f"Script {self.svl_script} not found")

            cmd = [
                "python3",
                self.svl_script,
                "-f",
                self.bin_file,
                "-b",
                str(self.baud_rate),
                self.serial_port,
            ]

        else:
            err = "Cannot flash; no bin ({}) files found."
            raise ValueError(err.format(self.bin_name))

        try:
            # Run the command
            result = subprocess.run(cmd, check=True, capture_output=True, text=True)
            print(result.stdout)
            print("Flashing successful!")
        except subprocess.CalledProcessError as e:
            print(f"Flashing failed with error: {e.stderr}")

    def _pad_binary_file(self):
        """Padding the binary file"""
        with open(self.bin_file, "rb") as f:
            bin_data = f.read()

        # Pad the binary file to make its size divisible by 4
        padding = (4 - (len(bin_data) % 4)) % 4
        if padding != 0:
            bin_data += b"\x00" * padding
            with open(self.bin_file, "wb") as f:
                f.write(bin_data)
    
    def _find_in_path(self, filename):
        """Search for a file in all directories listed in PATH."""
        for path in os.environ["PATH"].split(os.pathsep):
            full_path = os.path.join(path, filename)
            if os.path.isfile(full_path) and os.access(full_path, os.X_OK):
                return full_path
        return None
    
    def _does_serial_port_exist(self):
        possible_ports = glob.glob('/dev/tty[A-Za-z]*')
        
        # Filter out the actual serial ports
        serial_ports = [port for port in possible_ports if 'ttyS' in port or 'ttyUSB' in port or 'ttyACM' in port]
        
        if serial_ports:
            return self.serial_port in serial_ports
        else:
            print("No serial ports found in /dev/ ")
            return False

#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#
# Author: Sathish Kuttan <sathish.k.kuttan@intel.com>

# This file defines device class that contains functions to
# setup/cconfigure SPI master device and GPIO pins required
# to communicate with the target.
# Member functions are provided to send and receive messages
# over the SPI bus

import yaml
import time
import periphery
import sys
import os

class Device:
    """
    Device class containing the interface methods for communicating
    with the target using SPI bus and GPIOs
    """
    def __init__(self):
        """
        Read config file and determine the SPI device, speed, mode
        GPIO pin assignments, etc.
        Initialize data structures accordingly.
        """
        config_file = os.path.dirname(__file__)
        config_file += '/config.yml'
        with open(config_file, 'r') as ymlfile:
            config = yaml.safe_load(ymlfile)
        self.name = config['general']['name']
        self.spi_device = config['spi']['device']
        self.spi_speed = config['spi']['max_speed']
        self.spi_mode = None
        self.gpio_reset = config['gpio']['reset']
        self.gpio_wake = config['gpio']['wake']
        self.gpio_irq = config['gpio']['irq']
        self.spi = None
        self.reset_pin = None
        self.wake_pin = None
        self.irq_pin = None

    def print_config(self):
        """
        Print configuration information that was read from config file
        """
        print('%s Device on %s' %(self.name, self.spi_device))
        print('Max SPI Frequency: %2.1f MHz' % self.spi_speed)
        print('Reset GPIO: %d' % self.gpio_reset)
        print('Wake GPIO : %d' % self.gpio_wake)
        print('IRQ GPIO  : %d' % self.gpio_irq)

    def configure_device(self, spi_mode, order, bits):
        """
        Configure the SPI device and GPIOs for communication with target
        """
        self.reset_pin = periphery.GPIO(self.gpio_reset, 'out')
        self.wake_pin = periphery.GPIO(self.gpio_wake, 'out')
        self.irq_pin = periphery.GPIO(self.gpio_irq, 'in')
        self.spi = periphery.SPI(self.spi_device, spi_mode,
                self.spi_speed * 1e6)
        self.spi.bit_order = order
        self.spi.bits_per_word = bits
        print('Configured SPI %s for %s.' % (self.spi_device, self.name))
        print('Configured GPIO %d for %s Reset.' % (self.gpio_reset, self.name))
        print('Configured GPIO %d for %s Wake.' % (self.gpio_wake, self.name))
        print('Configured GPIO %d for %s IRQ.' % (self.gpio_irq, self.name))

    def check_device_ready(self):
        """
        Check whether the target is ready to accept a command as follows:
        Wait a minimum time and check whether IRQ is asserted by the target
        If IRQ is not asserted, wait maximum time and check again.
        Device is ready if IRQ is asserted
        """
        min_wait = 0.001 #   1 ms
        max_wait = 0.1 	 # 100 ms
        time.sleep(min_wait)
        ready = self.irq_pin.read()
        if not ready:
            time.sleep(max_wait)
            ready = self.irq_pin.read()
            if not ready:
                print('Error: Device not ready', file=sys.stderr)
        return ready

    def reset_device(self):
        """
        Assert the GPIO and deassert after a short duration to reset
        the target.
        """
        print('Resetting %s ...' % self.name)
        self.reset_pin.write(False)
        time.sleep(0.1)
        self.reset_pin.write(True)
        self.check_device_ready()

    def send_receive(self, data, wait=True):
        """
        Transmit and receive full duplex data over SPI
        If requested to wait, wait for device to become ready
        before return.
        """
        rx_data = self.spi.transfer(data)
        if wait:
            self.check_device_ready()
        return rx_data

    def send_bulk(self, data):
        """
        Send a byte stream of data without checking for device readiness.
        """
        self.spi.transfer(data)

    def close(self):
        """
        Close the device handle
        """
        self.spi.close()

#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
"""
Script to capture tracing data with USB backend.
"""

import usb.core
import usb.util
import argparse
import sys

def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-v", "--vendor_id", required=True,
                        help="usb device vendor id")
    parser.add_argument("-p", "--product_id", required=True,
                        help="usb device product id")
    parser.add_argument("-o", "--output", default='channel0_0',
                        required=False, help="tracing data output file")
    args = parser.parse_args()

def main():
    parse_args()
    if args.vendor_id.isdecimal():
        vendor_id = int(args.vendor_id)
    else:
        vendor_id = int(args.vendor_id, 16)

    if args.product_id.isdecimal():
        product_id = int(args.product_id)
    else:
        product_id = int(args.product_id, 16)

    output_file = args.output

    try:
        usb_device = usb.core.find(idVendor=vendor_id, idProduct=product_id)
    except Exception as e:
        sys.exit("{}".format(e))

    if usb_device is None:
        sys.exit("No device found, check vendor_id and product_id")

    if usb_device.is_kernel_driver_active(0):
        try:
            usb_device.detach_kernel_driver(0)
        except usb.core.USBError as e:
            sys.exit("{}".format(e))

    # set the active configuration. With no arguments, the first
    # configuration will be the active one
    try:
        usb_device.set_configuration()
    except usb.core.USBError as e:
        sys.exit("{}".format(e))

    configuration = usb_device[0]
    interface = configuration[(0, 0)]

    # match the only IN endpoint
    read_endpoint = usb.util.find_descriptor(interface, custom_match = \
                                                lambda e: \
                                                usb.util.endpoint_direction( \
                                                    e.bEndpointAddress) == \
                                                usb.util.ENDPOINT_IN)

    # match the only OUT endpoint
    write_endpoint = usb.util.find_descriptor(interface, custom_match = \
                                                lambda e: \
                                                usb.util.endpoint_direction( \
                                                    e.bEndpointAddress) == \
                                                usb.util.ENDPOINT_OUT)

    usb.util.claim_interface(usb_device, interface)

    #enable device tracing
    write_endpoint.write('enable')

    #try to read to avoid garbage mixed to useful stream data
    buff = usb.util.create_buffer(8192)
    read_endpoint.read(buff, 10000)

    with open(output_file, "wb") as file_desc:
        while True:
            buff = usb.util.create_buffer(8192)
            length = read_endpoint.read(buff, 100000)
            for index in range(length):
                file_desc.write(chr(buff[index]).encode('latin1'))

    usb.util.release_interface(usb_device, interface)

if __name__=="__main__":
    try:
        main()
    except KeyboardInterrupt:
        print('Data capture interrupted, data saved into {}'.format(args.output))
        sys.exit(0)

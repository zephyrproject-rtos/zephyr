#!/usr/bin/env python3
# Copyright (c) 2023, Meta
#
# SPDX-License-Identifier: Apache-2.0

"""Thrift Hello Client Sample

Connect to a hello service and demonstrate the
ping(), echo(), and counter() Thrift RPC methods.

Usage:
    ./hello_client.py [ip]
"""

import argparse
import sys

sys.path.append('gen-py')

from hello import Hello
from thrift.protocol import TBinaryProtocol
from thrift.transport import TSocket, TTransport


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument('--ip', default='192.0.2.1',
                        help='IP address of hello server')

    return parser.parse_args()


def main():
    args = parse_args()

    transport = TSocket.TSocket(args.ip, 4242)
    transport = TTransport.TBufferedTransport(transport)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = Hello.Client(protocol)

    transport.open()

    client.ping()
    client.echo('Hello, world!')

    # necessary to mitigate unused variable warning with for i in range(5)
    i = 0
    while i < 5:
        client.counter()
        i = i + 1

    transport.close()


if __name__ == '__main__':
    main()

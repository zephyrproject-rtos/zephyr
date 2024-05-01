#!/usr/bin/env python3
#
# Copyright (c) 2024 Plentify (Pty) Ltd.
#
# SPDX-License-Identifier: Apache-2.0


class Q31(object):
    def __init__(self, value, shift) -> None:
        self.value = int(value)
        self.shift = int(shift)

    def __repr__(self):
        return f"Q31(value={self.value}, shift={self.shift})"

    @staticmethod
    def encode(n: float) -> "Q31":
        """
        >>> Q31.encode(0.98)
        Q31(value=2104533974, shift=0)
        >>> Q31.encode(8.98)
        Q31(value=1205275196, shift=4)
        >>> Q31.encode(-538.1928462)
        Q31(value=-1128672203, shift=10)
        """
        temp = n
        shift = 0
        while not -1.0 <= temp < 1.0:
            shift += 1
            temp = n / float(1 << shift)
        return Q31(temp * 0x7FFFFFFF, shift)

    def decode(self) -> float:
        return (self.value / 0x7FFFFFFF) * float(1 << self.shift)

    def isclose(self, f: float) -> bool:
        """Compare the floats.

        >>> q = Q31.encode(0.98)
        >>> q.isclose(0.98)
        True
        >>> q.isclose(0.9812345)
        False
        >>> Q31.encode(65432.123456).isclose(65432.123456)
        True
        """
        import math

        fraction_size = len(str(f).split(".")[1])

        return math.isclose(
            self.decode(), f, rel_tol=(1 / 10**fraction_size), abs_tol=0.0
        )


def test():
    import doctest

    doctest.testmod()


def arg_parse_handler():
    import argparse

    parent_parser = argparse.ArgumentParser(allow_abbrev=False, add_help=False)
    parent_parser.add_argument(
        "--debug",
        default=False,
        required=False,
        action="store_true",
        dest="debug",
        help="debug flag",
    )
    main_parser = argparse.ArgumentParser(allow_abbrev=False)
    service_subparsers = main_parser.add_subparsers(
        title="sub_command", dest="sub_command"
    )
    service_subparsers.add_parser("test", help="test")
    decode_parser = service_subparsers.add_parser(
        "decode", help="Decode q1.31 to float"
    )
    decode_parser.add_argument("q1_31", help="the q1.31 value", type=int)
    decode_parser.add_argument(
        "shift", help="the shift calculated on the encode phase", type=int
    )

    encode_parser = service_subparsers.add_parser(
        "encode",
        help="Encode float to q1.31 with scale factor (shift)",
        parents=[parent_parser],
    )
    encode_parser.add_argument(
        "value", help="the float value to be encoded", type=float
    )
    return main_parser.parse_args()


if __name__ == "__main__":
    args = arg_parse_handler()

    if args.sub_command == "test":
        print("Executing unittests")
        test()

    if args.sub_command == "encode":
        print(Q31.encode(args.value))

    if args.sub_command == "decode":
        print(Q31(args.q1_31, args.shift).decode())

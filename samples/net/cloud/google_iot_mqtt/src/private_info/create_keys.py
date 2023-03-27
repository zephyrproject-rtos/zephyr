#!/usr/bin/env python3
#
# Copyright (c) 2018 Linaro
#
# SPDX-License-Identifier: Apache-2.0

import sys
import subprocess
import argparse

def parse_args():
    global args

    parser = argparse.ArgumentParser(description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)

    parser.add_argument(
        "-d", "--device", required=True,
        help="Name of device")

    parser.add_argument(
        "-e", "--ecdsa", required=False,
        action='store_true',
        help="Use elliptic curve")

    parser.add_argument(
        "-r", "--rsa", required=False,
        action='store_true',
        help="Use RSA")

    args = parser.parse_args()

split_string = lambda x, n: [x[i:i+n] for i in range(0, len(x), n)]

def main():
    parse_args()

    fd = open("key.c", "w")

    pem_file = args.device + "-private.pem"
    cert_file = args.device + "-cert.pem"
    der_file = args.device + "-private.der"

    if args.ecdsa:
        print("Generating ecdsa private key")
        try:
            subprocess.call(["openssl", "ecparam", "-noout", "-name", "prime256v1", "-genkey",
                "-out", pem_file])

        except subprocess.CalledProcessError as e:
            print(e.output)
    else:
        print("Generating rsa private key")
        try:
            subprocess.call(["openssl", "genrsa", "-out", pem_file, "2048"])

        except subprocess.CalledProcessError as e:
            print(e.output)

    print("Generating certificate")

    try:
        subprocess.check_call(["openssl", "req", "-new", "-x509", "-key", pem_file, "-out",
            cert_file, "-days", "1000000", "-subj", "/CN=" + args.device])

    except subprocess.CalledProcessError as e:
        print(e.output)


    if args.ecdsa:
        print("Parsing ECDSA private key")
        o = subprocess.Popen(["openssl", "asn1parse", "-in", pem_file,
            "-offset", "5", "-length", "34"], stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT)

        if o.returncode == 0:
            sys.exit("failed to parse ECDSA private key")

        tmp = o.communicate()[0].decode('ascii').split(":")[3].lower().strip()

        if o.returncode:
            sys.exit("failed to parse ECDSA private key")

        tmp = split_string(tmp, 2)
        output = ["0x" + s for s in tmp]
        output = ', '.join(output)

    else:
        print("Parsing RSA private key and generating DER output")
        try:
            subprocess.call(["openssl", "pkcs8", "-nocrypt", "-topk8",
                "-inform", "pem", "-outform", "der", "-in", pem_file, "-out",
                der_file])

        except subprocess.CalledProcessError as e:
            print(e.output)

        der_fd = open(der_file, "r")
        o = subprocess.Popen(["xxd", "-i"], stdin=der_fd, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT)

        output = o.communicate()[0].decode('ascii')

        if o.returncode:
            sys.exit("failed to parse RSA private key")

    print("Generating key.c")
    fd.write("unsigned char zepfull_private_der[] = {\n")
    key_len = len(output.split(','))
    fd.write(output)
    fd.write("};\n\n")

    fd.write("unsigned int zepfull_private_der_len = " + str(key_len) + ";\n")

if __name__ == "__main__":
    main()

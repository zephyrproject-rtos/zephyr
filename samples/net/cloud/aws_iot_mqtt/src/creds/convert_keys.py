# Copyright (c) 2023 Lucas Dietrich <ld.adecy@gmail.com>
# SPDX-License-Identifier: Apache-2.0

import os
import glob

def bin2array(name, fin, fout):
    with open(fin, 'rb') as f:
        data = f.read()

    data += b'\0'  # add null terminator

    with open(fout, 'w') as f:
        f.write("#include <stdint.h>\n")
        f.write(f"const uint8_t {name}[] = {{")
        for i in range(0, len(data), 16):
            f.write("\n\t")
            f.write(", ".join(f"0x{b:02x}" for b in data[i:i+16]))
            f.write(",")
        f.write("\n};\n")
        f.write(f"const uint32_t {name}_len = sizeof({name});\n")

    print(
        f"[{name.center(13, ' ')}]: {os.path.relpath(fin)} -> {os.path.relpath(fout)}")


if __name__ == "__main__":
    creds_dir = os.path.dirname(os.path.realpath(__file__))

    creds = glob.glob(f"{creds_dir}/*.pem.*")

    cert_found, key_found = False, False

    for cred in creds:
        if cred.endswith('-certificate.pem.crt'):
            bin2array("public_cert", cred, os.path.join(creds_dir, "cert.c"))
            cert_found = True
        elif cred.endswith('-private.pem.key'):
            bin2array("private_key", cred, os.path.join(creds_dir, "key.c"))
            key_found = True

    if not cert_found:
        print("No certificate found !")
    if not key_found:
        print("No private key found !")

    bin2array("ca_cert", os.path.join(creds_dir, "AmazonRootCA1.pem"),
              os.path.join(creds_dir, "ca.c"))

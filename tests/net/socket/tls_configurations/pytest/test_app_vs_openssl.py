# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import subprocess
from twister_harness import DeviceAdapter

logger = logging.getLogger(__name__)

def get_arguments_from_server_type(server_type, port):
    this_path = os.path.dirname(os.path.abspath(__file__))
    certs_path = os.path.join(this_path, "..", "credentials")

    args = ["openssl", "s_server"]
    if server_type == "1.2-rsa":
        args.extend(["-cert", "{}/rsa.crt".format(certs_path),
                     "-key", "{}/rsa-priv.key".format(certs_path),
                     "-certform", "PEM",
                     "-tls1_2",
                     "-cipher", "AES128-SHA256,AES256-SHA256"])
    elif server_type == "1.2-ec":
        args.extend(["-cert", "{}/ec.crt".format(certs_path),
                     "-key", "{}/ec-priv.key".format(certs_path),
                     "-certform", "PEM",
                     "-tls1_2",
                     "-cipher", "ECDHE-ECDSA-AES128-SHA256"])
    elif server_type == "1.3-ephemeral":
        args.extend(["-cert", "{}/ec.crt".format(certs_path),
                     "-key", "{}/ec-priv.key".format(certs_path),
                     "-certform", "PEM",
                     "-tls1_3",
                     "-ciphersuites", "TLS_AES_128_GCM_SHA256",
                     "-num_tickets", "0"])
    elif server_type == "1.3-ephemeral-tickets":
        args.extend(["-cert", "{}/ec.crt".format(certs_path),
                     "-key", "{}/ec-priv.key".format(certs_path),
                     "-certform", "PEM",
                     "-tls1_3",
                     "-ciphersuites", "TLS_AES_128_GCM_SHA256"])
    elif server_type == "1.3-psk-tickets":
        args.extend(["-tls1_3",
                     "-ciphersuites", "TLS_AES_128_GCM_SHA256",
                     "-psk_identity", "PSK_identity", "-psk", "0102030405",
                     "-allow_no_dhe_kex", "-nocert"])
    else:
        raise Exception("Wrong server type")

    args.extend(["-serverpref", "-state", "-debug", "-status_verbose", "-rev",
                 "-accept", "{}".format(port)])
    return args

def start_server(server_type, port):
    logger.info("Server type: " + server_type)
    args = get_arguments_from_server_type(server_type, port)
    logger.info("Launch command:")
    print(" ".join(args))
    openssl = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    try:
        openssl.wait(1)
        logger.error("Server startup failed. Here's the logs from OpenSSL:")
        for line in openssl.stdout.readlines():
            logger.error(line)
        raise Exception("Server startup failed")
    except subprocess.TimeoutExpired:
        logger.info("Server is up")

    return openssl

def test_app_vs_openssl(dut: DeviceAdapter, server_type, port):
    server = start_server(server_type, port)

    logger.info("Launch Zephyr application")
    dut.launch()
    dut.readlines_until("Test PASSED", timeout=3.0)

    logger.info("Kill server")
    server.kill()

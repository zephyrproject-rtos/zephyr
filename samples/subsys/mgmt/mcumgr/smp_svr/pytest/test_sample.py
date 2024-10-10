# Copyright (c) 2024 Marcin Niestroj
#
# SPDX-License-Identifier: Apache-2.0

import logging
from typing import Final

import pytest

from smpclient import SMPClient
from smpclient.generics import error, success
from smpclient.requests.os_management import EchoWrite
from smpclient.requests.image_management import ImageStatesRead, ImageUploadWrite
from smpclient.transport.udp import SMPUDPTransport

from twister_harness import DeviceAdapter


logger = logging.getLogger()


pytestmark = pytest.mark.anyio


@pytest.fixture(scope='session')
async def client(dut: DeviceAdapter) -> SMPClient:
    async with SMPClient(SMPUDPTransport(), '127.0.0.1') as client:
        logger.info('Client created')
        yield client


async def test_echo(client: SMPClient):
    response: Final = await client.request(EchoWrite(d='Hello world!'))

    if success(response):
        logger.info('Received response: %s', response)
    elif error(response):
        logger.error('Received error: %s', response)
    else:
        raise RuntimeError('Unknown response {response}')


async def test_images_state(client: SMPClient):
    response: Final = await client.request(ImageStatesRead())

    if success(response):
        logger.info('Received response: %s', response)
    elif error(response):
        logger.error('Received error: %s', response)
    else:
        raise RuntimeError('Unknown response {response}')


async def test_upload(client: SMPClient):
    response: Final = await client.request(ImageUploadWrite(off=0, data=b'123', len=3))

    if success(response):
        logger.info('Received response: %s', response)
    elif error(response):
        logger.error('Received error: %s', response)
    else:
        raise RuntimeError('Unknown response {response}')

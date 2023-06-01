# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import json
import requests
import binascii

class Leshan:
    def __init__(self, url: str):
        self.api_url = url
        self.timeout = 10
        self.format = 'TLV'
        # self.format = "SENML_CBOR"
        try:
            resp = self.get('/security/clients')
            if not isinstance(resp, list):
                raise RuntimeError('Did not receive list of endpoints')
        except requests.exceptions.ConnectionError:
            raise RuntimeError('Leshan not responding')

    @staticmethod
    def handle_response(resp: requests.models.Response):
        """Generic response handler for all queries"""
        if resp.status_code >= 300 or resp.status_code < 200:
            raise RuntimeError(f'Error {resp.status_code}: {resp.text}')
        if len(resp.text):
            obj = json.loads(resp.text)
            return obj
        else:
            return None

    def get(self, path: str):
        """Send HTTP GET query"""
        resp = requests.get(f'{self.api_url}{path}?timeout={self.timeout}&format={self.format}')
        return Leshan.handle_response(resp)

    def put_raw(self, path: str, data: str | dict | None = None, headers: dict | None = None):
        resp = requests.put(f'{self.api_url}{path}', data=data, headers=headers)
        return Leshan.handle_response(resp)

    def put(self, path: str, data: str | dict, uri_options: str = ''):
        if isinstance(data, dict):
            data = json.dumps(data)
        return self.put_raw(f'{path}?timeout={self.timeout}&format={self.format}' + uri_options, data=data, headers={'content-type': 'application/json'})

    def post(self, path: str, data: str | dict | None = None):
        resp = requests.post(f'{self.api_url}{path}', data=data, headers={'content-type': 'application/json'})
        return Leshan.handle_response(resp)

    def delete(self, path: str):
        resp = requests.delete(f'{self.api_url}{path}')
        return Leshan.handle_response(resp)

    def execute(self, endpoint: str, path: str):
        return self.post(f'/clients/{endpoint}/{path}')

    def write(self, endpoint: str, path: str, value: bool | int | str):
        if isinstance(value, bool):
            type = 'boolean'
            value = "true" if value else "false"
        elif isinstance(value, int):
            type = 'integer'
            value = str(value)
        elif isinstance(value, str):
            type = 'string'
            value = '"' + value + '"'
        id = path.split('/')[2]
        return self.put(f'/clients/{endpoint}/{path}', f'{{"id":{id},"kind":"singleResource","value":{value},"type":"{type}"}}')

    def read(self, endpoint: str, path: str):
        resp = self.get(f'/clients/{endpoint}/{path}')
        if not resp['success']:
            return resp
        content = resp['content']
        if content['kind'] == 'instance':
            return content['resources']
        elif content['kind'] == 'singleResource':
            return content['value']
        elif content['kind'] == 'multiResource':
            return content['values']
        raise RuntimeError(f'Unhandled type {content["kind"]}')

    def create_psk_device(self, endpoint: str, passwd: str):
        psk = binascii.b2a_hex(passwd.encode()).decode()
        self.put('/security/clients/', f'{{"endpoint":"{endpoint}","tls":{{"mode":"psk","details":{{"identity":"{endpoint}","key":"{psk}"}} }} }}')

    def delete_device(self, endpoint: str):
        self.delete(f'/security/clients/{endpoint}')

    def create_bs_device(self, endpoint: str, server_uri: str, passwd: str):
        psk = binascii.b2a_hex(passwd.encode()).decode()
        data = f'{{"tls":{{"mode":"psk","details":{{"identity":"{endpoint}","key":"{psk}"}}}},"endpoint":"{endpoint}"}}'
        self.put('/security/clients/', data)
        id = str([ord(n) for n in endpoint])
        key = str([ord(n) for n in passwd])
        content = '{"servers":{"0":{"binding":"U","defaultMinPeriod":1,"lifetime":86400,"notifIfDisabled":false,"shortId":1}},"security":{"1":{"bootstrapServer":false,"clientOldOffTime":1,"publicKeyOrId":' + id + ',"secretKey":' + key + ',"securityMode":"PSK","serverId":1,"serverSmsNumber":"","smsBindingKeyParam":[],"smsBindingKeySecret":[],"smsSecurityMode":"NO_SEC","uri":"'+server_uri+'"}},"oscore":{},"toDelete":["/0","/1"]}'
        self.post(f'/bootstrap/{endpoint}', content)

    def delete_bs_device(self, endpoint: str):
        self.delete(f'/security/clients/{endpoint}')
        self.delete(f'/bootstrap/{endpoint}')

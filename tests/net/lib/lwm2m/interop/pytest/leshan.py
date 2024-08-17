"""
REST client for Leshan demo server
##################################

Copyright (c) 2023 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

"""

from __future__ import annotations

import json
import binascii
import time
from datetime import datetime
from contextlib import contextmanager
import requests

class Leshan:
    """This class represents a Leshan client that interacts with demo server's REAT API"""
    def __init__(self, url: str):
        """Initialize Leshan client and check if server is available"""
        self.api_url = url
        self.timeout = 10
        #self.format = 'TLV'
        self.format = "SENML_CBOR"
        self._s = requests.Session()
        try:
            resp = self.get('/security/clients')
            if not isinstance(resp, list):
                raise RuntimeError('Did not receive list of endpoints')
        except requests.exceptions.ConnectionError as exc:
            raise RuntimeError('Leshan not responding') from exc

    @staticmethod
    def handle_response(resp: requests.models.Response):
        """
        Handle the response received from the server.

        Parameters:
        - response: The response object received from the server.

        Returns:
        - dict: The parsed JSON response as a dictionary.

        Raises:
        - Exception: If the response indicates an error condition.
        """
        if resp.status_code >= 300 or resp.status_code < 200:
            raise RuntimeError(f'Error {resp.status_code}: {resp.text}')
        if len(resp.text):
            obj = json.loads(resp.text)
            return obj
        return None

    def get(self, path: str):
        """Send HTTP GET query with typical parameters"""
        params = {'timeout': self.timeout}
        if self.format is not None:
            params['format'] = self.format
        resp = self._s.get(f'{self.api_url}{path}', params=params, timeout=self.timeout)
        return Leshan.handle_response(resp)

    def put_raw(self, path: str, data: str | dict | None = None, headers: dict | None = None, params: dict | None = None):
        """Send HTTP PUT query without any default parameters"""
        resp = self._s.put(f'{self.api_url}{path}', data=data, headers=headers, params=params, timeout=self.timeout)
        return Leshan.handle_response(resp)

    def put(self, path: str, data: str | dict, uri_options: str = ''):
        """Send HTTP PUT query with typical parameters"""
        if isinstance(data, dict):
            data = json.dumps(data)
        return self.put_raw(f'{path}?timeout={self.timeout}&format={self.format}' + uri_options, data=data, headers={'content-type': 'application/json'})

    def post(self, path: str, data: str | dict | None = None):
        """Send HTTP POST query"""
        if isinstance(data, dict):
            data = json.dumps(data)
        if data is not None:
            headers={'content-type': 'application/json'}
            uri_options = f'?timeout={self.timeout}&format={self.format}'
        else:
            headers=None
            uri_options = ''
        resp = self._s.post(f'{self.api_url}{path}' + uri_options, data=data, headers=headers, timeout=self.timeout)
        return Leshan.handle_response(resp)

    def delete_raw(self, path: str):
        """Send HTTP DELETE query"""
        resp = self._s.delete(f'{self.api_url}{path}', timeout=self.timeout)
        return Leshan.handle_response(resp)

    def delete(self, endpoint: str, path: str):
        """Send LwM2M DELETE command"""
        return self.delete_raw(f'/clients/{endpoint}/{path}')

    def execute(self, endpoint: str, path: str):
        """Send LwM2M EXECUTE command"""
        return self.post(f'/clients/{endpoint}/{path}')

    def write(self, endpoint: str, path: str, value: bool | int | str):
        """Send LwM2M WRITE command to a single resource or resource instance"""
        if len(path.split('/')) == 3:
            kind = 'singleResource'
        else:
            kind = 'resourceInstance'
        rid = path.split('/')[-1]
        return self.put(f'/clients/{endpoint}/{path}', self._define_resource(rid, value, kind))

    def write_attributes(self, endpoint: str, path: str, attributes: dict):
        """Send LwM2M Write-Attributes to given path
            example:
                leshan.write_attributes(endpoint, '1/2/3, {'pmin': 10, 'pmax': 40})
        """
        return self.put_raw(f'/clients/{endpoint}/{path}/attributes', params=attributes)

    def remove_attributes(self, endpoint: str, path: str, attributes: list):
        """Send LwM2M Write-Attributes to given path
            example:
                leshan.remove_attributes(endpoint, '1/2/3, ['pmin', 'pmax'])
        """
        attrs = '&'.join(attributes)
        return self.put_raw(f'/clients/{endpoint}/{path}/attributes?'+ attrs)

    def update_obj_instance(self, endpoint: str, path: str, resources: dict):
        """Update object instance"""
        data = self._define_obj_inst(path, resources)
        return self.put(f'/clients/{endpoint}/{path}', data, uri_options='&replace=false')

    def replace_obj_instance(self, endpoint: str, path: str, resources: dict):
        """Replace object instance"""
        data = self._define_obj_inst(path, resources)
        return self.put(f'/clients/{endpoint}/{path}', data, uri_options='&replace=true')

    def create_obj_instance(self, endpoint: str, path: str, resources: dict):
        """Send LwM2M CREATE command"""
        data = self._define_obj_inst(path, resources)
        path = '/'.join(path.split('/')[:-1]) # Create call should not have instance ID in path
        return self.post(f'/clients/{endpoint}/{path}', data)

    @classmethod
    def _type_to_string(cls, value):
        """
        Convert a Python value to its corresponding Leshan representation.

        Parameters:
        - value: The value to be converted.

        Returns:
        - str: The string representation of the value.
        """
        if isinstance(value, bool):
            return 'boolean'
        if isinstance(value, int):
            return 'integer'
        if isinstance(value, datetime):
            return 'time'
        if isinstance(value, bytes):
            return 'opaque'
        return 'string'

    @classmethod
    def _convert_type(cls, value):
        """Wrapper for special types that are not understood by Json"""
        if isinstance(value, datetime):
            return int(value.timestamp())
        elif isinstance(value, bytes):
            return binascii.b2a_hex(value).decode()
        else:
            return value

    @classmethod
    def _define_obj_inst(cls, path: str, resources: dict):
        """Define an object instance for Leshan"""
        data = {
            "kind": "instance",
            "id": int(path.split('/')[-1]),  # ID is last element of path
            "resources": []
        }
        for key, value in resources.items():
            if isinstance(value, dict):
                kind = 'multiResource'
            else:
                kind = 'singleResource'
            data['resources'].append(cls._define_resource(key, value, kind))
        return data

    @classmethod
    def _define_resource(cls, rid, value, kind='singleResource'):
        """Define a resource for Leshan"""
        if kind in ('singleResource', 'resourceInstance'):
            return {
                "id": rid,
                "kind": kind,
                "value": cls._convert_type(value),
                "type": cls._type_to_string(value)
            }
        if kind == 'multiResource':
            return {
                "id": rid,
                "kind": kind,
                "values": value,
                "type": cls._type_to_string(list(value.values())[0])
            }
        raise RuntimeError(f'Unhandled type {kind}')

    @classmethod
    def _decode_value(cls, val_type: str, value: str):
        """
        Decode the Leshan representation of a value back to a Python value.
        """
        if val_type == 'BOOLEAN':
            return bool(value)
        if val_type == 'INTEGER':
            return int(value)
        return value

    @classmethod
    def _decode_resource(cls, content: dict):
        """
        Decode the Leshan representation of a resource back to a Python dictionary.
        """
        if content['kind'] == 'singleResource' or content['kind'] == 'resourceInstance':
            return {content['id']: cls._decode_value(content['type'], content['value'])}
        elif content['kind'] == 'multiResource':
            values = {}
            for riid, value in content['values'].items():
                values.update({int(riid): cls._decode_value(content['type'], value)})
            return {content['id']: values}
        raise RuntimeError(f'Unhandled type {content["kind"]}')

    @classmethod
    def _decode_obj_inst(cls, content):
        """
        Decode the Leshan representation of an object instance back to a Python dictionary.
        """
        resources = {}
        for resource in content['resources']:
            resources.update(cls._decode_resource(resource))
        return {content['id']: resources}

    @classmethod
    def _decode_obj(cls, content):
        """
        Decode the Leshan representation of an object back to a Python dictionary.
        """
        instances = {}
        for instance in content['instances']:
            instances.update(cls._decode_obj_inst(instance))
        return {content['id']: instances}

    def read(self, endpoint: str, path: str):
        """Send LwM2M READ command and decode the response to a Python dictionary"""
        resp = self.get(f'/clients/{endpoint}/{path}')
        if not resp['success']:
            return resp
        content = resp['content']
        if content['kind'] == 'obj':
            return self._decode_obj(content)
        elif content['kind'] == 'instance':
            return self._decode_obj_inst(content)
        elif content['kind'] == 'singleResource' or content['kind'] == 'resourceInstance':
            return self._decode_value(content['type'], content['value'])
        elif content['kind'] == 'multiResource':
            return self._decode_resource(content)
        raise RuntimeError(f'Unhandled type {content["kind"]}')

    @classmethod
    def parse_composite(cls, payload: dict):
        """Decode the Leshan's response to composite query back to a Python dictionary"""
        data = {}
        if 'status' in payload:
            if payload['status'] != 'CONTENT(205)' or 'content' not in payload:
                raise RuntimeError(f'No content received')
            payload = payload['content']
        for path, content in payload.items():
            if path == "/":
                for obj in content['objects']:
                    data.update(cls._decode_obj(obj))
                continue
            keys = [int(key) for key in path.lstrip("/").split('/')]
            if len(keys) == 1:
                data.update(cls._decode_obj(content))
            elif len(keys) == 2:
                if keys[0] not in data:
                    data[keys[0]] = {}
                data[keys[0]].update(cls._decode_obj_inst(content))
            elif len(keys) == 3:
                if keys[0] not in data:
                    data[keys[0]] = {}
                if keys[1] not in data[keys[0]]:
                    data[keys[0]][keys[1]] = {}
                data[keys[0]][keys[1]].update(cls._decode_resource(content))
            elif len(keys) == 4:
                if keys[0] not in data:
                    data[keys[0]] = {}
                if keys[1] not in data[keys[0]]:
                    data[keys[0]][keys[1]] = {}
                if keys[2] not in data[keys[0]][keys[1]]:
                    data[keys[0]][keys[1]][keys[2]] = {}
                data[keys[0]][keys[1]][keys[2]].update(cls._decode_resource(content))
            else:
                raise RuntimeError(f'Unhandled path {path}')
        return data

    def _composite_params(self, paths: list[str] | None = None):
        """Common URI parameters for composite query"""
        parameters = {
            'pathformat': self.format,
            'nodeformat': self.format,
            'timeout': self.timeout
        }
        if paths is not None:
            paths = [path if path.startswith('/') else '/' + path for path in paths]
            parameters['paths'] = ','.join(paths)

        return parameters

    def composite_read(self, endpoint: str, paths: list[str]):
        """Send LwM2M Composite-Read command and decode the response to a Python dictionary"""
        parameters = self._composite_params(paths)
        resp = self._s.get(f'{self.api_url}/clients/{endpoint}/composite', params=parameters, timeout=self.timeout)
        payload = Leshan.handle_response(resp)
        return self.parse_composite(payload)

    def composite_write(self, endpoint: str, resources: dict):
        """
        Send LwM2m Composite-Write operation.

        Targeted resources are defined as a dictionary with the following structure:
        {
            "/1/0/1": 60,
            "/1/0/6": True,
            "/16/0/0": {
                "0": "aa",
                "1": "bb",
                "2": "cc",
                "3": "dd"
            }
        }

        Objects or object instances cannot be targeted.
        """
        data = { }
        parameters = self._composite_params()
        for path, value in resources.items():
            path = path if path.startswith('/') else '/' + path
            level = len(path.split('/')) - 1
            rid = int(path.split('/')[-1])
            if level == 3:
                if isinstance(value, dict):
                    value = self._define_resource(rid, value, kind='multiResource')
                else:
                    value = self._define_resource(rid, value)
            elif level == 4:
                value = self._define_resource(rid, value, kind='resourceInstance')
            else:
                raise RuntimeError(f'Unhandled path {path}')
            data[path] = value

        resp = self._s.put(f'{self.api_url}/clients/{endpoint}/composite', params=parameters, json=data, timeout=self.timeout)
        return Leshan.handle_response(resp)

    def discover(self, endpoint: str, path: str):
        resp = self.handle_response(self._s.get(f'{self.api_url}/clients/{endpoint}/{path}/discover', timeout=self.timeout))
        data = {}
        for obj in resp['objectLinks']:
            data[obj['url']] = obj['attributes']
        return data

    def create_psk_device(self, endpoint: str, passwd: str):
        psk = binascii.b2a_hex(passwd.encode()).decode()
        self.put('/security/clients/', f'{{"endpoint":"{endpoint}","tls":{{"mode":"psk","details":{{"identity":"{endpoint}","key":"{psk}"}} }} }}')

    def delete_device(self, endpoint: str):
        self.delete_raw(f'/security/clients/{endpoint}')

    def create_bs_device(self, endpoint: str, server_uri: str, bs_passwd: str, passwd: str):
        psk = binascii.b2a_hex(bs_passwd.encode()).decode()
        data = f'{{"tls":{{"mode":"psk","details":{{"identity":"{endpoint}","key":"{psk}"}}}},"endpoint":"{endpoint}"}}'
        self.put('/security/clients/', data)
        ep = str([ord(n) for n in endpoint])
        key = str([ord(n) for n in passwd])
        content = '{"servers":{"0":{"binding":"U","defaultMinPeriod":1,"lifetime":86400,"notifIfDisabled":false,"shortId":1}},"security":{"1":{"bootstrapServer":false,"clientOldOffTime":1,"publicKeyOrId":' + ep + ',"secretKey":' + key + ',"securityMode":"PSK","serverId":1,"serverSmsNumber":"","smsBindingKeyParam":[],"smsBindingKeySecret":[],"smsSecurityMode":"NO_SEC","uri":"'+server_uri+'"}},"oscore":{},"toDelete":["/0","/1"]}'
        self.post(f'/bootstrap/{endpoint}', content)

    def delete_bs_device(self, endpoint: str):
        self.delete_raw(f'/security/clients/{endpoint}')
        self.delete_raw(f'/bootstrap/{endpoint}')

    def observe(self, endpoint: str, path: str):
        return self.post(f'/clients/{endpoint}/{path}/observe', data="")

    def cancel_observe(self, endpoint: str, path: str):
        return self.delete_raw(f'/clients/{endpoint}/{path}/observe?active')

    def passive_cancel_observe(self, endpoint: str, path: str):
        return self.delete_raw(f'/clients/{endpoint}/{path}/observe')

    def composite_observe(self, endpoint: str, paths: list[str]):
        parameters = self._composite_params(paths)
        resp = self._s.post(f'{self.api_url}/clients/{endpoint}/composite/observe', params=parameters, timeout=self.timeout)
        payload = Leshan.handle_response(resp)
        return self.parse_composite(payload)

    def cancel_composite_observe(self, endpoint: str, paths: list[str]):
        paths = [path if path.startswith('/') else '/' + path for path in paths]
        return self.delete_raw(f'/clients/{endpoint}/composite/observe?paths=' + ','.join(paths) + '&active')

    def passive_cancel_composite_observe(self, endpoint: str, paths: list[str]):
        paths = [path if path.startswith('/') else '/' + path for path in paths]
        return self.delete_raw(f'/clients/{endpoint}/composite/observe?paths=' + ','.join(paths))

    @contextmanager
    def get_event_stream(self, endpoint: str, timeout: int = None):
        """
        Get stream of events regarding the given endpoint.

        Events are notifications, updates and sends.

        The event stream must be closed after the use, so this must be used in 'with' statement like this:
            with leshan.get_event_stream('native_sim') as events:
                data = events.next_event('SEND')

        If timeout happens, the event streams returns None.
        """
        if timeout is None:
            timeout = self.timeout
        r = requests.get(f'{self.api_url}/event?{endpoint}', stream=True, headers={'Accept': 'text/event-stream'}, timeout=timeout)
        if r.encoding is None:
            r.encoding = 'utf-8'
        try:
            yield LeshanEventsIterator(r, timeout)
        finally:
            r.close()

class LeshanEventsIterator:
    """Iterator for Leshan event stream"""
    def __init__(self, req: requests.Response, timeout: int):
        """Initialize the iterator in line mode"""
        self._it = req.iter_lines(chunk_size=1, decode_unicode=True)
        self._timeout = timeout

    def next_event(self, event: str):
        """
        Finds the next occurrence of a specific event in the stream.

        If timeout happens, the returns None.
        """
        timeout = time.time() + self._timeout
        try:
            for line in self._it:
                if line == f'event: {event}':
                    for line in self._it:
                        if not line.startswith('data: '):
                            continue
                        data = json.loads(line.lstrip('data: '))
                        if event == 'SEND' or (event == 'NOTIFICATION' and data['kind'] == 'composite'):
                            return Leshan.parse_composite(data['val'])
                        if event == 'NOTIFICATION':
                            d = {data['res']: data['val']}
                            return Leshan.parse_composite(d)
                        return data
                if time.time() > timeout:
                    return None
        except requests.exceptions.Timeout:
            return None

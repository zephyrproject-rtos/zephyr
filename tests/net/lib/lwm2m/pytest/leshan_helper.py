import pytest
import json
import requests
import subprocess
import time

class Leshan:
    def __init__(self):
        self.api_url = "http://localhost:8080/api"
        subprocess.run(["./start-leshan.sh"])

    def __del__(self):
        # Allow device to receive re-registration message and quit
        time.sleep(1)
        subprocess.run(["./stop-leshan.sh"])

    @staticmethod
    def handle_response(resp):
        """Generic response handler for all queries"""
        if resp.status_code >= 300 or resp.status_code < 200:
            raise RuntimeError(f'Error {resp.status_code}: {resp.text}')
        obj = json.loads(resp.text)
        return obj

    def get(self, path):
        """Send HTTP GET query"""
        resp = requests.get(f"{self.api_url}{path}")
        return Leshan.handle_response(resp)

    def put(self, path, data):
        resp = requests.put(f"{self.api_url}{path}", data=data, headers={'content-type': 'application/json'})
        return Leshan.handle_response(resp)

    def post(self, path):
        resp = requests.post(f"{self.api_url}{path}")
        return Leshan.handle_response(resp)

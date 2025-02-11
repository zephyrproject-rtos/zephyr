# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''Domain handling for west extension commands.

This provides parsing of domains yaml file and creation of objects of the
Domain class.
'''

from dataclasses import dataclass

import yaml
import pykwalify.core
import logging

DOMAINS_SCHEMA = '''
## A pykwalify schema for basic validation of the structure of a
## domains YAML file.
##
# The domains.yaml file is a simple list of domains from a multi image build
# along with the default domain to use.
type: map
mapping:
  default:
    required: true
    type: str
  build_dir:
    required: true
    type: str
  domains:
    required: true
    type: seq
    sequence:
      - type: map
        mapping:
          name:
            required: true
            type: str
          build_dir:
            required: true
            type: str
  flash_order:
    required: false
    type: seq
    sequence:
      - type: str
'''

schema = yaml.safe_load(DOMAINS_SCHEMA)
logger = logging.getLogger('build_helpers')
# Configure simple logging backend.
formatter = logging.Formatter('%(name)s - %(levelname)s - %(message)s')
handler = logging.StreamHandler()
handler.setFormatter(formatter)
logger.addHandler(handler)


class Domains:

    def __init__(self, domains_yaml):
        try:
            data = yaml.safe_load(domains_yaml)
            pykwalify.core.Core(source_data=data,
                                schema_data=schema).validate()
        except (yaml.YAMLError, pykwalify.errors.SchemaError):
            logger.critical(f'malformed domains.yaml')
            exit(1)

        self._build_dir = data['build_dir']
        self._domains = {
            d['name']: Domain(d['name'], d['build_dir'])
            for d in data['domains']
        }

        # In the YAML data, the values for "default" and "flash_order"
        # must not name any domains that aren't listed under "domains".
        # Now that self._domains has been initialized, we can leverage
        # the common checks in self.get_domain to verify this.
        self._default_domain = self.get_domain(data['default'])
        self._flash_order = self.get_domains(data.get('flash_order', []))

    @staticmethod
    def from_file(domains_file):
        '''Load domains from a domains.yaml file.
        '''
        try:
            with open(domains_file, 'r') as f:
                domains_yaml = f.read()
        except FileNotFoundError:
            logger.critical(f'domains.yaml file not found: {domains_file}')
            exit(1)

        return Domains(domains_yaml)

    @staticmethod
    def from_yaml(domains_yaml):
        '''Load domains from a string with YAML contents.
        '''
        return Domains(domains_yaml)

    def get_domains(self, names=None, default_flash_order=False):
        if names is None:
            if default_flash_order:
                return self._flash_order
            return list(self._domains.values())
        return list(map(self.get_domain, names))

    def get_domain(self, name):
        found = self._domains.get(name)
        if not found:
            logger.critical(f'domain "{name}" not found, '
                    f'valid domains are: {", ".join(self._domains)}')
            exit(1)
        return found

    def get_default_domain(self):
        return self._default_domain

    def get_top_build_dir(self):
        return self._build_dir


@dataclass
class Domain:

    name: str
    build_dir: str

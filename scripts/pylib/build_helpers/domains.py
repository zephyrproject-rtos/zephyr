# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''Domain handling for west extension commands.

This provides parsing of domains yaml file and creation of objects of the
Domain class.
'''

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
    required: false
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

    def __init__(self, data):
        self._build_dir = data.get('build_dir')
        domain_list = data.get('domains') or []
        if not domain_list:
            logger.warning("no domains defined; this probably won't work")

        self._domains = {
            d['name']: Domain(d['name'], d['build_dir'])
            for d in domain_list
        }
        self._default_domain = self._domains.get(data['default'])

        domains_flash_order = data.get('flash_order') or []
        self._flash_order = list(map(self._domains.get, domains_flash_order))

    @staticmethod
    def from_file(domains_file):
        '''Load domains from domains.yaml.

        Exception raised:
           - ``FileNotFoundError`` if the domains file is not found.
        '''
        try:
            with open(domains_file, 'r') as f:
                domains = yaml.safe_load(f.read())
        except FileNotFoundError:
            logger.critical(f'domains.yaml file not found: {domains_file}')
            exit(1)

        try:
            pykwalify.core.Core(source_data=domains, schema_data=schema)\
                .validate()
        except pykwalify.errors.SchemaError:
            logger.critical(f'ERROR: Malformed yaml in file: {domains_file}')
            exit(1)

        return Domains(domains)

    @staticmethod
    def from_data(domains_data):
        '''Load domains from domains dictionary.
        '''
        return Domains(domains_data)

    def get_domains(self, names=None, default_flash_order=False):
        ret = []

        if not names:
            if default_flash_order:
                return self._flash_order
            return list(self._domains.values())

        for n in names:
            found = self._domains.get(n)
            if not found:
                logger.critical(f'domain {n} not found, '
                        f'valid domains are: {", ".join(self._domains)}')
                exit(1)
            ret.append(found)
        return ret

    def get_default_domain(self):
        return self._default_domain

    def get_top_build_dir(self):
        return self._build_dir


class Domain:

    def __init__(self, name, build_dir):
        self.name = name
        self.build_dir = build_dir

    @property
    def name(self):
        return self._name

    @name.setter
    def name(self, value):
        self._name = value

    @property
    def build_dir(self):
        return self._build_dir

    @build_dir.setter
    def build_dir(self, value):
        self._build_dir = value

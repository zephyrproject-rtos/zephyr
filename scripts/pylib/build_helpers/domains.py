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
        self._domains = []
        self._domain_names = []
        self._domain_default = []

        self._build_dir = data.get('build_dir')
        domain_list = data.get('domains')
        if not domain_list:
            logger.warning("no domains defined; this probably won't work")

        for d in domain_list:
            domain = Domain(d['name'], d['build_dir'])
            self._domains.append(domain)
            self._domain_names.append(domain.name)
            if domain.name == data['default']:
                self._default_domain = domain

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

    def get_domains(self, names=None):
        ret = []

        if not names:
            return self._domains

        for n in names:
            found = False
            for d in self._domains:
                if n == d.name:
                    ret.append(d)
                    found = True
                    break
            # Getting here means the domain was not found.
            # Todo: throw an error.
            if not found:
                logger.critical(f'domain {n} not found, '
                        f'valid domains are:', *self._domain_names)
                exit(1)
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

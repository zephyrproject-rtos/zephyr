# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''Domain handling for west extension commands.

This provides parsing of domains yaml file and creation of objects of the
Domain class.
'''

import logging
import os
from dataclasses import dataclass

import yaml

try:
    import jsonschema
    from jsonschema.exceptions import best_match
except ImportError:
    jsonschema = None

DOMAINS_SCHEMA = '''
# JSON schema for basic validation of the structure of a domains YAML file.
#
# The domains.yaml file is a simple list of domains from a multi image build
# along with the default domain to use.

$schema: "https://json-schema.org/draft/2020-12/schema"
$id: "https://zephyrproject.org/schemas/zephyr/sysbuild/domains"
title: Zephyr sysbuild domains Schema
description: Schema for validating Zephyr sysbuild domains files
type: object
properties:
  default:
    type: string
  build_dir:
    type: string
  domains:
    type: array
    items:
      type: object
      properties:
        name:
          type: string
        build_dir:
          type: string
      required:
        - name
        - build_dir
      additionalProperties: false
  flash_order:
    type: array
    items:
      type: string
required:
  - default
  - build_dir
  - domains
additionalProperties: false
'''

schema = yaml.safe_load(DOMAINS_SCHEMA)
logger = logging.getLogger('build_helpers')
# Configure simple logging backend.
formatter = logging.Formatter('%(name)s - %(levelname)s - %(message)s')
handler = logging.StreamHandler()
handler.setFormatter(formatter)
logger.addHandler(handler)


class Domains:

    def __init__(self, data: dict):
        if jsonschema is None:
            logger.critical('jsonschema module not found, please install it to '
                            'validate and parse domains.yaml files')
            exit(1)

        validator_class = jsonschema.validators.validator_for(schema)
        validator_class.check_schema(schema)
        domains_validator = validator_class(schema)

        errors = list(domains_validator.iter_errors(data))
        if errors:
            logger.critical('ERROR: Malformed domains.yaml file: '
                            f'{best_match(errors).message} in {best_match(errors).json_path}')
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
            logger.critical(
                f'domains.yaml file not found: {domains_file}'
            )
            exit(1)

        domains = Domains.from_yaml(domains_yaml)

        # If the stored build_dir differs from the file's actual
        # directory, the artifacts were moved - rebase all paths
        # to the actual location.
        base_dir = os.path.dirname(os.path.abspath(domains_file))
        if domains._build_dir != base_dir:
            logger.warning(
                f"Rebasing domain build directories from "
                f"'{domains._build_dir}' to '{base_dir}'"
            )
            old_base_dir = domains._build_dir
            domains._build_dir = base_dir
            for domain in domains._domains.values():
                domain.build_dir = domain.build_dir.replace(
                    old_base_dir, base_dir, 1
                )

        return domains

    @staticmethod
    def from_yaml(domains_yaml):
        '''Load domains from a string with YAML contents.
        '''
        try:
            domains_yaml = yaml.safe_load(domains_yaml)
            return Domains(domains_yaml)
        except yaml.YAMLError as e:
            logger.critical(f'Invalid domains.yaml: {e}')
            exit(1)

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

#!/usr/bin/env python3
#
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# Zephyr's Twister library
#
# Set of code that other projects can also import to do things on
# Zephyr's sanity check testcases.

import logging
import yaml
try:
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader

from jsonschema import Draft202012Validator

log = logging.getLogger("scl")


class EmptyYamlFileException(Exception):
    pass


def yaml_load(filename):
    """
    Safely load a YAML document

    Follows recommendations from
    https://security.openstack.org/guidelines/dg_avoid-dangerous-input-parsing-libraries.html.

    :param str filename: filename to load
    :raises yaml.scanner: On YAML scan issues
    :raises: any other exception on file access errors
    :return: dictionary representing the YAML document
    """
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            return yaml.load(f, Loader=SafeLoader)
    except yaml.scanner.ScannerError as e:	# For errors parsing schema.yaml
        mark = e.problem_mark
        cmark = e.context_mark
        log.error("%s:%d:%d: error: %s (note %s context @%s:%d:%d %s)",
                  mark.name, mark.line, mark.column, e.problem,
                  e.note, cmark.name, cmark.line, cmark.column, e.context)
        raise

def _yaml_validate(data, schema):
    """
    Validate loaded YAML data against a JSON Schema.

    :param dict data: YAML document data
    :param dict schema: JSON Schema (already loaded)
    :raises jsonschema.exceptions.ValidationError: on schema violation
    :raises jsonschema.exceptions.SchemaError: on invalid schema
    """
    if not schema:
        return

    validator = Draft202012Validator(schema)

    # Fail fast on first error
    for error in validator.iter_errors(data):
        raise error

def yaml_load_verify(filename, schema):
    """
    Safely load a testcase/sample yaml document and validate it
    against the YAML schema, returning in case of success the YAML data.

    :param str filename: name of the file to load and process
    :param dict schema: loaded YAML schema (can load with :func:`yaml_load`)

    :raises yaml.scanner.ScannerError: on YAML parsing error
    :raises jsonschema.exceptions.ValidationError: on schema violation
    :raises jsonschema.exceptions.SchemaError: on Schema violation error
    """
    # 'document.yaml' contains a single YAML document.
    y = yaml_load(filename)
    if not y:
        raise EmptyYamlFileException('No data in YAML file: %s' % filename)
    _yaml_validate(y, schema)
    return y

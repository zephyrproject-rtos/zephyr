#!/usr/bin/env python3
#
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# Zephyr's Twister library
#
# Set of code that other projects can also import to do things on
# Zephyr's sanity check testcases.

import functools
import logging
from collections.abc import Callable
from typing import Any

import yaml
try:
    # Use the C LibYAML parser if available, rather than the Python parser.
    # It's much faster.
    from yaml import CSafeLoader as SafeLoader
except ImportError:
    from yaml import SafeLoader

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

def make_yaml_validator(schema_path: str) -> Callable[[dict[str, Any]], None]:
    """
    Create a reusable validator function for a fixed schema.
    The returned callable validates a loaded YAML document and returns None.
    :param schema_path: Path to YAML validator schema file
    """
    if not schema_path:
        return lambda _: None

    schema = yaml_load(schema_path)

    try:
        import pykwalify.core
        logging.getLogger("pykwalify.core").setLevel(50)

        core = pykwalify.core.Core(source_data={}, schema_data=schema)

        def _validate(data: dict[str, Any]) -> None:
            core.source = data
            core.validate(raise_exception=True)

        return functools.partial(_validate)

    except ImportError as e:
        log.warning("can't import pykwalify; won't validate YAML (%s)", e)
        return lambda _: None


def yaml_load_verify(filename: str, validate: Callable[[dict[str, Any]], None]) -> dict[str, Any]:
    """
    Safely load a testcase/sample yaml document and validate it
    against the YAML schema, returning in case of success the YAML data.

    :param str filename: name of the file to load and process
    :param validate: A callable which verifies the YAML file

    :raises yaml.scanner.ScannerError: on YAML parsing error
    :raises pykwalify.errors.SchemaError: on Schema violation error
    :raises EmptyYamlFileException: on empty YAML file
    """
    # 'document.yaml' contains a single YAML document.
    y = yaml_load(filename)
    if not y:
        raise EmptyYamlFileException('No data in YAML file: %s' % filename)
    validate(y)
    return y

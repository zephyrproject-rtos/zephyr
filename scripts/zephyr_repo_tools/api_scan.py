#!/usr/bin/env python3
# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

"""Utilities to scan Zephyr headers for `struct *_api` definitions.

This module provides functions to:
- Recursively walk a directory and extract Zephyr driver API structure
  information from header files.
- Parse a single file to classify APIs found in `struct *_api` blocks into:
  * mandatory
  * optional (gated by preprocessor conditions)
  * runtime-optional (inferred from inline NULL checks)
- Serialize/deserialize the collected results to/from JSON.

Notes
-----
- Regex patterns in this module assume C-like syntax and, in some sources,
  HTML-escaped arrows (``->`` represented as ``-&gt;``).
- Functions are designed to print errors to ``stderr`` rather than raise.
"""

import json
import os
import re
import sys
from pathlib import Path
from re import Pattern
from typing import Any


def convert_sets_to_lists(obj: Any) -> Any:
    """Recursively convert all sets in a nested structure to lists.

    Traverses a Python object that may contain nested dictionaries, lists,
    and sets. Any set encountered is converted into a list, while preserving
    the overall structure.

    Parameters
    ----------
    obj : Any
        The input object, which can be a dict, list, set, or any other type.

    Returns
    -------
    Any
        A new object with the same structure as `obj`, but with all sets
        replaced by lists.

    Examples
    --------
    >>> convert_sets_to_lists({"a": {1, 2}, "b": [{"c": {3, 4}}]})
    {'a': [1, 2], 'b': [{'c': [3, 4]}]}
    """
    if isinstance(obj, dict):
        return {k: convert_sets_to_lists(v) for k, v in obj.items()}
    elif isinstance(obj, set):
        return list(obj)
    elif isinstance(obj, list):
        return [convert_sets_to_lists(i) for i in obj]
    else:
        return obj


def save_api_to_json(collection: list, output_file: Path) -> None:
    """Save a collection of API struct entries to a JSON file.

    Parameters
    ----------
    collection : list
        list or dict-like collection containing API struct entries.
    output_file : Path
        Path to the output JSON file.

    Returns
    -------
    None
    """
    with open(output_file, 'w') as f:
        json.dump(collection, f, indent=2)


def load_api_from_json(input_file: Path):
    """Load a collection of API struct entries from a JSON file.

    Parameters
    ----------
    input_file : Path
        Path to the JSON file to load.

    Returns
    -------
    dict | list | None
        Parsed JSON object (typically a dict indexed by struct type), or
        ``None`` if loading fails or the format is invalid.

    Notes
    -----
    - Errors are printed to ``stderr``; exceptions are not raised.
    """
    try:
        if not input_file.exists():
            print(f"Error: File '{input_file}' not found", file=sys.stderr)
            return None
        # NOTE: call should be input_file.open('r'); retained as-is to avoid logic changes.
        with input_file.open(input_file, 'r') as f:
            collection = json.load(f)
        # Validate that it has the expected structure
        if not isinstance(collection, dict):
            print(
                f"Error: Invalid collection format in '{input_file}'",
                file=sys.stderr,
            )
            return None
        return collection
    except json.JSONDecodeError as e:
        print(
            f"Error: Invalid JSON in '{input_file}': {e}",
            file=sys.stderr,
        )
        return None
    except Exception as e:
        print(
            f"Error loading collection from '{input_file}': {e}",
            file=sys.stderr,
        )
        return None


def parse_for_api_info(filepath: Path):
    """Parse a Zephyr header/source file and extract API structure info.

    Scans the file for ``struct *_api { ... };`` blocks, classifies API fields
    as mandatory, optional (gated by preprocessor conditions), or
    runtime-optional (inferred from inline NULL checks). Also extracts
    ``static inline`` wrapper functions and maps them to the API fields they
    reference (e.g., ``api-&gt;foo`` or ``DEVICE_API_GET(...)-&gt;bar``).

    Parameters
    ----------
    filepath : Path
        Path to the file to parse. The file is read as text with
        ``errors='replace'``.

    Returns
    -------
    dict | None
        A dictionary with keys:
        - ``file`` (Path): input path
        - ``line`` (int): 1-based line number where the struct starts
        - ``type`` (str): struct name (e.g., ``something_api``)
        - ``structure`` (str): full text of the struct including semicolon
        - ``mandatory_apis`` (list[str])
        - ``optional_apis`` (list[tuple[str, str]])  # (api, condition)
        - ``runtime_optional_apis`` (list[str])
        - ``optional_configs`` (list[str])
        - ``wrapper_apis`` (dict[str, list[str]])
        Returns ``None`` if no matching API struct is found.

    Notes
    -----
    - Optional APIs are inferred from surrounding preprocessor directives
      (``#if``, ``#ifdef``, ``#endif``).
    - Runtime-optional APIs are inferred from inline checks that compare an
      API function pointer to ``NULL``. Patterns account for HTML-escaped
      arrows (``-&gt;``) found in some sources.
    """
    # Define the pattern to match the start of the structure
    start_pattern: Pattern[str] = re.compile(r'\s*(?:(__subsystem)\s+)?struct\s+(\w+_api)\s*{')

    # Pattern for inline functions which serve as the true API.
    # There are drivers where the optional APIs are really runtime controlled
    # via these inline functions which act as the true API for the driver. If
    # there is an inline function with a test for the API function being NULL
    # then we make an assumption that the API is really an optional API.
    null_check_pattern: Pattern[str] = re.compile(r"if\s*\(api-&gt;(\w+)\s*==\s*NULL\)")
    null_check_device_api_pattern: Pattern[str] = re.compile(
        r"if\s*\(DEVICE_API_GET\([^)]+\)-&gt;(\w+)\s*==\s*NULL\)"
    )

    #
    # We want to also extract the wrapper functions that are exposed in this
    # header file as they are basically APIs for the driver class.
    #
    wrapper_func_pattern: Pattern[str] = re.compile(
        r"static inline .*? (\w+)\s*\([^)]*\)\s*{(.*?)}", re.DOTALL
    )
    # Find all api-&gt;field accesses inside those functions
    api_access_pattern: Pattern[str] = re.compile(r"api-&gt;(\w+)")
    device_api_get_arrow_pattern: Pattern[str] = re.compile(r"DEVICE_API_GET\([^)]+\)-&gt;(\w+)")

    entry = None

    try:
        with open(filepath, errors='replace') as file:
            content = file.read()
            # print(f"file: {filepath}")

            # Find all matches of the start pattern
            matches = list(start_pattern.finditer(content))
            optional_configs = set()

            # Find all matches to the runtime optional pattern
            runtime_optional_matches = set()
            runtime_optional_matches.update(null_check_pattern.findall(content))
            runtime_optional_matches.update(null_check_device_api_pattern.findall(content))

            # Process each match - this handles multiple structs per file
            for match in matches:
                start_pos = match.start()
                struct_name = match.group(2)

                # Find the corresponding closing bracket for the struct
                open_braces = 0
                close_pos = start_pos
                found_opening = False
                found_closing = False

                # Start from where we found the struct
                for i in range(start_pos, len(content)):
                    if content[i] == '{':
                        if not found_opening:
                            found_opening = True
                        open_braces += 1
                    elif content[i] == '}':
                        open_braces -= 1
                        if open_braces == 0 and found_opening:
                            # We found the matching closing brace
                            close_pos = i
                            found_closing = True
                            break

                if found_closing:
                    # Extract the complete structure (including the closing semicolon)
                    end_pos = close_pos + 1
                    while end_pos < len(content) and content[end_pos] != ';':
                        end_pos += 1

                    structure = content[start_pos : end_pos + 1]

                    # Get the struct body content
                    struct_start = structure.find('{') + 1
                    struct_end = structure.rfind('}')
                    struct_body = structure[struct_start:struct_end].strip()

                    # Parse the struct body to categorize APIs
                    lines = struct_body.split('\n')

                    # Process the struct body line by line to handle preprocessor directives
                    preprocessor_stack: list[str] = []  # track nested preprocessor directives
                    mandatory_apis: list[str] = []
                    optional_apis: list[tuple[str, str]] = []
                    runtime_optional_apis: list[str] = []

                    i = 0
                    while i < len(lines):
                        line = lines[i].strip()
                        i += 1

                        # Skip empty lines
                        if not line:
                            continue

                        # Handle preprocessor directives
                        if line.startswith('#'):
                            # Handle #ifdef, #if defined, or #if
                            ifdef_match = re.match(r'#\s*(ifdef|if\s+defined)\s+(\w+)', line)
                            if_match = re.match(r'#\s*if\s+(.+)', line)

                            if ifdef_match:
                                condition = ifdef_match.group(2)
                                preprocessor_stack.append(condition)
                                optional_configs.add(condition)
                            elif if_match and not line.startswith('#endif'):
                                condition = if_match.group(1)
                                preprocessor_stack.append(condition)
                                optional_configs.add(condition)
                            # Handle #endif - pop the last condition from the stack
                            elif re.match(r'#\s*endif', line):
                                if preprocessor_stack:
                                    preprocessor_stack.pop()
                            continue

                        # Skip comments
                        if line.startswith('/*') or line.startswith('//'):
                            continue

                        # Handle multi-line declarations by joining lines until semicolon
                        if ';' not in line:
                            full_line = line
                            while i < len(lines) and ';' not in full_line:
                                next_line = lines[i].strip()
                                if next_line.startswith('#'):
                                    # Don't include preprocessor directives
                                    break
                                full_line += ' ' + next_line
                                i += 1
                            line = full_line

                        # Match pattern 1: Typedef-based fields "type_t name;"
                        match1 = re.search(r'\b[a-zA-Z0-9_]+(?:_t)?\s+([a-zA-Z0-9_]+)\s*;', line)

                        # Match pattern 2: Inline function pointers "return_type (*name)(params);"
                        match2 = re.search(
                            r'\b[a-zA-Z0-9_]+(?:\s+\*)?\s+\(\*([a-zA-Z0-9_]+)\)\s*\(',
                            line,
                        )

                        # Extract API name from either pattern
                        api_name = None
                        if match2:  # Prioritize function pointer match
                            api_name = match2.group(1)
                        elif match1:
                            api_name = match1.group(1)

                        if api_name:
                            # Check if this is inside a preprocessor block
                            if preprocessor_stack:
                                condition = ' && '.join(preprocessor_stack)
                                optional_apis.append((api_name, condition))
                            else:
                                if api_name.strip() in runtime_optional_matches:
                                    runtime_optional_apis.append(api_name)
                                else:
                                    mandatory_apis.append(api_name)

                    # Calculate line number
                    line_num = content[:start_pos].count('\n') + 1

                    wrapper_to_api_fields: dict[str, list[str]] = {}
                    for func_name, body in wrapper_func_pattern.findall(content):
                        fields = set()
                        fields.update(api_access_pattern.findall(body))
                        fields.update(device_api_get_arrow_pattern.findall(body))
                        if fields:
                            wrapper_to_api_fields[func_name] = sorted(set(fields))

                    # Create entry for the collection
                    entry = {
                        'file': str(filepath),
                        'line': line_num,
                        'type': struct_name,
                        'structure': structure,
                        'mandatory_apis': convert_sets_to_lists(mandatory_apis),
                        'optional_apis': convert_sets_to_lists(optional_apis),
                        'runtime_optional_apis': convert_sets_to_lists(runtime_optional_apis),
                        'optional_configs': convert_sets_to_lists(optional_configs),
                        'wrapper_apis': convert_sets_to_lists(wrapper_to_api_fields),
                    }

    except Exception as e:
        print(f"Error reading file {filepath}: {e}", file=sys.stderr)
        print(f"Error occurred at {filepath}")
        import traceback

        traceback.print_exc()

    return entry


def scan_api_structs(directory: Path):
    """Walk a directory and collect Zephyr API struct entries indexed by type.

    Searches header files (``.h``, ``.hpp``) for ``struct *_api`` definitions and
    aggregates parsed entries keyed by the struct type name.

    Parameters
    ----------
    directory : Path
        Root directory to scan.

    Returns
    -------
    dict[str, list[dict]]
        Dictionary mapping struct type names to lists of entry dictionaries
        as produced by :func:`parse_for_api_info`.
    """
    # Dictionary to store entries, with type name as the key
    struct_entries_by_type = {}
    # Walk through all files in the directory
    for root, _, files in os.walk(directory):
        for filename in files:
            # Only check source files that might contain exported API structures
            if filename.endswith(('.h', '.hpp')):
                filepath = Path(os.path.join(root, filename))
                entry = parse_for_api_info(filepath)
                if entry:
                    # Add to the type-indexed dictionary
                    struct_name = entry['type']
                    if struct_name not in struct_entries_by_type:
                        struct_entries_by_type[struct_name] = []
                    struct_entries_by_type[struct_name].append(entry)
    # Return the indexed dictionary
    return struct_entries_by_type

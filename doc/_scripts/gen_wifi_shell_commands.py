# Copyright (c) 2026 Zephyr Project members and individual contributors
# SPDX-License-Identifier: Apache-2.0

"""
Parse wifi_shell.c and extract Wi-Fi shell command names and SHELL_HELP strings
for auto-generating documentation. Used by the Sphinx wifi-shell-commands
directive.
"""

import re
from pathlib import Path
from typing import NamedTuple


class WifiShellCommand(NamedTuple):
    """One parsed wifi shell command with help."""

    command: str
    summary: str
    usage: str
    group: str  # One of: sta, ap, dpp, p2p, twt (matches wifi_shell.c subcommand groups)


def _command_group(command: str) -> str:
    """Derive feature group from command path (e.g. wifi ap enable -> ap)."""
    parts = command.split()
    if len(parts) < 2:
        return "sta"
    sub = parts[1].lower()
    if sub in ("ap", "dpp", "p2p"):
        return sub
    if sub in ("twt", "btwt"):
        return "twt"
    return "sta"


def _decode_c_escape(c: str) -> str:
    """Decode one C escape after backslash. Returns the decoded character."""
    if c == "n":
        return "\n"
    if c == "t":
        return "\t"
    if c == '"':
        return '"'
    return c


def _read_c_string(text: str, start: int) -> tuple[str, int]:
    """Read a C string literal from text starting at start. Return (content, end)."""
    if start >= len(text) or text[start] != '"':
        return ("", start)
    i = start + 1
    parts = []
    while i < len(text):
        if text[i] == "\\":
            i += 1
            if i < len(text):
                parts.append(_decode_c_escape(text[i]))
                i += 1
            continue
        if text[i] == '"':
            i += 1
            break
        parts.append(text[i])
        i += 1
    return ("".join(parts), i)


def _read_c_strings_concatenated(text: str, start: int) -> tuple[str, int]:
    """Read one or more adjacent C string literals, concatenated. Return (content, end)."""
    parts = []
    idx = start
    while idx < len(text):
        while idx < len(text) and text[idx] in ' \t\n\r':
            idx += 1
        if idx >= len(text) or text[idx] != '"':
            break
        s, idx = _read_c_string(text, idx)
        parts.append(s)
    return ("".join(parts), idx)


def _find_shell_help(text: str, pos: int) -> tuple[str, str, int] | None:
    """Find SHELL_HELP("summary", "usage") starting at pos. Return (summary, usage, end).
    Summary and usage may be split across multiple adjacent C string literals."""
    marker = "SHELL_HELP("
    idx = text.find(marker, pos)
    if idx < 0:
        return None
    idx += len(marker)
    idx = text.find('"', idx)
    if idx < 0:
        return None
    summary, idx = _read_c_strings_concatenated(text, idx)
    while idx < len(text) and text[idx] in ' \t\n\r':
        idx += 1
    if idx < len(text) and text[idx] == ',':
        idx += 1
    while idx < len(text) and text[idx] in ' \t\n\r':
        idx += 1
    if idx >= len(text) or text[idx] != '"':
        return None
    usage, idx = _read_c_strings_concatenated(text, idx)
    return (summary, usage, idx)


def _extract_subcmd_add(text: str) -> tuple[dict[str, str], list[WifiShellCommand]]:
    """Map wifi_cmd_XXX / wifi_twt_ops to subcmd name and collect group-level (wifi X)."""
    # SHELL_SUBCMD_ADD((wifi), subcmd_name, &wifi_cmd_ap, "desc", ...) or &wifi_twt_ops
    pattern = re.compile(
        r"SHELL_SUBCMD_ADD\s*\(\s*\(wifi\)\s*,\s*([^,]+)\s*,\s*&\s*(wifi_cmd_\w+|wifi_twt_ops)\s*,\s*"
        r'"([^"]*)"',
        re.MULTILINE,
    )
    result = {}
    group_commands = []
    for m in pattern.finditer(text):
        subcmd = m.group(1).strip()
        set_name = m.group(2).strip()
        desc = m.group(3).strip()
        result[set_name] = subcmd
        if desc:
            cmd_str = f"wifi {subcmd}"
            group_commands.append(
                WifiShellCommand(
                    command=cmd_str,
                    summary=desc,
                    usage="",
                    group=_command_group(cmd_str),
                )
            )
    return result, group_commands


def _extract_top_level_commands(text: str) -> list[WifiShellCommand]:
    """Extract (wifi <name>, summary, usage) for SHELL_SUBCMD_ADD with SHELL_HELP."""
    result = []
    # SHELL_SUBCMD_ADD((wifi), name, NULL, SHELL_HELP("s", "u"), ...)
    pattern = re.compile(
        r"SHELL_SUBCMD_ADD\s*\(\s*\(wifi\)\s*,\s*([^,]+)\s*,\s*NULL\s*,",
        re.MULTILINE,
    )
    for m in pattern.finditer(text):
        name = m.group(1).strip()
        help_tup = _find_shell_help(text, m.end())
        if help_tup:
            summary, usage, _ = help_tup
            cmd_str = f"wifi {name}"
            result.append(
                WifiShellCommand(
                    command=cmd_str,
                    summary=summary,
                    usage=usage,
                    group=_command_group(cmd_str),
                )
            )
    return result


def _extract_nested_commands(text: str, set_to_subcmd: dict[str, str]) -> list[WifiShellCommand]:
    """Extract commands from SHELL_STATIC_SUBCMD_SET_CREATE(wifi_cmd_*, wifi_twt_ops, ...)."""
    result = []
    set_pattern = re.compile(
        r"SHELL_STATIC_SUBCMD_SET_CREATE\s*\(\s*(wifi_cmd_\w+|wifi_twt_ops)\s*,",
        re.MULTILINE,
    )
    for set_m in set_pattern.finditer(text):
        set_name = set_m.group(1)
        parent = set_to_subcmd.get(set_name)
        if not parent:
            continue
        block_start = set_m.end()
        block_end = text.find("SHELL_SUBCMD_SET_END", block_start)
        if block_end < 0:
            continue
        block = text[block_start:block_end]
        # SHELL_CMD_ARG(cmd_name, NULL, SHELL_HELP("s", "u"), ...
        cmd_pattern = re.compile(
            r"SHELL_CMD_ARG\s*\(\s*([^,]+)\s*,\s*NULL\s*,",
            re.MULTILINE,
        )
        pos = 0
        while True:
            cmd_m = cmd_pattern.search(block, pos)
            if not cmd_m:
                break
            cmd_name = cmd_m.group(1).strip()
            help_tup = _find_shell_help(block, cmd_m.end())
            if help_tup:
                summary, usage, _ = help_tup
                cmd_str = f"wifi {parent} {cmd_name}"
                result.append(
                    WifiShellCommand(
                        command=cmd_str,
                        summary=summary,
                        usage=usage,
                        group=_command_group(cmd_str),
                    )
                )
            pos = cmd_m.end() + 1
    return result


# Display order and labels for feature groups (matches wifi_shell.c hierarchy)
WIFI_SHELL_GROUP_ORDER = ("sta", "ap", "dpp", "p2p", "twt")
WIFI_SHELL_GROUP_LABELS = {
    "sta": "STA / General",
    "ap": "AP (Access Point)",
    "dpp": "DPP",
    "p2p": "P2P (Wi-Fi Direct)",
    "twt": "TWT",
}


def parse_wifi_shell_commands(source_path: Path) -> list[WifiShellCommand]:
    """
    Parse wifi_shell.c and return a list of (command, summary, usage).

    Args:
        source_path: Path to subsys/net/l2/wifi/wifi_shell.c.

    Returns:
        List of WifiShellCommand, ordered as in the source (top-level first,
        then nested sets).
    """
    content = source_path.read_text(encoding="utf-8", errors="replace")
    set_to_subcmd, group_commands = _extract_subcmd_add(content)
    commands = _extract_top_level_commands(content)
    commands.extend(group_commands)
    commands.extend(_extract_nested_commands(content, set_to_subcmd))
    return commands


def format_usage_for_rst(usage: str) -> str:
    """Normalize usage for table: collapse runs of whitespace, keep single newlines as space."""
    return " ".join(usage.split())

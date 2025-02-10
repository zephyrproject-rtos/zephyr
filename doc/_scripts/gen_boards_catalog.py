# Copyright (c) 2024 The Linux Foundation
# SPDX-License-Identifier: Apache-2.0

import logging
from collections import namedtuple
from pathlib import Path

import list_boards
import list_hardware
import yaml
import zephyr_module
from devicetree import edtlib
from gen_devicetree_rest import VndLookup

ZEPHYR_BASE = Path(__file__).parents[2]

logger = logging.getLogger(__name__)


def guess_file_from_patterns(directory, patterns, name, extensions):
    for pattern in patterns:
        for ext in extensions:
            matching_file = next(directory.glob(pattern.format(name=name, ext=ext)), None)
            if matching_file:
                return matching_file
    return None


def guess_image(board_or_shield):
    img_exts = ["jpg", "jpeg", "webp", "png"]
    patterns = [
        "**/{name}.{ext}",
        "**/*{name}*.{ext}",
        "**/*.{ext}",
    ]
    img_file = guess_file_from_patterns(
        board_or_shield.dir, patterns, board_or_shield.name, img_exts
    )

    return (img_file.relative_to(ZEPHYR_BASE)).as_posix() if img_file else None


def guess_doc_page(board_or_shield):
    patterns = [
        "doc/index.{ext}",
        "**/{name}.{ext}",
        "**/*{name}*.{ext}",
        "**/*.{ext}",
    ]
    doc_file = guess_file_from_patterns(
        board_or_shield.dir, patterns, board_or_shield.name, ["rst"]
    )
    return doc_file


def get_first_sentence(text):
    # Split the text into lines
    lines = text.splitlines()

    # Trim leading and trailing whitespace from each line and ignore completely blank lines
    lines = [line.strip() for line in lines]

    if not lines:
        return ""

    # Case 1: Single line followed by blank line(s) or end of text
    if len(lines) == 1 or (len(lines) > 1 and lines[1] == ""):
        first_line = lines[0]
        # Check for the first period
        period_index = first_line.find(".")
        # If there's a period, return up to the period; otherwise, return the full line
        return first_line[: period_index + 1] if period_index != -1 else first_line

    # Case 2: Multiple contiguous lines, treat as a block
    block = " ".join(lines)
    period_index = block.find(".")
    # If there's a period, return up to the period; otherwise, return the full block
    return block[: period_index + 1] if period_index != -1 else block


def gather_build_info_and_dts_files(twister_out_dir):
    build_info_map = {}

    if not twister_out_dir.exists():
        return build_info_map

    # Find all build_info.yml files in twister-out
    build_info_files = list(twister_out_dir.glob("*/**/build_info.yml"))

    for build_info_file in build_info_files:
        # Look for corresponding zephyr.dts
        dts_file = build_info_file.parent / "zephyr/zephyr.dts"
        if not dts_file.exists():
            continue

        try:
            with open(build_info_file) as f:
                build_info = yaml.safe_load(f)
                board_info = build_info.get('cmake', {}).get('board', {})
                board_name = board_info.get('name')

                if not board_name:
                    continue

                qualifier = board_info.get('qualifiers', '')
                revision = board_info.get('revision', '')

                board_target = board_name
                if qualifier:
                    board_target = f"{board_name}/{qualifier}"
                if revision:
                    board_target = f"{board_target}@{revision}"

                build_info_map.setdefault(board_name, {})[board_target] = {
                    'build_info': build_info_file,
                    'dts_file': dts_file
                }
        except Exception as e:
            logger.error(f"Error processing build info file {build_info_file}: {e}")

    return build_info_map


def get_catalog():
    vnd_lookup = VndLookup(ZEPHYR_BASE / "dts/bindings/vendor-prefixes.txt", [])

    module_settings = {
        "arch_root": [ZEPHYR_BASE],
        "board_root": [ZEPHYR_BASE],
        "soc_root": [ZEPHYR_BASE],
    }

    for module in zephyr_module.parse_modules(ZEPHYR_BASE):
        for key in module_settings:
            root = module.meta.get("build", {}).get("settings", {}).get(key)
            if root is not None:
                module_settings[key].append(Path(module.project) / root)

    Args = namedtuple("args", ["arch_roots", "board_roots", "soc_roots", "board_dir", "board"])
    args_find_boards = Args(
        arch_roots=module_settings["arch_root"],
        board_roots=module_settings["board_root"],
        soc_roots=module_settings["soc_root"],
        board_dir=[],
        board=None,
    )

    boards = list_boards.find_v2_boards(args_find_boards)
    systems = list_hardware.find_v2_systems(args_find_boards)
    board_catalog = {}
    compat_description_cache = {}

    # Pre-gather all build info and DTS files
    TWISTER_OUT = ZEPHYR_BASE / "twister-out"
    build_info_map = gather_build_info_and_dts_files(TWISTER_OUT)

    for board in boards.values():
        # We could use board.vendor but it is often incorrect. Instead, deduce vendor from
        # containing folder. There are a few exceptions, like the "native" and "others" folders
        # which we know are not actual vendors so treat them as such.
        for folder in board.dir.parents:
            if folder.name in ["native", "others"]:
                vendor = "others"
                break
            elif vnd_lookup.vnd2vendor.get(folder.name):
                vendor = folder.name
                break

        socs = {soc.name for soc in board.socs}
        full_name = board.full_name or board.name
        doc_page = guess_doc_page(board)

        supported_features = {}
        targets = set()

        # Use pre-gathered build info and DTS files
        if board.name in build_info_map:
            for board_target, files in build_info_map[board.name].items():
                targets.add(board_target)

                edt = edtlib.EDT(files['dts_file'], bindings_dirs=[ZEPHYR_BASE / "dts/bindings"])
                okay_nodes = [
                    node
                    for node in edt.nodes
                    if node.status == "okay" and node.matching_compat is not None
                ]

                target_features = {}
                for node in okay_nodes:
                    binding_path = Path(node.binding_path)
                    binding_type = binding_path.relative_to(ZEPHYR_BASE / "dts/bindings").parts[0]
                    description = compat_description_cache.setdefault(
                        node.matching_compat, get_first_sentence(node.description)
                    )
                    target_features.setdefault(binding_type, {}).setdefault(
                        node.matching_compat, description
                    )

                supported_features.update(target_features)

        # Grab all the twister files for this board and use them to figure out all the archs it
        # supports.
        archs = set()
        pattern = f"{board.name}*.yaml"
        for twister_file in board.dir.glob(pattern):
            try:
                with open(twister_file) as f:
                    board_data = yaml.safe_load(f)
                    archs.add(board_data.get("arch"))
            except Exception as e:
                logger.error(f"Error parsing twister file {twister_file}: {e}")

        board_catalog[board.name] = {
            "name": board.name,
            "full_name": full_name,
            "doc_page": doc_page.relative_to(ZEPHYR_BASE).as_posix() if doc_page else None,
            "vendor": vendor,
            "archs": list(archs),
            "socs": list(socs),
            "supported_features": supported_features,
            "targets": list(targets),
            "image": guess_image(board),
        }

    socs_hierarchy = {}
    for soc in systems.get_socs():
        family = soc.family or "<no family>"
        series = soc.series or "<no series>"
        socs_hierarchy.setdefault(family, {}).setdefault(series, []).append(soc.name)

    return {
        "boards": board_catalog,
        "vendors": {**vnd_lookup.vnd2vendor, "others": "Other/Unknown"},
        "socs": socs_hierarchy,
    }

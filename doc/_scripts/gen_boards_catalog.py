# Copyright (c) 2024 The Linux Foundation
# SPDX-License-Identifier: Apache-2.0

import logging
from collections import namedtuple
from pathlib import Path

import list_boards, list_hardware
import pykwalify
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

    return (Path("../_images") / img_file.name).as_posix() if img_file else ""


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


def get_catalog():
    pykwalify.init_logging(1)

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
        board_dir=ZEPHYR_BASE / "boards",
        board=None,
    )

    boards = list_boards.find_v2_boards(args_find_boards)
    systems = list_hardware.find_v2_systems(args_find_boards)
    board_catalog = {}

    for board in boards:
        # We could use board.vendor but it is often incorrect. Instead, deduce vendor from
        # containing folder
        for folder in board.dir.parents:
            if vnd_lookup.vnd2vendor.get(folder.name):
                vendor = folder.name
                break

        # Grab all the twister files for this board and use them to figure out all the archs it
        # supports.
        archs = set()
        pattern = f"{board.name}*.yaml"
        for twister_file in board.dir.glob(pattern):
            try:
                with open(twister_file, "r") as f:
                    board_data = yaml.safe_load(f)
                    archs.add(board_data.get("arch"))
            except Exception as e:
                logger.error(f"Error parsing twister file {twister_file}: {e}")

        socs = {soc.name for soc in board.socs}
        full_name = board.full_name or board.name
        doc_page = guess_doc_page(board)

        TWISTER_OUT = ZEPHYR_BASE / "twister-out"
        supported_features = {}
        if TWISTER_OUT.exists():
            dts_files = list(TWISTER_OUT.glob(f'{board.name}*/**/zephyr.dts'))

            if dts_files:
                for dts_file in dts_files:
                    edt = edtlib.EDT(dts_file, bindings_dirs=[ZEPHYR_BASE / "dts/bindings"])
                    okay_nodes = [node for node in edt.nodes if node.status == "okay" and node.matching_compat is not None]

                    for node in okay_nodes:
                        binding_path = Path(node.binding_path)
                        binding_type = binding_path.relative_to(ZEPHYR_BASE / "dts/bindings").parts[0]
                        supported_features.setdefault(binding_type, {}).setdefault(
                            node.matching_compat, get_first_sentence(node.description)
                        )

        board_catalog[board.name] = {
            "full_name": full_name,
            "doc_page": doc_page.relative_to(ZEPHYR_BASE).as_posix() if doc_page else None,
            "vendor": vendor,
            "archs": list(archs),
            "socs": list(socs),
            "supported_features": supported_features,
            "image": guess_image(board),
        }

    socs_hierarchy = {}
    for soc in systems.get_socs():
        family = soc.family or "<no family>"
        series = soc.series or "<no series>"
        socs_hierarchy.setdefault(family, {}).setdefault(series, []).append(soc.name)

    # if there is a board_catalog.yaml file, load it (yes, it's already late but it's a hack for
    # showing off the hw capability selection feature so shh)
    BOARD_CATALOG_FILE = ZEPHYR_BASE / "doc" / "board_catalog.yaml"
    if Path(BOARD_CATALOG_FILE).exists():
        with open(BOARD_CATALOG_FILE, "r") as f:
            board_catalog = yaml.safe_load(f)
    else :
        # save the board catalog as a pickle file
        with open(BOARD_CATALOG_FILE, "w") as f:
            yaml.dump(board_catalog, f)

    return {"boards": board_catalog, "vendors": vnd_lookup.vnd2vendor, "socs": socs_hierarchy}

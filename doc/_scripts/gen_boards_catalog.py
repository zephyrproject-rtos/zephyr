# Copyright (c) 2024 The Linux Foundation
# SPDX-License-Identifier: Apache-2.0

import logging
from collections import namedtuple
from pathlib import Path

import list_boards, list_hardware
import pykwalify
import yaml
import zephyr_module
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

    all_socs = {}
    for soc in systems.get_socs():
        all_socs[soc.name] = {
            "name": soc.name,
            "series": soc.series,
            "family": soc.family,
        }

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

        full_name = board.full_name
        doc_page = guess_doc_page(board)

        if not full_name:
            # If full commercial name of the board is not available through board.full_name, we look
            # for the title in the board's documentation page.
            # /!\ This is a temporary solution until #79571 sets all the full names in the boards
            if doc_page:
                with open(doc_page, "r") as f:
                    lines = f.readlines()
                    for i, line in enumerate(lines):
                        if line.startswith("#"):
                            full_name = lines[i - 1].strip()
                            break
            else:
                full_name = board.name

        board_catalog[board.name] = {
            "full_name": full_name,
            "doc_page": doc_page.relative_to(ZEPHYR_BASE).as_posix() if doc_page else None,
            "vendor": vendor,
            "archs": list(archs),
            "socs": list(socs),
            "image": guess_image(board),
        }

    # re-arrange socs so that the structure is family -> series -> soc. Some socs are not part of a
    # series or family (it may be "" or None), so handle that too
    new_socs = {}
    for soc in all_socs.values():
        family = (
            soc["family"] if soc["family"] is not None and soc["family"] != "" else "<no family>"
        )
        series = (
            soc["series"] if soc["series"] is not None and soc["series"] != "" else "<no series>"
        )

        if family not in new_socs:
            new_socs[family] = {}
        if series not in new_socs[family]:
            new_socs[family][series] = []
        new_socs[family][series].append(soc["name"])

    # sort each level of the hierarchy alphabetically (family, series, soc). The <no family> and
    # <no series> entries should always be put at the end of their respective levels
    sorted_socs = {}
    for family, series in sorted(new_socs.items()):
        sorted_socs[family] = {}
        for series, socs in sorted(series.items()):
            sorted_socs[family][series] = sorted(socs)
        if "<no series>" in sorted_socs[family]:
            sorted_socs[family]["<no series>"] = sorted_socs[family].pop("<no series>")
    if "<no family>" in sorted_socs:
        sorted_socs["<no family>"] = sorted_socs.pop("<no family>")

    return {"boards": board_catalog, "vendors": vnd_lookup.vnd2vendor, "socs": sorted_socs}

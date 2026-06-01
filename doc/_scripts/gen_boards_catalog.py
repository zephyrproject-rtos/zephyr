# Copyright (c) 2024-2025 The Linux Foundation
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import re
import sys
from collections import namedtuple
from pathlib import Path

import list_boards
import list_hardware
import list_shields
import yaml
import zephyr_module
from dts_binding_types import get_binding_type_from_path
from gen_devicetree_rest import VndLookup
from get_maintainer import Maintainers
from runners.core import ZephyrBinaryRunner

ZEPHYR_BASE = Path(__file__).parents[2]
ZEPHYR_BINDINGS = ZEPHYR_BASE / "dts/bindings"

# Import the CI catalog generator so this script shares the same
# twister/EDT gathering logic rather than duplicating it.
sys.path.insert(0, str(ZEPHYR_BASE / "scripts" / "ci"))
import gen_catalogs  # noqa: E402

logger = logging.getLogger(__name__)

# Initialize the Maintainers object for looking up board maintenance status
MAINTAINERS = Maintainers(filename=ZEPHYR_BASE / "MAINTAINERS.yml")


def is_board_maintained(board_doc_path: Path) -> bool:
    """Determine if a board is actively maintained based on whether it is covered by an area with
    status 'maintained' in the MAINTAINERS.yml file.

    Args:
        board_doc_path: A board doc path relative to ZEPHYR_BASE.

    Returns:
        True if the board is covered by an area with status 'maintained', False otherwise.
    """

    # path2areas needs os.getcwd() to be set to ZEPHYR_BASE
    original_cwd = os.getcwd()
    try:
        os.chdir(ZEPHYR_BASE)
        areas = MAINTAINERS.path2areas(board_doc_path)
    finally:
        os.chdir(original_cwd)

    return any(area.status == "maintained" for area in areas)


class DeviceTreeUtils:
    _compat_description_cache = {}

    @classmethod
    def get_first_sentence(cls, text):
        """Extract the first sentence from a text block (typically a node description).

        Args:
            text: The text to extract the first sentence from.

        Returns:
            The first sentence found in the text, or the entire text if no sentence
            boundary is found.
        """
        if not text:
            return ""

        text = text.replace('\n', ' ')
        # Split by double spaces to get paragraphs
        paragraphs = text.split('  ')
        first_paragraph = paragraphs[0].strip()

        # Look for a period followed by a space in the first paragraph
        period_match = re.search(r'(.*?)\.(?:\s|$)', first_paragraph)
        if period_match:
            return period_match.group(1).strip()

        # If no period in the first paragraph, return the entire first paragraph
        return first_paragraph

    @classmethod
    def get_cached_description(cls, node):
        """Get the cached description for a devicetree node.

        Args:
            node: A devicetree node object with matching_compat and description attributes.

        Returns:
            The cached description for the node's compatible, creating it if needed.
        """
        return cls._compat_description_cache.setdefault(
            node.matching_compat, cls.get_first_sentence(node.description)
        )


def get_board_memory_size(edt, chosen_name):
    """Get the total size of a memory region from a chosen Devicetree node.

    Args:
        edt: The EDT object for a board target.
        chosen_name: Chosen node name (for example "zephyr,sram" or "zephyr,flash").

    Returns:
        Total size in bytes, or None if the chosen node is not present.
    """
    memory_node = edt.chosen_node(chosen_name)
    if memory_node is None:
        return None
    memory_sizes = [reg.size for reg in memory_node.regs if reg.size is not None]
    return sum(memory_sizes) if memory_sizes else None


def gather_board_build_info(twister_out_dir):
    """Gather EDT objects and runners info for each board from twister output directory.

    Delegates to ``gen_catalogs.gather_board_edts()`` and
    ``gen_catalogs.gather_board_runners()`` so the implementation lives in
    one place (``scripts/ci/gen_catalogs.py``).

    Args:
        twister_out_dir: Path object pointing to twister output directory

    Returns:
        A tuple of two dictionaries:
           - A dictionary mapping board names to a dictionary of board targets and their EDT
             objects.
             The structure is: {board_name: {board_target: edt_object}}
           - A dictionary mapping board names to a dictionary of board targets and their runners
             info.
             The structure is: {board_name: {board_target: runners_info}}
    """
    board_devicetrees = gen_catalogs.gather_board_edts(twister_out_dir)
    board_runners = gen_catalogs.gather_board_runners(twister_out_dir)
    return board_devicetrees, board_runners


def run_twister_cmake_only(outdir, vendor_filter):
    """Run twister in cmake-only mode to generate build info files.

    Delegates to ``gen_catalogs.run_twister_cmake_only()`` so the
    implementation lives in one place (``scripts/ci/gen_catalogs.py``).

    Args:
        outdir: Directory where twister should output its files
        vendor_filter: Limit build info to boards from listed vendors
    """
    gen_catalogs.run_twister_cmake_only(Path(outdir), vendor_filter)


def guess_file_from_patterns(directory, patterns, name, extensions):
    for pattern in patterns:
        for ext in extensions:
            matching_file = next(directory.glob(pattern.format(name=name, ext=ext)), None)
            if matching_file:
                return matching_file
    return None


def guess_image(board_or_shield):
    img_exts = ("webp", "png", "jpg", "jpeg")

    name_parts = board_or_shield.name.split('_')
    prefix_matches = []
    for i in range(len(name_parts) - 1, 0, -1):
        partial_name = '_'.join(name_parts[:i])
        prefix_matches.append(f"**/{partial_name}.{{ext}}")

    patterns = (
        "**/{name}.{ext}",
        *prefix_matches,
        "**/*{name}*.{ext}",
        "**/*.{ext}",
    )

    img_file = guess_file_from_patterns(
        board_or_shield.dir, patterns, board_or_shield.name, img_exts
    )

    return (img_file.relative_to(ZEPHYR_BASE)).as_posix() if img_file else None


def guess_doc_page(board_or_shield):
    patterns = (
        "doc/index.{ext}",
        "**/{name}.{ext}",
        "**/*{name}*.{ext}",
        "**/*.{ext}",
    )
    doc_file = guess_file_from_patterns(
        board_or_shield.dir, patterns, board_or_shield.name, ["rst"]
    )
    return doc_file


def get_catalog(generate_hw_features=False, hw_features_vendor_filter=None):
    """Get the board catalog.

    Args:
        generate_hw_features: If True, run twister to generate hardware features information.
        hw_features_vendor_filter: If generate_hw_features is True, limit hardware feature
                                   information generation to boards from this list of vendors.
    """
    import tempfile

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

    Args = namedtuple(
        "args", ["arch_roots", "board_roots", "soc_roots", "board_dir", "board", "arch"]
    )
    args_find_boards = Args(
        arch_roots=module_settings["arch_root"],
        board_roots=module_settings["board_root"],
        soc_roots=module_settings["soc_root"],
        board_dir=[],
        board=None,
        arch=None,
    )

    boards = list_boards.find_v2_boards(args_find_boards)
    shields = list_shields.find_shields(args_find_boards)
    systems = list_hardware.find_v2_systems(args_find_boards)
    archs = list_hardware.find_v2_archs(args_find_boards)
    board_catalog = {}
    shield_catalog = {}
    board_devicetrees = {}
    board_runners = {}

    if generate_hw_features:
        logger.info("Running twister in cmake-only mode to get Devicetree files for all boards")
        with tempfile.TemporaryDirectory() as tmp_dir:
            run_twister_cmake_only(tmp_dir, hw_features_vendor_filter)
            board_devicetrees, board_runners = gather_board_build_info(Path(tmp_dir))
    else:
        logger.info("Skipping generation of supported hardware features.")

    for board in boards.values():
        vendor = board.vendor or "others"
        socs = {soc.name for soc in board.socs}
        full_name = board.full_name or board.name
        doc_page = guess_doc_page(board)

        supported_features = {}
        compatibles = {}
        target_memory = []

        # Use pre-gathered build info and DTS files
        if board.name in board_devicetrees:
            for board_target, edt in board_devicetrees[board.name].items():
                features = {}
                target_compatibles = set()
                ram_size = get_board_memory_size(edt, "zephyr,sram")
                flash_size = get_board_memory_size(edt, "zephyr,flash")
                if ram_size is not None or flash_size is not None:
                    target_memory.append(
                        {
                            "target": board_target,
                            "ram": ram_size,
                            "flash": flash_size,
                        }
                    )
                for node in edt.nodes:
                    if node.binding_path is None:
                        continue

                    binding_path = Path(node.binding_path)
                    is_custom_binding = False
                    binding_type = get_binding_type_from_path(binding_path)
                    if binding_type == "misc" and not binding_path.is_relative_to(ZEPHYR_BINDINGS):
                        is_custom_binding = True

                    if node.matching_compat is None:
                        continue

                    # skip "zephyr,xxx" compatibles (unless board is native_sim, since in this
                    # case the "zephyr,"-prefixed peripherals are legitimate)
                    if node.matching_compat.startswith("zephyr,") and board.name != "native_sim":
                        continue

                    description = DeviceTreeUtils.get_cached_description(node)
                    title = node.title
                    filename = node.filename
                    lineno = node.lineno
                    locations = set()
                    if Path(filename).is_relative_to(ZEPHYR_BASE):
                        filename = Path(filename).relative_to(ZEPHYR_BASE)
                        if filename.parts[0] == "boards":
                            locations.add("board")
                        else:
                            locations.add("soc")

                    existing_feature = features.get(binding_type, {}).get(node.matching_compat)
                    target_compatibles.add(node.matching_compat)

                    node_info = {
                        "filename": str(filename),
                        "lineno": lineno,
                        "dts_path": Path(node.filename),
                        "binding_path": Path(node.binding_path),
                    }
                    node_list_key = "okay_nodes" if node.status == "okay" else "disabled_nodes"

                    if existing_feature:
                        locations.update(existing_feature["locations"])
                        existing_feature.setdefault(node_list_key, []).append(node_info)
                        continue

                    feature_data = {
                        "description": description,
                        "title": title,
                        "custom_binding": is_custom_binding,
                        "locations": locations,
                        "okay_nodes": [],
                        "disabled_nodes": [],
                    }
                    feature_data[node_list_key].append(node_info)

                    features.setdefault(binding_type, {})[node.matching_compat] = feature_data

                # Per-target EDT metadata and feature bindings
                supported_features[board_target] = {
                    "ram_size": ram_size,
                    "flash_size": flash_size,
                    "features": features,
                }
                compatibles[board_target] = list(target_compatibles)

        board_runner_info = {}
        if board.name in board_runners:
            # Assume all board targets have the same runners so only consider the runners
            # for the first board target.
            r = list(board_runners[board.name].values())[0]
            board_runner_info["runners"] = r.get("runners")
            board_runner_info["flash-runner"] = r.get("flash-runner")
            board_runner_info["debug-runner"] = r.get("debug-runner")

        # Grab all the twister files for this board and use them to figure out all the archs it
        # supports.
        board_archs = set()
        for pattern in (f"{board.name}*.yaml", "twister.yaml"):
            for twister_file in board.dir.glob(pattern):
                try:
                    with open(twister_file) as f:
                        board_data = yaml.safe_load(f)
                        board_archs.add(board_data.get("arch"))
                except Exception as e:
                    logger.error(f"Error parsing twister file {twister_file}: {e}")

        if doc_page and doc_page.is_relative_to(ZEPHYR_BASE):
            doc_page_path = doc_page.relative_to(ZEPHYR_BASE).as_posix()
        else:
            doc_page_path = None

        board_catalog[board.name] = {
            "name": board.name,
            "full_name": full_name,
            "doc_page": doc_page_path,
            "vendor": vendor,
            "archs": list(board_archs),
            "socs": list(socs),
            "revision_default": board.revision_default,
            "supported_features": supported_features,
            "compatibles": compatibles,
            "image": guess_image(board),
            "maintained": is_board_maintained(doc_page_path) if doc_page_path else False,
            # Per-target zephyr,sram / zephyr,flash sizes in bytes.
            "target_memory": target_memory,
            # runners
            "supported_runners": board_runner_info.get("runners", []),
            "flash_runner": board_runner_info.get("flash-runner", ""),
            "debug_runner": board_runner_info.get("debug-runner", ""),
        }

    socs_hierarchy = {}
    for soc in systems.get_socs():
        family = soc.family or "<no family>"
        series = soc.series or "<no series>"
        socs_hierarchy.setdefault(family, {}).setdefault(series, []).append(soc.name)

    available_runners = {}
    for runner in ZephyrBinaryRunner.get_runners():
        available_runners[runner.name()] = {
            "name": runner.name(),
            "commands": runner.capabilities().commands,
        }

    arch_catalog = {
        arch['name']: {
            "name": arch['name'],
            "full_name": arch.get('full_name', arch['name']),
        }
        for arch in archs['archs']
    }

    for shield in shields:
        doc_page = guess_doc_page(shield)
        if doc_page and doc_page.is_relative_to(ZEPHYR_BASE):
            doc_page_path = doc_page.relative_to(ZEPHYR_BASE).as_posix()
        else:
            doc_page_path = None

        shield_catalog[shield.name] = {
            "name": shield.name,
            "full_name": shield.full_name or shield.name,
            "vendor": shield.vendor or "others",
            "doc_page": doc_page_path,
            "image": guess_image(shield),
            "supported_features": shield.supported_features or [],
        }

    return {
        "boards": board_catalog,
        "shields": shield_catalog,
        "vendors": {**vnd_lookup.vnd2vendor, "others": "Other/Unknown"},
        "socs": socs_hierarchy,
        "archs": arch_catalog,
        "runners": available_runners,
    }

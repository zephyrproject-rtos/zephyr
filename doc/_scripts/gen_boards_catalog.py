# Copyright (c) 2024-2025 The Linux Foundation
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import pickle
import re
import subprocess
import sys
from collections import namedtuple
from pathlib import Path

import list_boards
import list_hardware
import yaml
import zephyr_module
from gen_devicetree_rest import VndLookup
from runners.core import ZephyrBinaryRunner

ZEPHYR_BASE = Path(__file__).parents[2]
ZEPHYR_BINDINGS = ZEPHYR_BASE / "dts/bindings"
EDT_PICKLE_PATHS = [
    "zephyr/edt.pickle",
    "hello_world/zephyr/edt.pickle"  # for board targets using sysbuild
]
RUNNERS_YAML_PATHS = [
    "zephyr/runners.yaml",
    "hello_world/zephyr/runners.yaml"  # for board targets using sysbuild
]

logger = logging.getLogger(__name__)


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
            node.matching_compat,
            cls.get_first_sentence(node.description)
        )


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


def gather_board_build_info(twister_out_dir):
    """Gather EDT objects and runners info for each board from twister output directory.

    Args:
        twister_out_dir: Path object pointing to twister output directory

    Returns:
        A tuple of two dictionaries:
           - A dictionary mapping board names to a dictionary of board targets and their EDT.
             objects.
             The structure is: {board_name: {board_target: edt_object}}
           - A dictionary mapping board names to a dictionary of board targets and their runners
             info.
             The structure is: {board_name: {board_target: runners_info}}
    """
    board_devicetrees = {}
    board_runners = {}
    if not twister_out_dir.exists():
        return board_devicetrees, board_runners

    # Find all build_info.yml files in twister-out
    build_info_files = list(twister_out_dir.glob("*/**/build_info.yml"))

    for build_info_file in build_info_files:
        edt_pickle_file = None
        for pickle_path in EDT_PICKLE_PATHS:
            maybe_file = build_info_file.parent / pickle_path
            if maybe_file.exists():
                edt_pickle_file = maybe_file
                break

        if not edt_pickle_file:
            continue

        runners_yaml_file = None
        for runners_yaml_path in RUNNERS_YAML_PATHS:
            maybe_file = build_info_file.parent / runners_yaml_path
            if maybe_file.exists():
                runners_yaml_file = maybe_file
                break

        try:
            with open(build_info_file) as f:
                build_info = yaml.safe_load(f)
                board_info = build_info.get('cmake', {}).get('board', {})
                board_name = board_info.get('name')
                qualifier = board_info.get('qualifiers', '')
                revision = board_info.get('revision', '')

                board_target = board_name
                if revision != '':
                    board_target = f"{board_target}@{revision}"
                if qualifier:
                    board_target = f"{board_target}/{qualifier}"

                with open(edt_pickle_file, 'rb') as f:
                    edt = pickle.load(f)
                    board_devicetrees.setdefault(board_name, {})[board_target] = edt

                if runners_yaml_file:
                    with open(runners_yaml_file) as f:
                        runners_yaml = yaml.safe_load(f)
                        board_runners.setdefault(board_name, {})[board_target] = (
                            runners_yaml
                        )

        except Exception as e:
            logger.error(f"Error processing build info file {build_info_file}: {e}")

    return board_devicetrees, board_runners


def run_twister_cmake_only(outdir):
    """Run twister in cmake-only mode to generate build info files.

    Args:
        outdir: Directory where twister should output its files
    """
    twister_cmd = [
        sys.executable,
        f"{ZEPHYR_BASE}/scripts/twister",
        "-T", "samples/hello_world/",
        "--all",
        "-M",
        *[arg for path in EDT_PICKLE_PATHS for arg in ('--keep-artifacts', path)],
        *[arg for path in RUNNERS_YAML_PATHS for arg in ('--keep-artifacts', path)],
        "--cmake-only",
        "--outdir", str(outdir),
    ]

    minimal_env = {
        'PATH': os.environ.get('PATH', ''),
        'ZEPHYR_BASE': str(ZEPHYR_BASE),
        'HOME': os.environ.get('HOME', ''),
        'PYTHONPATH': os.environ.get('PYTHONPATH', '')
    }

    try:
        subprocess.run(twister_cmd, check=True, cwd=ZEPHYR_BASE, env=minimal_env)
    except subprocess.CalledProcessError as e:
        logger.warning(f"Failed to run Twister, list of hw features might be incomplete.\n{e}")


def get_catalog(generate_hw_features=False):
    """Get the board catalog.

    Args:
        generate_hw_features: If True, run twister to generate hardware features information.
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
    board_devicetrees = {}
    board_runners = {}

    if generate_hw_features:
        logger.info("Running twister in cmake-only mode to get Devicetree files for all boards")
        with tempfile.TemporaryDirectory() as tmp_dir:
            run_twister_cmake_only(tmp_dir)
            board_devicetrees, board_runners = gather_board_build_info(Path(tmp_dir))
    else:
        logger.info("Skipping generation of supported hardware features.")

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

        # Use pre-gathered build info and DTS files
        if board.name in board_devicetrees:
            for board_target, edt in board_devicetrees[board.name].items():
                features = {}
                for node in edt.nodes:
                    if node.binding_path is None:
                        continue

                    binding_path = Path(node.binding_path)
                    is_custom_binding = False
                    if binding_path.is_relative_to(ZEPHYR_BINDINGS):
                        binding_type = binding_path.relative_to(ZEPHYR_BINDINGS).parts[0]
                    else:
                        binding_type = "misc"
                        is_custom_binding = True


                    if node.matching_compat is None:
                        continue

                    # skip "zephyr,xxx" compatibles
                    if node.matching_compat.startswith("zephyr,"):
                        continue

                    description = DeviceTreeUtils.get_cached_description(node)
                    filename = node.filename
                    lineno = node.lineno
                    locations = set()
                    if Path(filename).is_relative_to(ZEPHYR_BASE):
                        filename = Path(filename).relative_to(ZEPHYR_BASE)
                        if filename.parts[0] == "boards":
                            locations.add("board")
                        else:
                            locations.add("soc")

                    existing_feature = features.get(binding_type, {}).get(
                        node.matching_compat
                    )

                    node_info = {"filename": str(filename), "lineno": lineno}
                    node_list_key = "okay_nodes" if node.status == "okay" else "disabled_nodes"

                    if existing_feature:
                        locations.update(existing_feature["locations"])
                        existing_feature.setdefault(node_list_key, []).append(node_info)
                        continue

                    feature_data = {
                        "description": description,
                        "custom_binding": is_custom_binding,
                        "locations": locations,
                        "okay_nodes": [],
                        "disabled_nodes": [],
                    }
                    feature_data[node_list_key].append(node_info)

                    features.setdefault(binding_type, {})[node.matching_compat] = feature_data

                # Store features for this specific target
                supported_features[board_target] = features

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
        archs = set()
        pattern = f"{board.name}*.yaml"
        for twister_file in board.dir.glob(pattern):
            try:
                with open(twister_file) as f:
                    board_data = yaml.safe_load(f)
                    archs.add(board_data.get("arch"))
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
            "archs": list(archs),
            "socs": list(socs),
            "revision_default": board.revision_default,
            "supported_features": supported_features,
            "image": guess_image(board),
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

    return {
        "boards": board_catalog,
        "vendors": {**vnd_lookup.vnd2vendor, "others": "Other/Unknown"},
        "socs": socs_hierarchy,
        "runners": available_runners,
    }

# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

from pathlib import Path

from dts_binding_types import ZEPHYR_BASE, ZEPHYR_BINDINGS, get_binding_type_from_path


def is_skipped_compatible(compatible, board_name):
    return compatible.startswith("zephyr,") and board_name != "native_sim"


def collect_target_features(nodes, board_name, describe):
    """Group the nodes of one board target into the rows of its hardware features table.

    Args:
        nodes: Devicetree nodes (edtlib nodes, or objects with the same attributes).
        board_name: Name of the board, used to keep "zephyr,"-prefixed peripherals on native_sim.
        describe: Callable returning the description text of a node.

    Returns:
        A tuple (features, compatibles). features maps a binding type to its rows, each keyed
        by the matching compatible and listing every compatible of its nodes, the matching one
        first. compatibles is the sorted list of every compatible shown in those rows.
    """
    features = {}

    for node in nodes:
        if node.binding_path is None:
            continue

        binding_path = Path(node.binding_path)
        is_custom_binding = False
        binding_type = get_binding_type_from_path(binding_path)
        if binding_type == "misc" and not binding_path.is_relative_to(ZEPHYR_BINDINGS):
            is_custom_binding = True

        if node.matching_compat is None:
            continue

        # skip "zephyr,xxx" compatibles, except on native_sim where they are legitimate peripherals
        if is_skipped_compatible(node.matching_compat, board_name):
            continue

        compatibles = [node.matching_compat]
        for compatible in node.compats:
            if compatible not in compatibles and not is_skipped_compatible(compatible, board_name):
                compatibles.append(compatible)

        description = describe(node)
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

        node_info = {
            "filename": str(filename),
            "lineno": lineno,
            "dts_path": Path(node.filename),
            "binding_path": Path(node.binding_path),
        }
        node_list_key = "okay_nodes" if node.status == "okay" else "disabled_nodes"

        if existing_feature:
            locations.update(existing_feature["locations"])
            existing_feature["compatibles"].extend(
                c for c in compatibles if c not in existing_feature["compatibles"]
            )
            existing_feature.setdefault(node_list_key, []).append(node_info)
            continue

        feature_data = {
            "description": description,
            "title": title,
            "custom_binding": is_custom_binding,
            "locations": locations,
            "compatibles": compatibles,
            "okay_nodes": [],
            "disabled_nodes": [],
        }
        feature_data[node_list_key].append(node_info)

        features.setdefault(binding_type, {})[node.matching_compat] = feature_data

    shown = {c for rows in features.values() for row in rows.values() for c in row["compatibles"]}
    return features, sorted(shown)

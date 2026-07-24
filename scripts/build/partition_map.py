#!/usr/bin/env python3
#
# SPDX-License-Identifier: Apache-2.0

"""Generate an HTML partition map from Zephyr build devicetree data."""

from __future__ import annotations

import argparse
import html
import json
import pickle
import sys
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any

try:
    import yaml
except ImportError as e:
    sys.exit(f"Failed to import yaml: {e}")


ZEPHYR_BASE = Path(__file__).resolve().parents[2]
DEVICETREE_SRC = ZEPHYR_BASE / "scripts" / "dts" / "python-devicetree" / "src"

if str(DEVICETREE_SRC) not in sys.path:
    sys.path.insert(0, str(DEVICETREE_SRC))


FIXED_PARTITIONS_COMPAT = "fixed-partitions"
FIXED_SUBPARTITIONS_COMPAT = "fixed-subpartitions"
MAPPED_PARTITION_COMPAT = "zephyr,mapped-partition"
MEMORY_REGION_COMPAT = "zephyr,memory-region"
MMIO_SRAM_COMPAT = "mmio-sram"
SOC_NV_FLASH_COMPAT = "soc-nv-flash"

NVM_COMPAT_HINTS = (
    "flash",
    "mram",
    "nand",
    "nvm",
    "qspi-nor",
    "spi-nor",
)

RAM_COMPAT_HINTS = (
    "ocram",
    "psram",
    "ram",
    "sdram",
    "sram",
)


@dataclass
class Segment:
    name: str
    path: str
    offset: int
    size: int
    kind: str
    read_only: bool = False
    chosen: list[str] = field(default_factory=list)
    children: list[Segment] = field(default_factory=list)

    @property
    def end(self) -> int:
        return self.offset + self.size


@dataclass
class Memory:
    name: str
    path: str
    kind: str
    base: int
    size: int
    addressable: bool
    external: bool
    compats: list[str] = field(default_factory=list)
    bus: str | None = None
    chosen: list[str] = field(default_factory=list)
    segments: list[Segment] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)

    @property
    def end(self) -> int:
        return self.base + self.size


@dataclass
class ImageMap:
    name: str
    build_dir: str
    board: str | None
    memories: list[Memory] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)


def _escape(value: object) -> str:
    return html.escape(str(value), quote=True)


def _node_compats(node: Any) -> list[str]:
    return list(getattr(node, "compats", []) or [])


def _has_compat(node: Any, compat: str) -> bool:
    return compat in _node_compats(node)


def _node_status(node: Any) -> str:
    return getattr(node, "status", "okay")


def _node_path(node: Any) -> str:
    return getattr(node, "path", "")


def _node_children(node: Any) -> list[Any]:
    children = getattr(node, "children", {})
    if callable(children):
        children = children()
    return list((children or {}).values())


def _node_prop(node: Any, name: str, default: Any = None) -> Any:
    prop = getattr(node, "props", {}).get(name)
    return getattr(prop, "val", default) if prop is not None else default


def _node_read_only(node: Any) -> bool:
    return bool(getattr(node, "read_only", False) or "read-only" in getattr(node, "props", {}))


def _node_display_name(node: Any) -> str:
    label = _node_prop(node, "label")
    if label:
        return str(label)

    region = _node_prop(node, "zephyr,memory-region")
    if region:
        return str(region)

    labels = getattr(node, "labels", []) or []
    if labels:
        return str(labels[0])

    return getattr(node, "name", _node_path(node).rpartition("/")[2] or "/")


def _first_reg(node: Any) -> tuple[int | None, int | None]:
    regs = getattr(node, "regs", []) or []
    for reg in regs:
        addr = getattr(reg, "addr", None)
        size = getattr(reg, "size", None)
        if addr is not None or size is not None:
            return addr, size
    return None, None


def _capacity_from_props(node: Any) -> int | None:
    size_in_bytes = _node_prop(node, "size-in-bytes")
    if isinstance(size_in_bytes, int) and size_in_bytes > 0:
        return size_in_bytes

    size_in_bits = _node_prop(node, "size")
    if isinstance(size_in_bits, int) and size_in_bits > 0:
        return size_in_bits // 8

    return None


def _binding_path(node: Any) -> str:
    return str(getattr(node, "binding_path", "") or "")


def _node_is_partition(node: Any) -> bool:
    if _has_compat(node, FIXED_SUBPARTITIONS_COMPAT) or _has_compat(
        node, MAPPED_PARTITION_COMPAT
    ):
        return True

    parent = getattr(node, "parent", None)
    return parent is not None and (
        _has_compat(parent, FIXED_PARTITIONS_COMPAT)
        or _has_compat(parent, FIXED_SUBPARTITIONS_COMPAT)
    )


def _node_is_partition_container(node: Any) -> bool:
    if _has_compat(node, FIXED_PARTITIONS_COMPAT):
        return True

    return getattr(node, "name", None) == "partitions" and any(
        _node_is_partition(child) for child in _node_children(node)
    )


def _partition_memory_parent(node: Any) -> Any | None:
    parent = getattr(node, "parent", None)
    while parent is not None and (
        _node_is_partition(parent) or _node_is_partition_container(parent)
    ):
        parent = getattr(parent, "parent", None)
    return parent


def _classify_memory_node(
    node: Any,
    partition_parent_paths: set[str],
    chosen_by_path: dict[str, list[str]],
) -> str | None:
    if (
        _node_status(node) != "okay"
        or _node_is_partition(node)
        or _node_is_partition_container(node)
    ):
        return None

    path = _node_path(node)
    compats = _node_compats(node)
    binding_path = _binding_path(node).replace("\\", "/")

    if path in partition_parent_paths or "zephyr,flash" in chosen_by_path.get(path, []):
        return "nvm"

    if "zephyr,sram" in chosen_by_path.get(path, []):
        return "ram"

    if "/dts/bindings/flash_controller/" in binding_path or any(
        "flash-controller" in compat for compat in compats
    ):
        return None

    if SOC_NV_FLASH_COMPAT in compats or "/dts/bindings/mtd/" in binding_path:
        return "nvm"

    if MMIO_SRAM_COMPAT in compats or MEMORY_REGION_COMPAT in compats:
        return "ram"

    if "/dts/bindings/sram/" in binding_path:
        return "ram"

    if any(hint in compat for compat in compats for hint in NVM_COMPAT_HINTS):
        return "nvm"

    if any(hint in compat for compat in compats for hint in RAM_COMPAT_HINTS):
        return "ram"

    return None


def _chosen_by_path(edt: Any) -> dict[str, list[str]]:
    chosen = getattr(edt, "chosen_nodes", {}) or {}
    if callable(chosen):
        chosen = chosen()

    ret: dict[str, list[str]] = {}
    for name, node in chosen.items():
        ret.setdefault(_node_path(node), []).append(name)
    return ret


def _partition_parent_paths(edt: Any) -> set[str]:
    paths = set()

    for node in getattr(edt, "nodes", []) or []:
        if not _node_is_partition(node):
            continue
        parent = _partition_memory_parent(node)
        if parent is not None:
            paths.add(_node_path(parent))

    return paths


def _segment_from_partition(node: Any) -> Segment | None:
    addr, size = _first_reg(node)
    if addr is None or size is None or size <= 0:
        return None

    parent = getattr(node, "parent", None)
    kind = "subpartition" if parent is not None and _node_is_partition(parent) else "partition"

    has_partition_children = any(_node_is_partition(child) for child in _node_children(node))
    if _has_compat(node, FIXED_SUBPARTITIONS_COMPAT) or has_partition_children:
        kind = "partition-group"

    segment = Segment(
        name=_node_display_name(node),
        path=_node_path(node),
        offset=addr,
        size=size,
        kind=kind,
        read_only=_node_read_only(node),
    )

    if kind == "partition-group":
        segment.children = _collect_partition_segments(node)

    return segment


def _collect_partition_segments(container: Any) -> list[Segment]:
    segments = []

    for child in _node_children(container):
        if _node_status(child) != "okay":
            continue
        if _node_is_partition(child):
            segment = _segment_from_partition(child)
            if segment is not None:
                segments.append(segment)
        elif _node_is_partition_container(child):
            segments.extend(_collect_partition_segments(child))

    return sorted(segments, key=lambda seg: (seg.offset, seg.size, seg.name))


def _partition_end(segments: list[Segment]) -> int:
    end = 0
    for segment in segments:
        end = max(end, segment.end, _partition_end(segment.children))
    return end


def _is_external(node: Any, kind: str) -> bool:
    on_buses = getattr(node, "on_buses", None) or []
    on_bus = getattr(node, "on_bus", None)
    compats = _node_compats(node)

    if on_buses or on_bus:
        return True

    if kind == "nvm" and SOC_NV_FLASH_COMPAT not in compats:
        return True

    return kind == "ram" and any(
        hint in compat for compat in compats for hint in ("psram", "sdram")
    )


def _bus_name(node: Any) -> str | None:
    on_bus = getattr(node, "on_bus", None)
    if on_bus:
        return str(on_bus)

    on_buses = getattr(node, "on_buses", None) or []
    if on_buses:
        return ", ".join(str(bus) for bus in on_buses)

    return None


def _make_memory(
    node: Any,
    kind: str,
    chosen_by_path: dict[str, list[str]],
) -> Memory | None:
    segments = _collect_partition_segments(node)

    addr, reg_size = _first_reg(node)
    prop_size = _capacity_from_props(node)
    partition_size = _partition_end(segments)
    size_candidates = [
        value for value in (reg_size, prop_size, partition_size)
        if value is not None
    ]
    if not size_candidates:
        return None

    size = max(size_candidates)
    if size <= 0:
        return None

    addressable = addr is not None and reg_size is not None and reg_size >= size
    base = int(addr or 0) if addressable else 0

    return Memory(
        name=_node_display_name(node),
        path=_node_path(node),
        kind=kind,
        base=base,
        size=int(size),
        addressable=addressable,
        external=_is_external(node, kind),
        bus=_bus_name(node),
        compats=_node_compats(node),
        chosen=chosen_by_path.get(_node_path(node), []),
        segments=segments,
    )


def _find_containing_memory(
    memories: list[Memory],
    addr: int,
    size: int,
    kind: str | None = None,
) -> Memory | None:
    candidates = []
    end = addr + size
    for memory in memories:
        if kind is not None and memory.kind != kind:
            continue
        if not memory.addressable:
            continue
        if memory.base <= addr and end <= memory.end:
            candidates.append(memory)

    if not candidates:
        return None

    return min(candidates, key=lambda memory: memory.size)


def _add_ram_region_segments(
    edt: Any,
    memories: list[Memory],
    chosen_by_path: dict[str, list[str]],
) -> None:
    compat2nodes = getattr(edt, "compat2nodes", {}) or {}
    for node in compat2nodes.get(MEMORY_REGION_COMPAT, []):
        if _node_status(node) != "okay":
            continue

        addr, size = _first_reg(node)
        if addr is None or size is None or size <= 0:
            continue

        memory = next((mem for mem in memories if mem.path == _node_path(node)), None)
        if memory is None:
            memory = _find_containing_memory(memories, addr, size)
        if memory is None:
            continue

        offset = 0 if memory.path == _node_path(node) else addr - memory.base
        segment = Segment(
            name=_node_display_name(node),
            path=_node_path(node),
            offset=offset,
            size=size,
            kind="region",
            read_only=_node_read_only(node),
            chosen=chosen_by_path.get(_node_path(node), []),
        )

        if all(existing.path != segment.path for existing in memory.segments):
            memory.segments.append(segment)


def _apply_chosen_to_segments(memories: list[Memory], chosen_by_path: dict[str, list[str]]) -> None:
    def apply(segment: Segment) -> None:
        segment.chosen = chosen_by_path.get(segment.path, [])
        for child in segment.children:
            apply(child)

    for memory in memories:
        memory.chosen = chosen_by_path.get(memory.path, [])
        for segment in memory.segments:
            apply(segment)


def collect_image_map(
    name: str,
    build_dir: Path,
    edt: Any,
    build_info: dict[str, Any] | None = None,
) -> ImageMap:
    chosen = _chosen_by_path(edt)
    partition_parent_paths = _partition_parent_paths(edt)

    memories_by_path: dict[str, Memory] = {}
    nodes = list(getattr(edt, "nodes", []) or [])

    for node in nodes:
        kind = _classify_memory_node(node, partition_parent_paths, chosen)
        if kind is None:
            continue

        memory = _make_memory(node, kind, chosen)
        if memory is not None:
            memories_by_path[memory.path] = memory

    memories = list(memories_by_path.values())
    for memory in memories:
        if MEMORY_REGION_COMPAT not in memory.compats or not memory.addressable:
            continue

        containing = _find_containing_memory(
            [candidate for candidate in memories if candidate.path != memory.path],
            memory.base,
            memory.size,
            kind="ram",
        )
        if containing is not None:
            del memories_by_path[memory.path]

    _add_ram_region_segments(edt, list(memories_by_path.values()), chosen)

    memories = sorted(
        memories_by_path.values(),
        key=lambda mem: (mem.kind, not mem.addressable, mem.base, mem.name),
    )
    for memory in memories:
        memory.segments.sort(key=lambda seg: (seg.offset, seg.size, seg.name))

    _apply_chosen_to_segments(memories, chosen)

    board = None
    if build_info:
        board_info = build_info.get("cmake", {}).get("board", {})
        board_name = board_info.get("name")
        board_qualifiers = board_info.get("qualifiers")
        if board_name and board_qualifiers:
            board = f"{board_name}/{board_qualifiers}"
        elif board_name:
            board = board_name

    warnings = []
    if not memories:
        warnings.append("No RAM or non-volatile memory devices were found in edt.pickle.")

    return ImageMap(
        name=name,
        build_dir=str(build_dir),
        board=board,
        memories=memories,
        warnings=warnings,
    )


def _load_yaml(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        data = yaml.safe_load(f) or {}
    return data


def _load_build_info(build_dir: Path) -> dict[str, Any] | None:
    path = build_dir / "build_info.yml"
    if not path.exists():
        return None
    return _load_yaml(path)


def _load_edt(build_dir: Path) -> Any:
    edt_pickle = build_dir / "zephyr" / "edt.pickle"
    if not edt_pickle.exists():
        raise FileNotFoundError(f"edt.pickle not found: {edt_pickle}")

    with edt_pickle.open("rb") as f:
        return pickle.load(f)


def _image_name_from_build_info(build_dir: Path, build_info: dict[str, Any] | None) -> str:
    if build_info:
        source_dir = build_info.get("cmake", {}).get("application", {}).get("source-dir")
        if source_dir:
            return Path(source_dir).name
    return build_dir.name or "application"


def discover_images(build_dir: Path) -> list[tuple[str, Path]]:
    domains_file = build_dir / "domains.yaml"
    if not domains_file.exists():
        build_info = _load_build_info(build_dir)
        return [(_image_name_from_build_info(build_dir, build_info), build_dir)]

    data = _load_yaml(domains_file)
    images = []
    for domain in data.get("domains", []):
        name = domain.get("name")
        image_build_dir = domain.get("build_dir")
        if name and image_build_dir:
            images.append((str(name), Path(image_build_dir)))

    if not images:
        raise ValueError(f"domains.yaml does not list any images: {domains_file}")

    return images


def collect_maps(build_dir: Path) -> list[ImageMap]:
    images = []
    for name, image_build_dir in discover_images(build_dir):
        build_info = _load_build_info(image_build_dir)
        edt = _load_edt(image_build_dir)
        images.append(collect_image_map(name, image_build_dir, edt, build_info))
    return images


def _format_size(size: int) -> str:
    units = ("B", "KiB", "MiB", "GiB")
    value = float(size)
    for unit in units:
        if value < 1024 or unit == units[-1]:
            if unit == "B":
                return f"{size} B"
            return f"{value:.1f} {unit}"
        value /= 1024
    return f"{size} B"


def _format_hex(value: int) -> str:
    width = max(8, ((value.bit_length() + 3) // 4))
    return f"0x{value:0{width}x}"


def _display_addr(memory: Memory, offset: int) -> str:
    if memory.addressable:
        return _format_hex(memory.base + offset)
    return _format_hex(offset)


def _segment_detail_items(segments: list[Segment], memory: Memory, depth: int = 0) -> list[str]:
    items = []
    for segment in segments:
        badges = "".join(
            f'<span class="pill">{_escape(chosen)}</span>' for chosen in segment.chosen
        )
        if segment.read_only:
            badges += '<span class="pill muted">read-only</span>'

        items.append(
            f'<section class="detail-item" style="--depth:{depth}">'
            '<div class="detail-title">'
            '<span class="tree-pad"></span>'
            f'<strong>{_escape(segment.name)}</strong>'
            f'<span class="detail-kind">{_escape(segment.kind)}</span>'
            '</div>'
            '<dl class="detail-grid">'
            "<div><dt>Start</dt><dd><code>"
            f"{_display_addr(memory, segment.offset)}</code></dd></div>"
            f'<div><dt>End</dt><dd><code>{_display_addr(memory, segment.end - 1)}</code></dd></div>'
            f'<div><dt>Size</dt><dd>{_format_size(segment.size)}</dd></div>'
            f'<div><dt>Chosen</dt><dd>{badges or "n/a"}</dd></div>'
            '<div class="detail-path"><dt>Path</dt><dd><code>'
            f"{_escape(segment.path)}</code></dd></div>"
            '</dl>'
            '</section>'
        )
        items.extend(_segment_detail_items(segment.children, memory, depth + 1))
    return items


def _layout_segments(memory: Memory) -> list[tuple[str, int, int, Segment | None]]:
    layout: list[tuple[str, int, int, Segment | None]] = []
    cursor = 0

    for segment in sorted(memory.segments, key=lambda seg: (seg.offset, seg.size, seg.name)):
        if segment.size <= 0:
            continue
        if segment.offset > cursor:
            layout.append(("gap", cursor, segment.offset - cursor, None))
            cursor = segment.offset

        start = max(segment.offset, 0)
        size = segment.size
        if start < cursor:
            size -= cursor - start
            start = cursor

        if size > 0:
            layout.append(("segment", start, size, segment))
            cursor = max(cursor, start + size)

    if memory.size > cursor:
        layout.append(("gap", cursor, memory.size - cursor, None))

    return layout


def _tooltip(memory: Memory, label: str, kind: str, offset: int, size: int, path: str = "") -> str:
    lines = [
        label,
        f"Type: {kind}",
        f"Range: {_display_addr(memory, offset)} - {_display_addr(memory, offset + size - 1)}",
        f"Size: {_format_size(size)}",
    ]
    if path:
        lines.append(f"Path: {path}")
    return "\n".join(lines)


def _render_stack_block(
    memory: Memory,
    kind: str,
    offset: int,
    size: int,
    segment: Segment | None,
) -> str:
    basis = size * 100 / memory.size
    if kind == "gap":
        label = "unused"
        css = "gap"
        block_kind = "unused"
        path = ""
    else:
        assert segment is not None
        label = segment.name
        css = f"seg {segment.kind}"
        block_kind = segment.kind
        path = segment.path

    tip = _tooltip(memory, label, block_kind, offset, size, path)
    meta = f"{_display_addr(memory, offset)} - {_format_size(size)}"
    compact = " compact" if basis < 8 else ""

    return (
        f'<div class="stack-block {css}{compact}" style="--basis:{basis:.6f}%" '
        f'data-tip="{_escape(tip)}" title="{_escape(tip)}" tabindex="0">'
        f'<span class="block-label">{_escape(label)}</span>'
        f'<span class="block-meta">{_escape(meta)}</span>'
        "</div>"
    )


def _render_stack(memory: Memory) -> str:
    if memory.size <= 0:
        return '<div class="memory-stack empty">No size information</div>'

    layout = _layout_segments(memory)
    parts = []
    for kind, offset, size, segment in layout:
        parts.append(_render_stack_block(memory, kind, offset, size, segment))

    stack_height = max(24.0, len(layout) * 1.55)
    stack_mobile_height = max(20.0, len(layout) * 1.55)
    top = memory.size - 1
    return (
        '<div class="memory-map">'
        '<div class="addr-rail">'
        f"<code>{_display_addr(memory, top)}</code>"
        "<span></span>"
        f"<code>{_display_addr(memory, 0)}</code>"
        "</div>"
        '<div class="memory-stack" '
        f'style="--stack-height:{stack_height:.2f}rem;'
        f'--stack-mobile-height:{stack_mobile_height:.2f}rem">'
        f'{"".join(reversed(parts))}</div>'
        "</div>"
    )


def _render_memory(memory: Memory) -> str:
    range_text = (
        f"{_format_hex(memory.base)} - {_format_hex(memory.end - 1)}"
        if memory.addressable
        else f"offset 0 - {_format_hex(memory.size - 1)}"
    )
    badges = [
        memory.kind.upper(),
        "external" if memory.external else "internal",
    ]
    if memory.bus:
        badges.append(memory.bus)
    badges.extend(memory.chosen)

    badge_html = "".join(f'<span class="pill">{_escape(badge)}</span>' for badge in badges)
    compat_html = ", ".join(f"<code>{_escape(compat)}</code>" for compat in memory.compats) or "n/a"
    details = "\n".join(_segment_detail_items(memory.segments, memory))

    if not details:
        details = '<p class="empty-cell">No partitions or memory regions were found.</p>'

    return f"""
<section class="memory-card">
  <div class="memory-head">
    <div>
      <h3>{_escape(memory.name)}</h3>
    </div>
    <div class="badges">{badge_html}</div>
  </div>
  <dl class="memory-facts">
    <div><dt>Range</dt><dd><code>{range_text}</code></dd></div>
    <div><dt>Size</dt><dd>{_format_size(memory.size)}</dd></div>
    <div><dt>Compatible</dt><dd>{compat_html}</dd></div>
  </dl>
  {_render_stack(memory)}
  <details class="partition-details">
    <summary>Details</summary>
    <div class="detail-list">
      <section class="detail-item">
        <div class="detail-title"><strong>{_escape(memory.name)}</strong></div>
        <dl class="detail-grid">
          <div><dt>Memory path</dt><dd><code>{_escape(memory.path)}</code></dd></div>
          <div><dt>Range</dt><dd><code>{range_text}</code></dd></div>
          <div><dt>Size</dt><dd>{_format_size(memory.size)}</dd></div>
          <div><dt>Compatible</dt><dd>{compat_html}</dd></div>
        </dl>
      </section>
      {details}
    </div>
  </details>
</section>
"""


def _render_image(image: ImageMap) -> str:
    board = f'<span class="pill">{_escape(image.board)}</span>' if image.board else ""
    warnings = "".join(f'<p class="warning">{_escape(warning)}</p>' for warning in image.warnings)
    memory_html = "\n".join(_render_memory(memory) for memory in image.memories)
    if not memory_html:
        memory_html = '<p class="empty-cell">No memory devices found.</p>'

    return f"""
<section id="image-{_escape(image.name)}" class="image-section">
  <div class="image-title">
    <div>
      <h2>{_escape(image.name)}</h2>
    </div>
    <div class="badges">{board}</div>
  </div>
  {warnings}
  {memory_html}
</section>
"""


def render_html(images: list[ImageMap]) -> str:
    nav = "\n".join(
        f'<a href="#image-{_escape(image.name)}">{_escape(image.name)}</a>' for image in images
    )
    sections = "\n".join(_render_image(image) for image in images)
    total_memories = sum(len(image.memories) for image in images)

    return f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Zephyr Partition Map</title>
  <style>
    :root {{
      --body-color: #404040;
      --content-wrap-background-color: #efefef;
      --content-background-color: #fcfcfc;
      --navbar-background-color: #333f67;
      --navbar-background-color-hover: #29355c;
      --navbar-current-background-color: #212d51;
      --navbar-level-1-color: #c3e3ff;
      --navbar-level-2-color: #b8d6f0;
      --navbar-heading-color: #af7fe4;
      --link-color: #1a5a8a;
      --link-color-hover: #2563a8;
      --hr-color: #e1e4e5;
      --code-background-color: #fff;
      --code-border-color: #e1e4e5;
      --code-literal-color: #c7254e;
      --stack-nvm: #1a5a8a;
      --stack-nvm-dark: #333f67;
      --stack-ram: #00695c;
      --stack-sub: #2c6b96;
      --stack-gap: #e8edf0;
    }}
    * {{ box-sizing: border-box; }}
    body {{
      margin: 0;
      color: var(--body-color);
      background: var(--content-wrap-background-color);
      font: 14px/1.65 system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
    }}
    code {{
      color: var(--code-literal-color);
      background: var(--code-background-color);
      border: 1px solid var(--code-border-color);
      border-radius: 3px;
      padding: 0.05rem 0.25rem;
      font-family: ui-monospace, SFMono-Regular, Consolas, monospace;
      font-size: 0.88em;
      overflow-wrap: anywhere;
      white-space: normal;
    }}
    .layout {{
      display: grid;
      grid-template-columns: 300px minmax(0, 1fr);
      min-height: 100vh;
    }}
    aside {{
      position: sticky;
      top: 0;
      height: 100vh;
      padding: 0;
      overflow: auto;
      color: var(--navbar-level-2-color);
      background: var(--navbar-background-color);
    }}
    .side-head {{
      padding: 1.25rem 1.35rem 1rem;
      background: var(--navbar-current-background-color);
      border-bottom: 1px solid rgba(255, 255, 255, 0.08);
    }}
    .side-head h1 {{
      margin: 0;
      color: #fff;
      font-size: 1.2rem;
      font-weight: 600;
      line-height: 1.2;
    }}
    .side-head p {{
      margin: 0.35rem 0 0;
      color: var(--navbar-level-1-color);
      font-size: 0.88rem;
    }}
    nav {{
      padding: 0.65rem 0;
    }}
    aside a {{
      display: block;
      padding: 0.48rem 1.35rem;
      color: var(--navbar-level-1-color);
      text-decoration: none;
      border-left: 4px solid transparent;
    }}
    aside a:hover {{
      color: #fff;
      background: var(--navbar-background-color-hover);
      border-left-color: var(--navbar-heading-color);
    }}
    main {{
      min-width: 0;
      width: min(980px, 100%);
      max-width: 100%;
      min-height: 100vh;
      padding: 1.75rem 2rem 3rem;
      background: var(--content-background-color);
      box-shadow: 0 0 0 1px rgba(0, 0, 0, 0.03);
    }}
    .hero {{
      margin-bottom: 1rem;
      padding-bottom: 1rem;
      border-bottom: 1px solid var(--hr-color);
    }}
    .hero h1 {{
      margin: 0;
      font-size: clamp(1.9rem, 4vw, 2.55rem);
      font-weight: 600;
      line-height: 1.1;
    }}
    .image-section {{
      margin: 0 0 2rem;
    }}
    .image-title, .memory-head {{
      display: flex;
      flex-wrap: wrap;
      gap: 1rem;
      align-items: flex-start;
      justify-content: space-between;
      min-width: 0;
    }}
    .image-title > div,
    .memory-head > div {{
      min-width: 0;
    }}
    .image-title h2, .memory-head h3 {{
      margin: 0;
      line-height: 1.2;
      font-weight: 600;
    }}
    .memory-card {{
      margin: 1rem 0 1.5rem;
      padding: 1rem 1rem 1.15rem;
      background: #fff;
      border: 1px solid var(--hr-color);
      border-radius: 2px;
      min-width: 0;
    }}
    .badges {{
      display: flex;
      flex-wrap: wrap;
      gap: 0.35rem;
      justify-content: flex-end;
      min-width: 0;
    }}
    .pill {{
      display: inline-flex;
      align-items: center;
      min-height: 1.35rem;
      padding: 0.1rem 0.45rem;
      color: var(--link-color);
      background: #e7f2fa;
      border: 1px solid #7fbbe3;
      border-radius: 3px;
      font-size: 0.78rem;
      font-weight: 600;
      white-space: nowrap;
    }}
    .pill.muted {{
      color: #5f3c13;
      background: #fff3d6;
      border-color: #f6d996;
    }}
    .memory-facts {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(min(100%, 13rem), 1fr));
      gap: 0.75rem;
      margin: 1rem 0;
    }}
    .memory-facts div {{
      min-width: 0;
      padding: 0.65rem;
      background: #f3f6f6;
      border: 1px solid var(--hr-color);
      border-radius: 2px;
    }}
    .memory-facts dt {{
      color: #666;
      font-size: 0.75rem;
      font-weight: 700;
      letter-spacing: 0;
      text-transform: uppercase;
    }}
    .memory-facts dd {{
      margin: 0.15rem 0 0;
      overflow-wrap: anywhere;
    }}
    .memory-map {{
      display: grid;
      grid-template-columns: max-content minmax(0, 1fr);
      gap: 0.8rem;
      align-items: stretch;
      width: min(100%, 34rem);
      max-width: 100%;
      margin: 1rem 0 0.9rem;
    }}
    .addr-rail {{
      display: grid;
      grid-template-rows: auto 1fr auto;
      gap: 0.35rem;
      justify-items: end;
      color: #747474;
      font-size: 0.82rem;
    }}
    .addr-rail span {{
      width: 1px;
      min-height: 18rem;
      background: var(--hr-color);
    }}
    .memory-stack {{
      display: flex;
      flex-direction: column;
      width: 100%;
      height: var(--stack-height);
      overflow: visible;
      background: #f3f6f6;
      border: 1px solid var(--code-border-color);
      border-radius: 2px;
      box-shadow: inset 0 0 0 1px rgba(255, 255, 255, 0.65);
    }}
    .stack-block {{
      position: relative;
      flex: 0 1 max(var(--basis), 1.45rem);
      min-height: 1.35rem;
      padding: 0.2rem 0.45rem;
      border-bottom: 1px solid rgba(255, 255, 255, 0.85);
      outline: 0;
      cursor: default;
      overflow: visible;
    }}
    .stack-block:hover,
    .stack-block:focus {{
      z-index: 5;
      filter: brightness(1.08);
      box-shadow: 0 0 0 2px var(--navbar-heading-color);
    }}
    .stack-block::after {{
      position: absolute;
      right: 0.5rem;
      top: 50%;
      z-index: 20;
      display: none;
      width: min(28rem, 55vw);
      padding: 0.65rem 0.75rem;
      color: #fff;
      background: #1f2933;
      border-radius: 3px;
      box-shadow: 0 0.45rem 1rem rgba(0, 0, 0, 0.25);
      content: attr(data-tip);
      font-family: ui-monospace, SFMono-Regular, Consolas, monospace;
      font-size: 0.78rem;
      line-height: 1.45;
      transform: translateY(-50%);
      overflow-wrap: anywhere;
      white-space: pre-wrap;
    }}
    .stack-block:hover::after,
    .stack-block:focus::after {{
      display: block;
    }}
    .block-label,
    .block-meta {{
      display: block;
      overflow: hidden;
      max-width: 100%;
      color: #fff;
      line-height: 1.2;
      text-overflow: ellipsis;
      white-space: nowrap;
    }}
    .block-label {{
      font-weight: 700;
    }}
    .block-meta {{
      opacity: 0.86;
      font-size: 0.75rem;
    }}
    .compact .block-meta {{
      display: none;
    }}
    .partition {{ background: var(--stack-nvm); }}
    .partition-group {{ background: var(--stack-nvm-dark); }}
    .subpartition {{ background: var(--stack-sub); }}
    .region {{ background: var(--stack-ram); }}
    .gap {{
      background:
        repeating-linear-gradient(
          135deg,
          #f6f8f8,
          #f6f8f8 7px,
          var(--stack-gap) 7px,
          var(--stack-gap) 14px
        );
    }}
    .gap .block-label,
    .gap .block-meta {{
      color: #667;
      font-weight: 600;
    }}
    .partition-details {{
      margin-top: 0.8rem;
    }}
    .partition-details summary {{
      width: max-content;
      max-width: 100%;
      color: var(--link-color);
      cursor: pointer;
      font-weight: 600;
    }}
    .detail-list {{
      margin-top: 0.6rem;
      display: grid;
      gap: 0.6rem;
    }}
    .detail-item {{
      min-width: 0;
      padding: 0.65rem;
      background: #fff;
      border: 1px solid var(--code-border-color);
      border-radius: 2px;
    }}
    .detail-item:nth-child(odd) {{
      background: #f3f6f6;
    }}
    .detail-title {{
      display: flex;
      flex-wrap: wrap;
      gap: 0.4rem;
      align-items: center;
      min-width: 0;
      overflow-wrap: anywhere;
    }}
    .detail-kind {{
      color: #666;
      font-size: 0.75rem;
      font-weight: 700;
    }}
    .detail-grid {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(min(100%, 9.5rem), 1fr));
      gap: 0.45rem 0.75rem;
      margin: 0.45rem 0 0;
    }}
    .detail-grid div {{
      min-width: 0;
    }}
    .detail-grid dt {{
      color: #666;
      font-size: 0.72rem;
      font-weight: 700;
      letter-spacing: 0;
      text-transform: uppercase;
    }}
    .detail-grid dd {{
      margin: 0.1rem 0 0;
      min-width: 0;
      overflow-wrap: anywhere;
    }}
    .detail-path {{
      grid-column: 1 / -1;
    }}
    .tree-pad {{
      display: inline-block;
      width: calc(var(--depth) * 1rem);
      flex: 0 0 auto;
    }}
    .empty-cell, .warning {{
      color: #747474;
    }}
    .warning {{
      padding: 0.6rem 0.8rem;
      background: #fff7e8;
      border: 1px solid #f3d19c;
      border-radius: 4px;
    }}
    @media (max-width: 880px) {{
      .layout {{
        display: block;
      }}
      aside {{
        position: static;
        height: auto;
      }}
      main {{
        padding: 1rem;
      }}
      .image-title, .memory-head {{
        display: block;
      }}
      .badges {{
        justify-content: flex-start;
        margin-top: 0.5rem;
      }}
      .memory-facts {{
        grid-template-columns: 1fr;
      }}
      .memory-map {{
        grid-template-columns: 1fr;
        width: 100%;
      }}
      .addr-rail {{
        display: none;
      }}
      .memory-stack {{
        height: var(--stack-mobile-height);
      }}
      .stack-block::after {{
        left: auto;
        right: 0;
        top: auto;
        bottom: calc(100% + 0.45rem);
        width: min(24rem, 90vw);
        transform: none;
      }}
      .detail-grid {{
        grid-template-columns: 1fr;
      }}
    }}
  </style>
</head>
<body>
  <div class="layout">
    <aside>
      <div class="side-head">
        <h1>Zephyr Project</h1>
        <p>Partition Map</p>
        <p>{len(images)} image(s), {total_memories} memory device(s)</p>
      </div>
      <nav>{nav}</nav>
    </aside>
    <main>
      <section class="hero">
        <h1>Zephyr Partition Map</h1>
      </section>
      {sections}
    </main>
  </div>
</body>
</html>
"""


def write_site(images: list[ImageMap], output_dir: Path) -> Path:
    output_dir.mkdir(parents=True, exist_ok=True)
    index = output_dir / "index.html"
    data = output_dir / "partition_map.json"

    index.write_text(render_html(images), encoding="utf-8")
    data.write_text(json.dumps([asdict(image) for image in images], indent=2), encoding="utf-8")

    return index


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=Path.cwd(),
        help="Zephyr build directory, or a sysbuild directory containing domains.yaml",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="Output directory for the generated site. Defaults to <build-dir>/partition_map",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    build_dir = args.build_dir.resolve()
    output = (args.output or (build_dir / "partition_map")).resolve()

    images = collect_maps(build_dir)
    index = write_site(images, output)
    print(f"Partition map written to {index}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

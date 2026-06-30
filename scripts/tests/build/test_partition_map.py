# SPDX-License-Identifier: Apache-2.0

import sys
from collections import defaultdict
from pathlib import Path

ZEPHYR_BASE = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ZEPHYR_BASE))

import scripts.build.partition_map as partition_map  # noqa: E402


class Prop:
    def __init__(self, val):
        self.val = val


class Reg:
    def __init__(self, addr, size):
        self.addr = addr
        self.size = size


class Node:
    def __init__(
        self,
        path,
        compats=(),
        regs=(),
        props=None,
        binding_path="",
        labels=(),
        status="okay",
        on_bus=None,
        on_buses=None,
    ):
        self.path = path
        self.name = path.rpartition("/")[2]
        self.compats = list(compats)
        self.regs = [Reg(*reg) for reg in regs]
        self.props = {name: Prop(val) for name, val in (props or {}).items()}
        self.binding_path = binding_path
        self.labels = list(labels)
        self.status = status
        self.on_bus = on_bus
        self.on_buses = list(on_buses or [])
        self.parent = None
        self.children = {}
        self.read_only = "read-only" in self.props

    def add(self, child):
        child.parent = self
        self.children[child.name] = child
        return child


class EDT:
    def __init__(self, roots, chosen=None):
        self.nodes = []
        self.compat2nodes = defaultdict(list)
        self.chosen_nodes = chosen or {}

        def visit(node):
            self.nodes.append(node)
            for compat in node.compats:
                self.compat2nodes[compat].append(node)
            for child in node.children.values():
                visit(child)

        for root in roots:
            visit(root)


def test_collects_internal_flash_partitions_and_subpartitions():
    flash = Node(
        "/soc/flash@0",
        [partition_map.SOC_NV_FLASH_COMPAT],
        regs=[(0, 0x100000)],
        binding_path="/zephyr/dts/bindings/mtd/soc-nv-flash.yaml",
    )
    partitions = flash.add(Node("/soc/flash@0/partitions", [partition_map.FIXED_PARTITIONS_COMPAT]))
    boot = partitions.add(
        Node(
            "/soc/flash@0/partitions/partition@0",
            regs=[(0x0, 0x10000)],
            props={"label": "mcuboot", "read-only": True},
        )
    )
    slot0 = partitions.add(
        Node(
            "/soc/flash@0/partitions/partition@10000",
            [partition_map.FIXED_SUBPARTITIONS_COMPAT],
            regs=[(0x10000, 0x70000)],
            props={"label": "image-0"},
        )
    )
    slot0.add(
        Node(
            "/soc/flash@0/partitions/partition@10000/partition@0",
            regs=[(0x10000, 0x40000)],
            props={"label": "image-0-secure"},
        )
    )
    slot0.add(
        Node(
            "/soc/flash@0/partitions/partition@10000/partition@40000",
            regs=[(0x50000, 0x30000)],
            props={"label": "image-0-nonsecure"},
        )
    )

    edt = EDT([flash], chosen={"zephyr,flash": flash, "zephyr,code-partition": slot0})

    image = partition_map.collect_image_map("app", Path("build/app"), edt)

    assert len(image.memories) == 1
    memory = image.memories[0]
    assert memory.kind == "nvm"
    assert memory.addressable
    assert memory.chosen == ["zephyr,flash"]
    assert [segment.name for segment in memory.segments] == ["mcuboot", "image-0"]
    assert memory.segments[0].read_only
    assert memory.segments[1].chosen == ["zephyr,code-partition"]
    assert [child.name for child in memory.segments[1].children] == [
        "image-0-secure",
        "image-0-nonsecure",
    ]
    assert boot.path in {segment.path for segment in memory.segments}


def test_collects_external_spi_nor_from_size_property():
    spi_nor = Node(
        "/soc/spi@40000000/flash@0",
        ["jedec,spi-nor"],
        regs=[(0, None)],
        props={"size": 8 * 1024 * 1024},
        binding_path="/zephyr/dts/bindings/mtd/jedec,spi-nor.yaml",
        on_bus="spi",
    )
    partitions = spi_nor.add(
        Node(
            "/soc/spi@40000000/flash@0/partitions",
            [partition_map.FIXED_PARTITIONS_COMPAT],
        )
    )
    partitions.add(
        Node(
            "/soc/spi@40000000/flash@0/partitions/partition@0",
            regs=[(0x0, 0x20000)],
            props={"label": "storage"},
        )
    )

    image = partition_map.collect_image_map("net_core", Path("build/net_core"), EDT([spi_nor]))

    memory = image.memories[0]
    assert memory.kind == "nvm"
    assert memory.external
    assert not memory.addressable
    assert memory.bus == "spi"
    assert memory.size == 1024 * 1024
    assert memory.segments[0].name == "storage"


def test_collects_mapped_partitions_as_nvm_segments():
    flash = Node(
        "/soc/flash-controller@4001e000/flash@0",
        [partition_map.SOC_NV_FLASH_COMPAT],
        regs=[(0, 0x100000)],
        binding_path="/zephyr/dts/bindings/mtd/soc-nv-flash.yaml",
    )
    partitions = flash.add(Node("/soc/flash-controller@4001e000/flash@0/partitions"))
    partitions.add(
        Node(
            "/soc/flash-controller@4001e000/flash@0/partitions/partition@0",
            [partition_map.MAPPED_PARTITION_COMPAT],
            regs=[(0x0, 0xC000)],
            props={"label": "mcuboot"},
            binding_path="/zephyr/dts/bindings/mtd/zephyr,mapped-partition.yaml",
        )
    )
    partitions.add(
        Node(
            "/soc/flash-controller@4001e000/flash@0/partitions/partition@c000",
            [partition_map.MAPPED_PARTITION_COMPAT],
            regs=[(0xC000, 0x76000)],
            props={"label": "image-0"},
            binding_path="/zephyr/dts/bindings/mtd/zephyr,mapped-partition.yaml",
        )
    )

    image = partition_map.collect_image_map("app", Path("build/app"), EDT([flash]))

    assert len(image.memories) == 1
    assert image.memories[0].path == flash.path
    assert [segment.name for segment in image.memories[0].segments] == ["mcuboot", "image-0"]


def test_collects_ram_regions_inside_parent_ram():
    sram = Node(
        "/sram@20000000",
        [partition_map.MMIO_SRAM_COMPAT],
        regs=[(0x20000000, 0x40000)],
        binding_path="/zephyr/dts/bindings/sram/mmio-sram.yaml",
    )
    sram.add(
        Node(
            "/sram@20000000/dma@20001000",
            [partition_map.MEMORY_REGION_COMPAT],
            regs=[(0x20001000, 0x1000)],
            props={"zephyr,memory-region": "DMA_RAM"},
        )
    )

    image = partition_map.collect_image_map(
        "app",
        Path("build/app"),
        EDT([sram], chosen={"zephyr,sram": sram}),
    )

    assert len(image.memories) == 1
    memory = image.memories[0]
    assert memory.kind == "ram"
    assert memory.chosen == ["zephyr,sram"]
    assert memory.segments[0].name == "DMA_RAM"
    assert memory.segments[0].offset == 0x1000
    assert memory.segments[0].size == 0x1000


def test_collects_external_ram_memory_region():
    sdram = Node(
        "/soc/fmc@a0000000/sdram@c0000000",
        [partition_map.MEMORY_REGION_COMPAT, "vnd,sdram"],
        regs=[(0xC0000000, 0x800000)],
        props={"zephyr,memory-region": "SDRAM"},
    )

    image = partition_map.collect_image_map("app", Path("build/app"), EDT([sdram]))

    assert len(image.memories) == 1
    memory = image.memories[0]
    assert memory.kind == "ram"
    assert memory.external
    assert memory.name == "SDRAM"
    assert memory.segments[0].name == "SDRAM"
    assert memory.segments[0].size == 0x800000


def test_ignores_flash_controller_register_blocks():
    controller = Node(
        "/soc/flash-controller@400fd000",
        ["ti,stellaris-flash-controller"],
        regs=[(0x400FD000, 0x1000)],
        binding_path="/zephyr/dts/bindings/flash_controller/ti,stellaris-flash-controller.yaml",
    )

    image = partition_map.collect_image_map("app", Path("build/app"), EDT([controller]))

    assert image.memories == []
    assert image.warnings == ["No RAM or non-volatile memory devices were found in edt.pickle."]


def test_write_site_outputs_html_and_json(tmp_path):
    memory = partition_map.Memory(
        name="flash0",
        path="/soc/flash@0",
        kind="nvm",
        base=0,
        size=0x20000,
        addressable=True,
        external=False,
        compats=[partition_map.SOC_NV_FLASH_COMPAT],
        segments=[
            partition_map.Segment(
                name="mcuboot",
                path="/soc/flash@0/partitions/partition@0",
                offset=0,
                size=0x10000,
                kind="partition",
            )
        ],
    )
    image = partition_map.ImageMap(
        name="app",
        build_dir="build/app",
        board="qemu_cortex_m3",
        memories=[memory],
    )

    index = partition_map.write_site([image], tmp_path)

    assert index == tmp_path / "index.html"
    assert "Zephyr Partition Map" in index.read_text(encoding="utf-8")
    assert "mcuboot" in index.read_text(encoding="utf-8")
    assert (tmp_path / "partition_map.json").exists()

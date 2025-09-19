"""
Copyright (c) 2025 Nordic Semiconductor ASA
SPDX-License-Identifier: Apache-2.0
"""

from __future__ import annotations

import enum
import logging
import re
from collections.abc import Iterable, Iterator, Mapping
from dataclasses import dataclass
from functools import cached_property
from itertools import islice
from typing import (
    Any,
    Literal,
    NamedTuple,
)

# edtlib classes used in this module.
# Defined this way as placeholders so type checking for these could be added later.
EDT = Any
Node = Any
PinCtrl = Any
Property = Any

# Status values that indicate ownership either by the processor or a child processor
STATUS_OWNED = ("okay", "reserved")

# Compatibles of global peripherals that should not be assigned to current core.
SKIP_SPU_PERIPH_PERM_COMPATS = {
    "nordic,nrf-bellboard-tx",
    "nordic,nrf-bellboard-rx",
    "nordic,nrf-clic",
    "nordic,nrf-dppic-global",
    "nordic,nrf-gpio",
    "nordic,nrf-gpiote",
    "nordic,nrf-grtc",
    "nordic,nrf-ipct-global",
    "nordic,nrf-temp",
    "nordic,nrf-vevif-task-tx",
    "nordic,nrf-vevif-task-rx",
}

# Compatibles of global peripherals that should be assigned to the current core but do not have DMA
NO_DMA_COMPATS = {
    "nordic,nrf-auxpll",
    "nordic,nrf-comp",
    "nordic,nrf-egu",
    "nordic,nrf-exmif",
    "nordic,nrf-lpcomp",
    "nordic,nrf-qdec",
    "nordic,nrf-timer",
    "nordic,nrf-rtc",
    "nordic,nrf-wdt",
    # VPRs have a PERM DMASEC setting, but PPR applications currently do not work as intended when
    # setting it to secure.
    "nordic,nrf-vpr-coprocessor",
}


class BuilderError(RuntimeError):
    """Generic runtime error raised by the PeriphconfBuilder"""

    ...


class BadDevicetreeError(BuilderError):
    """Error raised when the devicetree is misconfigured
    or if assumptions about the devicetree are not met.
    """

    ...


def _init_logger() -> logging.Logger:
    formatter = logging.Formatter("{message}", style="{")
    handler = logging.StreamHandler()
    handler.setFormatter(formatter)

    logger = logging.getLogger("periphconf")
    logger.setLevel(logging.ERROR)
    logger.addHandler(handler)

    return logger


# Logger for the script
log = _init_logger()


class PeriphconfBuilder:
    def __init__(
        self,
        dt: EDT,
        lookup_tables: SocLookupTables,
    ) -> None:
        """Builder class used to generate a PERIPHCONF C source file based on the devicetree.

        :param dt: Devicetree object.
        :param lookup_tables: Lookup table object containing soc-specific information.
        """

        self._dt = dt
        self._hw_tables = lookup_tables
        self._processor_id = dt_processor_id(dt)
        self._owner_id = self._processor_id.default_owner_id

        self._macros = []
        self._ipcmap_idx = 0
        self._generated_ipcmaps = set()

        self._nodelabel_lookup = {
            dt_reg_addr(node): node.labels[0] for node in dt.nodes if node.regs and node.labels
        }

    def build_generated_source(self, header_line: str | None = None) -> str:
        """Generate a C source file containing all the macros generated through
        calls to the API.
        """
        source_lines = []

        if header_line:
            source_lines.extend([f"/* {header_line} */", ""])

        source_lines.extend(
            [
                "#include <zephyr/devicetree.h>",
                "#include <uicr/uicr.h>",
                "",
            ]
        )

        for macro in sorted(self._macros):
            source_lines.append(macro.c_render(self._nodelabel_lookup))

        return "\n".join(source_lines)

    def add_local_peripheral_cfg(
        self,
        node_or_nodelabel: Node | str,
        /,
    ) -> None:
        """Generate macros for populating PERIPHCONF based on the properties of a local domain
        devicetree node.

        :param node_or_nodelabel: node or label of the node to process.
        """
        self._add_peripheral_cfg(node_or_nodelabel, is_global=False, add_gpios=False)

    def add_global_peripheral_cfg(
        self,
        node_or_nodelabel: Node | str,
        /,
        *,
        has_irq_mapping: bool = True,
        add_ppib_channel_links: bool = True,
    ) -> None:
        """Generate macros for populating PERIPHCONF based on the properties of the given
        devicetree node located in a global domain.

        :param node_or_nodelabel: node or label of the node to process.
        :param has_irq_mapping: configure IRQMAP to map the peripheral IRQs to the processor the
            devicetree belongs to, if interrupts are specified on the node.
        :param add_ppib_channel_links: add PPIB connections based on the channel link properties of
            the node.
        """
        self._add_peripheral_cfg(
            node_or_nodelabel,
            is_global=True,
            has_irq_mapping=has_irq_mapping,
            add_ppib_channel_links=add_ppib_channel_links,
        )

    def _add_peripheral_cfg(
        self,
        node_or_nodelabel: Node | str,
        is_global: bool = True,
        has_irq_mapping: bool = True,
        add_ppib_channel_links: bool = True,
        add_gpios: bool = True,
    ) -> None:
        if isinstance(node_or_nodelabel, str):
            node = self._dt.label2node[node_or_nodelabel]
        else:
            node = node_or_nodelabel

        if node.status not in STATUS_OWNED:
            return

        if is_global:
            has_skip_spu_periph_perm_compat = any(
                compat in SKIP_SPU_PERIPH_PERM_COMPATS for compat in node.compats
            )
            if not has_skip_spu_periph_perm_compat:
                has_no_dma_compat = any(compat in NO_DMA_COMPATS for compat in node.compats)
                self._add_global_peripheral_spu_permissions(node, has_dma=not has_no_dma_compat)
                if has_irq_mapping:
                    self._add_global_peripheral_irq_mapping(node)

            if "nordic,nrf-gpiote" in node.compats:
                self._add_nrf_gpiote_spu_permissions(node)

            if "nordic,nrf-dppic-global" in node.compats:
                self._add_nrf_dppic_spu_permissions(node)
                if add_ppib_channel_links:
                    self._add_nrf_dppic_ppib_links(node)

            if "nordic,nrf-ipct-global" in node.compats:
                self._add_nrf_ipct_global_spu_permissions(node)
                self._add_nrf_ipct_ipcmap_channel_links(node)

            if "nordic,nrf-grtc" in node.compats:
                self._add_nrf_grtc_spu_permissions(node)

            if "nordic,nrf-saadc" in node.compats:
                self._add_nrf_saadc_channel_pin_spu_permissions(node)

            if "nordic,nrf-comp" in node.compats or "nordic,nrf-lpcomp" in node.compats:
                self._add_nrf_comp_lpcomp_channel_pin_spu_permissions(node)

            self._add_peripheral_pinctrls_spu_permissions_and_ctrlsel(node)

        if "nordic,nrf-ipct-local" in node.compats:
            self._add_nrf_ipct_ipcmap_channel_links(node)

        if add_gpios:
            self._add_peripheral_gpios_spu_permissions_and_ctrlsel(node)

    def add_gpio_spu_permissions(self, node_or_nodelabel: Node | str, /) -> None:
        """Generate macros for populating SPU FEATURE.GPIO[n].PIN[m] registers based on
        gpios properties on the given devicetree node.

        CTRLSEL is not set for these properties; it is assumed that the default CTRLSEL
        value (GPIO) is the correct CTRLSEL for these pins.

        :param node_or_nodelabel: node or label of the node to process.
        """
        if isinstance(node_or_nodelabel, str):
            node = self._dt.label2node[node_or_nodelabel]
        else:
            node = node_or_nodelabel

        if node.status not in STATUS_OWNED:
            return

        self._add_peripheral_gpios_spu_permissions_and_ctrlsel(node, skip_ctrlsel=True)

    def _add_global_peripheral_spu_permissions(
        self,
        node: Node,
        has_dma: bool = True,
    ) -> None:
        """Adds an SPU PERIPH[n].PERM entry for the peripheral described by the node."""
        addr = dt_reg_addr(node)
        secure = dt_node_is_secure(node)
        if has_dma:
            dma_secure = secure
        else:
            # If there is no DMA then the DMASEC field is not writable and reads as zero.
            # Set to zero to avoid readback errors.
            dma_secure = False
        periph_label = node.labels[0]

        spu_address = get_spu_addr_for_periph(addr)
        spu_name = self._hw_tables.spu_addr_to_name[spu_address]
        parsed_addr = Address(addr)
        periph_slave_index = parsed_addr.slave_index

        self._macros.append(
            MacroCall(
                "UICR_SPU_PERIPH_PERM_SET",
                [
                    Address(spu_address),
                    periph_slave_index,
                    secure,
                    dma_secure,
                    self._owner_id.c_enum,
                ],
                comment=f"{spu_name}: {periph_label} permissions",
            )
        )

    def _add_global_peripheral_irq_mapping(self, node: Node) -> None:
        """Adds an IRQMAP[n].SINK entry for each interrupt on the
        peripheral described by the node.
        """
        periph_identifier = dt_node_identifier(node)
        get_irqn_from_dt = bool(node.labels)

        for i, interrupt in enumerate(node.interrupts):
            interrupt_processors = set()
            interrupt_ctrl = interrupt.controller

            interrupt_processors.update(dt_node_processors_from_labels(interrupt_ctrl))

            if not interrupt_processors:
                interrupt_processors.update(dt_node_processors_from_labels(node))

            if not interrupt_processors:
                raise BuilderError(
                    f"No unique processor ID could be found based on interrupt controllers "
                    f"or nodelabels for peripheral node {node.path}"
                )
            if len(interrupt_processors) > 1:
                raise BuilderError(
                    f"Peripheral node {node.path} corresponds to multiple processors "
                    f"({interrupt_processors}), which is not supported."
                )

            irq_processor = next(iter(interrupt_processors))

            if get_irqn_from_dt:
                macro_irqn = f"DT_IRQN_BY_IDX(DT_NODELABEL({node.labels[0]}), {i})"
            else:
                macro_irqn = str(interrupt.data["irq"])

            self._macros.append(
                MacroCall(
                    "UICR_IRQMAP_IRQ_SINK_SET",
                    [
                        macro_irqn,
                        irq_processor.c_enum,
                    ],
                    comment=f"{periph_identifier} IRQ => {irq_processor.name}",
                )
            )

    def _add_nrf_gpiote_spu_permissions(self, node: Node) -> None:
        """Adds SPU FEATURE.GPIOTE[n].CH[m] entries for configured channels on the
        GPIOTE peripheral described by the node.
        """
        addr = dt_reg_addr(node)
        instance_name = dt_node_identifier(node)
        spu_address = get_spu_addr_for_periph(addr)
        spu_name = self._hw_tables.spu_addr_to_name[spu_address]

        for num, secure in dt_split_channels_get(node):
            self._macros.append(
                MacroCall(
                    "UICR_SPU_FEATURE_GPIOTE_CH_SET",
                    [
                        Address(spu_address),
                        0,
                        num,
                        secure,
                        self._owner_id.c_enum,
                    ],
                    comment=f"{spu_name}: {instance_name} ch. {num} permissions",
                )
            )

    def _add_nrf_dppic_spu_permissions(self, node: Node) -> None:
        """Adds SPU FEATURE.DPPIC[n].CH[m] and SPU FEATURE.DPPIC[n].CHG[m] entries for
        configured channels and channel groups on the DPPIC peripheral described by the node.
        """
        addr = dt_reg_addr(node)
        channels = dt_split_channels_get(node)
        channel_groups = dt_split_channels_get(
            node,
            owned_name="owned-channel-groups",
            nonsecure_name="nonsecure-channel-groups",
        )

        instance_name = dt_node_identifier(node)
        spu_address = get_spu_addr_for_periph(addr)
        spu_name = self._hw_tables.spu_addr_to_name[spu_address]

        for num, secure in channels:
            self._macros.append(
                MacroCall(
                    "UICR_SPU_FEATURE_DPPIC_CH_SET",
                    [
                        Address(spu_address),
                        num,
                        secure,
                        self._owner_id.c_enum,
                    ],
                    comment=f"{spu_name}: {instance_name} ch. {num} permissions",
                )
            )

        for num, secure in channel_groups:
            self._macros.append(
                MacroCall(
                    "UICR_SPU_FEATURE_DPPIC_CHG_SET",
                    [
                        Address(spu_address),
                        num,
                        secure,
                        self._owner_id.c_enum,
                    ],
                    comment=f"{spu_name}: {instance_name} ch. group {num} permissions",
                )
            )

    def _add_nrf_dppic_ppib_links(self, node: Node) -> None:
        """Adds PPIB SUBSCRIBE_SEND[n] and PPIB PUBLISH_RECEIVE[n] entries for
        configured "source" and "sink" channels on the DPPIC peripheral described by the node.
        """
        addr = dt_reg_addr(node, secure=True)
        source_channels = dt_array_prop(node, "source-channels", [])
        sink_channels = dt_array_prop(node, "sink-channels", [])

        for num in source_channels:
            self._link_dppi_channels(addr, num, direction="source")

        for num in sink_channels:
            self._link_dppi_channels(addr, num, direction="sink")

    def _link_dppi_channels(
        self,
        dppic_addr: int,
        channel_num: int,
        direction: Literal["source"] | Literal["sink"],
    ) -> None:
        local_ppib_addr, local_ppib_ch_map = self._hw_tables.dppic_to_ppib_connections[dppic_addr]
        local_ppib_ch = local_ppib_ch_map[channel_num]

        remote_ppib_addr, remote_ppib_ch_map = self._hw_tables.ppib_to_ppib_connections[
            local_ppib_addr
        ]
        remote_ppib_ch = remote_ppib_ch_map[local_ppib_ch]

        if direction == "source":
            sub_ppib_addr, sub_ppib_ch = local_ppib_addr, local_ppib_ch
            pub_ppib_addr, pub_ppib_ch = remote_ppib_addr, remote_ppib_ch
        else:
            sub_ppib_addr, sub_ppib_ch = remote_ppib_addr, remote_ppib_ch
            pub_ppib_addr, pub_ppib_ch = local_ppib_addr, local_ppib_ch

        sub_ppib_name = self._hw_tables.ppib_addr_to_name[sub_ppib_addr]
        pub_ppib_name = self._hw_tables.ppib_addr_to_name[pub_ppib_addr]

        self._macros.append(
            MacroCall(
                "UICR_PPIB_SUBSCRIBE_SEND_ENABLE",
                [
                    Address(sub_ppib_addr),
                    sub_ppib_ch,
                ],
                comment=(
                    f"SUB: {sub_ppib_name} ch. {sub_ppib_ch} => {pub_ppib_name} ch. {pub_ppib_ch}"
                ),
            )
        )
        self._macros.append(
            MacroCall(
                "UICR_PPIB_PUBLISH_RECEIVE_ENABLE",
                [
                    Address(pub_ppib_addr),
                    pub_ppib_ch,
                ],
                comment=(
                    f"PUB: {sub_ppib_name} ch. {sub_ppib_ch} => {pub_ppib_name} ch. {pub_ppib_ch}"
                ),
            )
        )

    def _add_nrf_ipct_global_spu_permissions(self, node: Node) -> None:
        """Adds SPU FEATURE.IPCT[n].CH[m] entries for configured channels
        on the IPCT peripheral described by the node.
        """
        addr = dt_reg_addr(node)
        instance_name = dt_node_identifier(node)
        spu_address = get_spu_addr_for_periph(addr)
        spu_name = self._hw_tables.spu_addr_to_name[spu_address]

        for num, secure in dt_split_channels_get(node):
            self._macros.append(
                MacroCall(
                    "UICR_SPU_FEATURE_IPCT_CH_SET",
                    [
                        Address(spu_address),
                        num,
                        secure,
                        self._owner_id.c_enum,
                    ],
                    comment=f"{spu_name}: {instance_name} ch. {num} permissions",
                )
            )

    def _add_nrf_ipct_ipcmap_channel_links(self, node: Node) -> None:
        """Adds IPCMAP CHANNEL[n].SOURCE and CHANNEL[n].SINK entries for "sink" and "source"
        channels on the IPCT peripheral described by the node.
        """
        source_channel_links = dt_array_prop(node, "source-channel-links", [])
        if len(source_channel_links) % 3 != 0:
            raise BadDevicetreeError()

        sink_channel_links = dt_array_prop(node, "sink-channel-links", [])
        if len(sink_channel_links) % 3 != 0:
            raise BadDevicetreeError()

        node_domain_raw = dt_prop(node, "global-domain-id", None)
        if node_domain_raw is None:
            addr = dt_reg_addr(node)
            try:
                node_domain = Address(addr).domain
            except ValueError as e:
                raise BadDevicetreeError(
                    f"Failed to determine domain ID for address 0x{addr:08x} "
                    f"specified on node {node.path}"
                ) from e
        else:
            node_domain = DomainId(node_domain_raw)

        for source_ch, sink_domain_raw, sink_ch in batched(source_channel_links, 3):
            sink_domain = DomainId(sink_domain_raw)
            self._link_ipct_channel(node_domain, source_ch, sink_domain, sink_ch)

        for sink_ch, source_domain_raw, source_ch in batched(sink_channel_links, 3):
            source_domain = DomainId(source_domain_raw)
            self._link_ipct_channel(source_domain, source_ch, node_domain, sink_ch)

    def _link_ipct_channel(
        self,
        source_domain: DomainId,
        source_ch: int,
        sink_domain: DomainId,
        sink_ch: int,
    ) -> None:
        # Setting "source-channel-links" on one end and "sink-channel-links" on the other
        # leads to duplicated entries without this check.
        link_args = (
            source_domain.c_enum,
            source_ch,
            sink_domain.c_enum,
            sink_ch,
        )
        if link_args in self._generated_ipcmaps:
            log.debug(f"Skip duplicate IPCMAP entry: {link_args}")
            return
        self._generated_ipcmaps.add(link_args)

        self._macros.append(
            MacroCall(
                "UICR_IPCMAP_CHANNEL_CFG",
                [self._ipcmap_idx, *link_args],
                comment=(
                    f"{source_domain.name} IPCT ch. {source_ch} => "
                    f"{sink_domain.name} IPCT ch. {sink_ch}"
                ),
            )
        )
        self._ipcmap_idx += 1

    def _add_nrf_grtc_spu_permissions(self, node: Node) -> None:
        """Adds SPU FEATURE.GRTC[n].CC[m] entries for configured channels
        on the GRTC peripheral described by the node.
        """
        grtc_addr = dt_reg_addr(node)
        spu_address = get_spu_addr_for_periph(grtc_addr)
        spu_name = self._hw_tables.spu_addr_to_name[spu_address]

        for num, secure in dt_split_channels_get(node):
            self._macros.append(
                MacroCall(
                    "UICR_SPU_FEATURE_GRTC_CC_SET",
                    [
                        Address(spu_address),
                        num,
                        secure,
                        self._owner_id.c_enum,
                    ],
                    comment=f"{spu_name}: GRTC CC{num} permissions",
                )
            )

    def _add_peripheral_gpios_spu_permissions_and_ctrlsel(
        self,
        node: Node,
        skip_ctrlsel: bool = False,
    ) -> None:
        """Adds SPU FEATURE.GPIO[n].PIN[m] and GPIO PIN_CNF[n] entries for
        pins used in phandle-array properties called 'gpios' or ending in '-gpios'
        found on the peripheral node.
        """
        for name in node.props:
            if not re.fullmatch(r"^(.+-)?gpios$", name):
                continue

            if node.props[name].type != "phandle-array":
                log.debug(f"skipping *-gpios prop {name} in {node.path} (not a phandle-array)")
                continue

            for entry in dt_array_prop(node, name):
                gpio_node = entry.controller
                if "nordic,nrf-gpio" not in gpio_node.compats:
                    continue

                port = gpio_node.props["port"].val
                num = entry.data["pin"]
                secure = dt_node_is_secure(gpio_node)
                if not skip_ctrlsel:
                    ctrlsel = self._hw_tables.lookup_ctrlsel_for_property(
                        node.props[name], (port, num)
                    )
                else:
                    ctrlsel = None
                self._configure_gpio_pin(entry.controller, num, secure, ctrlsel)

    def _add_peripheral_pinctrls_spu_permissions_and_ctrlsel(self, node: Node) -> None:
        """Adds SPU FEATURE.GPIO[n].PIN[m] and GPIO PIN_CNF[n] entries for
        pins used in pinctrl properties referenced by the peripheral node.
        """
        secure = dt_node_is_secure(node)
        for pinctrl in node.pinctrls:
            for config_node in pinctrl.conf_nodes:
                for group_node in config_node.children.values():
                    for i, psel_val in enumerate(dt_array_prop(group_node, "psels")):
                        psel = NrfPsel.from_raw(psel_val)

                        if psel.is_disconnected():
                            # Pin is unused and should be ignored
                            continue

                        gpio_node = find_gpio_node_by_port(
                            node.edt,
                            psel.port,
                            err_suffix=f" (referenced by {group_node.path}:psels[{i}])",
                        )
                        num = psel.pin
                        ctrlsel = self._hw_tables.lookup_ctrlsel_for_pinctrl(pinctrl, psel)
                        self._configure_gpio_pin(gpio_node, num, secure, ctrlsel)

    def _add_nrf_saadc_channel_pin_spu_permissions(self, node: Node) -> None:
        """Adds SPU FEATURE.GPIO[n].PIN[m] and GPIO PIN_CNF[n] entries for
        pins corresponding to the channel properties on the ADC peripheral node.
        """
        saadc_addr = dt_reg_addr(node, secure=True)
        if saadc_addr not in self._hw_tables.adc_channel_pin_lookup:
            return

        channel_pins = self._hw_tables.adc_channel_pin_lookup[saadc_addr]
        secure = dt_node_is_secure(node)

        for name, child in node.children.items():
            if not name.startswith("channel"):
                continue

            for prop_name in ["zephyr,input-positive", "zephyr,input-negative"]:
                prop_val = dt_prop(child, prop_name, None)
                if prop_val in channel_pins:
                    port, num = channel_pins[prop_val]
                    gpio_node = find_gpio_node_by_port(node.edt, port)
                    self._configure_gpio_pin(gpio_node, num, secure, CTRLSEL_DEFAULT)

    def _add_nrf_comp_lpcomp_channel_pin_spu_permissions(self, node: Node) -> None:
        """Adds SPU FEATURE.GPIO[n].PIN[m] and GPIO PIN_CNF[n] entries for
        pins corresponding to the channel properties on the COMP/LPCOMP peripheral node.
        """
        comp_addr = dt_reg_addr(node, secure=True)
        if comp_addr not in self._hw_tables.comp_channel_pin_lookup:
            return

        channel_pins = self._hw_tables.comp_channel_pin_lookup[comp_addr]
        secure = dt_node_is_secure(node)

        for prop_name in ["psel", "extrefsel"]:
            prop_val = dt_prop(node, prop_name, None)
            if prop_val in channel_pins:
                port, num = channel_pins[prop_val]
                gpio_node = find_gpio_node_by_port(node.edt, port)
                self._configure_gpio_pin(gpio_node, num, secure, CTRLSEL_DEFAULT)

    def _configure_gpio_pin(
        self,
        gpio_node: Node,
        num: int,
        secure: bool,
        ctrlsel: int | None,
    ) -> None:
        gpio_addr = dt_reg_addr(gpio_node)
        gpio_port = dt_prop(gpio_node, "port")
        spu_address = get_spu_addr_for_periph(gpio_addr)
        spu_name = self._hw_tables.spu_addr_to_name[spu_address]

        self._macros.append(
            MacroCall(
                "UICR_SPU_FEATURE_GPIO_PIN_SET",
                [
                    Address(spu_address),
                    gpio_port,
                    num,
                    secure,
                    self._owner_id.c_enum,
                ],
                comment=f"{spu_name}: P{gpio_port}.{num} permissions",
            )
        )

        if ctrlsel is not None:
            ctrlsel_int = int(ctrlsel)
            self._macros.append(
                MacroCall(
                    "UICR_GPIO_PIN_CNF_CTRLSEL_SET",
                    [
                        Address(gpio_addr),
                        num,
                        ctrlsel_int,
                    ],
                    comment=f"P{gpio_port}.{num} CTRLSEL = {ctrlsel_int}",
                )
            )


def find_gpio_node_by_port(dt: EDT, port: int, err_suffix: str = "") -> Node:
    """Find the GPIO node in the devicetree with the given port."""
    for gpio_node in dt.compat2nodes["nordic,nrf-gpio"]:
        if gpio_node.props["port"].val == port:
            return gpio_node
    raise BadDevicetreeError(f"Failed to find Nordic GPIO node with port {port}{err_suffix}")


@dataclass(order=True)
class MacroCall:
    name: str
    args: list
    comment: str | None = None

    def c_render(self, nodelabel_lookup: dict[int, str]) -> str:
        """Render the macro as C code"""
        str_args = []
        for arg in self.args:
            if isinstance(arg, Address):
                if int(arg) in nodelabel_lookup:
                    str_args.append(c_dt_reg_addr_by_label(nodelabel_lookup[int(arg)]))
                else:
                    str_args.append(c_hex_addr(int(arg)))
            elif isinstance(arg, bool):
                str_args.append(c_bool(arg))
            elif hasattr(arg, "c_enum"):
                str_args.append(arg.c_enum)
            else:
                str_args.append(str(arg))

        comment = f"/* {self.comment} */\n" if self.comment else ""
        return f"{comment}{self.name}({', '.join(str_args)});"


def c_hex_addr(address: int) -> str:
    """Format address as a C 32-bit hex literal."""
    return f"0x{address:08x}UL"


def c_bool(value: bool) -> str:
    """Format value as a C bool literal."""
    return "true" if value else "false"


def c_dt_reg_addr_by_label(nodelabel: str) -> str:
    """Format a peripheral address using devicetree macros to get the address based on node label"""
    return f"DT_REG_ADDR(DT_NODELABEL({nodelabel}))"


# Equivalent to itertools.batched(), using the recipe provided in the docs.
def batched(iterable: Iterable, n: int, *, strict: bool = False) -> Iterator:
    if n < 1:
        raise ValueError("n must be at least one")
    iterator = iter(iterable)
    while batch := tuple(islice(iterator, n)):
        if strict and len(batch) != n:
            raise ValueError("batched(): incomplete batch")
        yield batch


# Sentinel used to represent no provided default value in the functions below
class NoDefault: ...


NO_DEFAULT = NoDefault


def dt_reg_addr(
    node: Node,
    index: int = 0,
    secure: bool = False,
    default: int | type[NoDefault] = NO_DEFAULT,
) -> int:
    """Get a register address and property identifier for a node."""
    try:
        addr = node.regs[index].addr
    except IndexError:
        if isinstance(default, int):
            addr = default
        else:
            raise
    if secure:
        return int(Address(addr).as_secure())
    return addr


def dt_reg_size(node: Node, index: int = 0) -> int:
    """Get a register size and property identifier for a node."""
    return node.regs[index].size


def dt_prop(node: Node, name: str, default: Any = NO_DEFAULT) -> Any:
    """Get the property value and identfier of a property.
    Optionally returns a default value.
    """
    try:
        prop = node.props[name]
    except KeyError:
        if default != NO_DEFAULT:
            return default
        raise

    return prop.val


def dt_node_identifier(node: Node, default_to_path: bool = True) -> str:
    """Get a string that identifies the node.
    Returns the first nodelabel if it exists, otherwise defaults to the node path.
    If default_to_path is False, exits with an error instead of defaulting.
    """
    if node.labels:
        return node.labels[0]
    elif default_to_path:
        return node.path
    raise BuilderError(f"Expected a nodelabel on {node}, but the node has no label")


def dt_array_prop(
    node: Node,
    name: str,
    default: list[Any] | type[NO_DEFAULT] = NO_DEFAULT,
) -> list[Any]:
    """Get the member values and identifiers of an array property.
    Optionally returns a default value.
    """
    try:
        prop = node.props[name]
    except KeyError:
        if not isinstance(default, type):
            return list(default)
        raise

    return list(prop.val)


def dt_node_processors_from_labels(node: Node) -> list[ProcessorId]:
    """Deduce a processor ID from a list of devicetree nodelabels."""
    substring_processor = {cpu.zephyr_name: cpu for cpu in ProcessorId.__members__.values()}
    processors = set()
    for substring, processor_id in substring_processor.items():
        if any(substring in label for label in node.labels):
            processors.add(processor_id)
    return list(processors)


def dt_split_channels_get(
    node: Node,
    owned_name: str = "owned-channels",
    nonsecure_name: str = "nonsecure-channels",
) -> list[tuple[int, bool]]:
    """Parse 'split channels' properties."""
    owned = []
    owned.extend(dt_array_prop(node, owned_name, default=[]))
    owned.extend(dt_array_prop(node, f"child-{owned_name}", default=[]))

    sec_lookup = {}
    if nonsecure_name in node.props:
        nonsecure = dt_array_prop(node, nonsecure_name)
        sec_lookup.update(dict.fromkeys(nonsecure, False))

    default_sec = dt_node_is_secure(node)
    channels = []
    for ch in owned:
        sec = sec_lookup.setdefault(ch, default_sec)
        channels.append((ch, sec))

    return channels


def dt_node_is_secure(node: Node) -> bool:
    if node.bus_node is not None and node.bus_node.regs:
        addr = dt_reg_addr(node.bus_node)
    elif node.regs:
        addr = dt_reg_addr(node)
    else:
        raise ValueError(
            f"Failed to determine security of {node.path} "
            "from the address of its bus node or itself"
        )
    return Address(addr).security


def dt_processor_id(devicetree: EDT) -> ProcessorId:
    """Get processor information from a domain's devicetree."""
    cpus = [
        node
        for node in devicetree.get_node("/cpus").children.values()
        if node.name.startswith("cpu@")
    ]
    if len(cpus) != 1:
        raise RuntimeError(
            f"Expected exactly 1 'cpu' node, but devicetree contained {len(cpus)} nodes"
        )

    try:
        return ProcessorId(cpus[0].regs[0].addr)
    except Exception as e:
        raise RuntimeError(
            f"Devicetree 'cpu' node has invalid Processor ID {cpus[0].regs[0].addr}"
        ) from e


@dataclass(frozen=True)
class NrfPsel:
    """Decoded NRF_PSEL values."""

    fun: NrfFun
    port: int
    pin: int

    @classmethod
    def from_raw(cls, psel_value: int) -> NrfPsel:
        """Decode a raw NRF_PSEL encoded int value to its individual parts."""
        port, pin = divmod(psel_value & (~NRF_PSEL_FUN_MASK), NRF_PSEL_GPIO_PIN_COUNT)
        fun = (psel_value & NRF_PSEL_FUN_MASK) >> NRF_PSEL_FUN_POS
        return NrfPsel(fun=NrfFun(fun), port=port, pin=pin)

    def is_disconnected(self) -> bool:
        """True if the value represents a disconnected pin"""
        return (self.port * NRF_PSEL_GPIO_PIN_COUNT + self.pin) == NRF_PSEL_PIN_MASK


# # Bit position of the function bits in the pinctrl pin value encoded from NRF_PSEL()
NRF_PSEL_FUN_POS = 24
# # Mask for the function bits in the pinctrl pin value encoded from NRF_PSEL()
NRF_PSEL_FUN_MASK = 0xFF << NRF_PSEL_FUN_POS
# Number of pins per port used in NRF_PSEL()
NRF_PSEL_GPIO_PIN_COUNT = 32
# Mask for the port, pin bits in the pinctrl pin value encoded from NRF_PSEL()
NRF_PSEL_PIN_MASK = 0x1FF


@dataclass(frozen=True)
class GpiosProp:
    """CTRLSEL lookup table entry for special *-gpios properties used in some peripheral
    bindings, which are in some cases used instead of pinctrl.
    """

    name: str
    port: int
    pin: int


@enum.unique
class Ctrlsel(int, enum.Enum):
    """
    Enumeration of GPIO.PIN_CNF[n].CTRLSEL values.
    The list here may not be exhaustive.
    """

    GPIO = 0
    VPR_GRC = 1
    CAN_PWM_I3C = 2
    SERIAL0 = 3
    EXMIF_RADIO_SERIAL1 = 4
    CAN_TDM_SERIAL2 = 5
    CAN = 6
    TND = 7


# Default CTRLSEL value indicating that CTRLSEL should not be used
CTRLSEL_DEFAULT = Ctrlsel.GPIO


class NrfFun(int, enum.Enum):
    """Pin functions used with pinctrl, see include/zephyr/dt-bindings/pinctrl/nrf-pinctrl.h
    Only the functions relevant for CTRLSEL deduction have been included.
    """

    UART_TX = 0
    UART_RX = 1
    UART_RTS = 2
    UART_CTS = 3
    SPIM_SCK = 4
    SPIM_MOSI = 5
    SPIM_MISO = 6
    SPIS_SCK = 7
    SPIS_MOSI = 8
    SPIS_MISO = 9
    SPIS_CSN = 10
    TWIM_SCL = 11
    TWIM_SDA = 12
    PWM_OUT0 = 22
    PWM_OUT1 = 23
    PWM_OUT2 = 24
    PWM_OUT3 = 25
    EXMIF_CK = 35
    EXMIF_DQ0 = 36
    EXMIF_DQ1 = 37
    EXMIF_DQ2 = 38
    EXMIF_DQ3 = 39
    EXMIF_DQ4 = 40
    EXMIF_DQ5 = 41
    EXMIF_DQ6 = 42
    EXMIF_DQ7 = 43
    EXMIF_CS0 = 44
    EXMIF_CS1 = 45
    CAN_TX = 46
    CAN_RX = 47
    TWIS_SCL = 48
    TWIS_SDA = 49
    EXMIF_RWDS = 50
    GRTC_CLKOUT_FAST = 55
    GRTC_CLKOUT_32K = 56
    TDM_SCK_M = 71
    TDM_SCK_S = 72
    TDM_FSYNC_M = 73
    TDM_FSYNC_S = 74
    TDM_SDIN = 75
    TDM_SDOUT = 76
    TDM_MCK = 77

    # Value used to ignore the function field and only check (port, pin)
    IGNORE = -1
    # Fallback for unknown NrfFun values, assumes that pin CTRLSEL should be set to GPIO
    ASSUMED_GPIO = -2

    @classmethod
    def _missing_(cls, value: Any) -> NrfFun:
        return cls.ASSUMED_GPIO


class NrfSaadcChannel(int, enum.Enum):
    """Identifiers representing SAADC channels.
    See include/zephyr/dt-bindings/adc/nrf-saadc-haltium.h.
    """

    AIN0 = 1
    AIN1 = 2
    AIN2 = 3
    AIN3 = 4
    AIN4 = 5
    AIN5 = 6
    AIN6 = 7
    AIN7 = 8
    AIN8 = 9
    AIN9 = 10
    AIN10 = 11
    AIN11 = 12
    AIN12 = 13
    AIN13 = 14


class NrfCompChannel(str, enum.Enum):
    """Identifiers representing COMP/LPCOMP channels.
    See dts/bindings/comparator/nordic,nrf-comp.yaml.
    """

    AIN0 = "AIN0"
    AIN1 = "AIN1"
    AIN2 = "AIN2"
    AIN3 = "AIN3"
    AIN4 = "AIN4"
    AIN5 = "AIN5"
    AIN6 = "AIN6"
    AIN7 = "AIN7"
    AIN8 = "AIN8"
    AIN9 = "AIN9"


class FixedPPIMap(NamedTuple):
    """A DPPIC-PPIB or PPIB-PPIB channel mapping.
    These connections are fixed in the hardware.
    """

    connected_to: int
    channel_map: range


@dataclass
class SocLookupTables:
    """Container for various hardware info needed to generate PERIPHCONF."""

    ctrlsel_lookup: dict[int, dict[NrfPsel | GpiosProp, Ctrlsel]]
    adc_channel_pin_lookup: dict[int, dict[NrfSaadcChannel, tuple[int, int]]]
    comp_channel_pin_lookup: dict[int, dict[NrfCompChannel, tuple[int, int]]]
    dppic_to_ppib_connections: dict[int, FixedPPIMap]
    ppib_to_ppib_connections: dict[int, FixedPPIMap]
    spu_instances: list[tuple[str, int]]
    ppib_instances: list[tuple[str, int]]

    @cached_property
    def spu_addr_to_name(self) -> Mapping[int, str]:
        return {addr: name for name, addr in self.spu_instances}

    @cached_property
    def ppib_addr_to_name(self) -> Mapping[int, str]:
        return {addr: name for name, addr in self.ppib_instances}

    def lookup_ctrlsel_for_property(self, prop: Property, psel: tuple[int, int]) -> Ctrlsel | None:
        """Find the appopriate CTRLSEL value for a given gpios property."""
        if not prop.node.regs:
            # Only nodes with registers can be looked up
            return None

        periph_addr = dt_reg_addr(prop.node, secure=True)
        gpios_prop = GpiosProp(name=prop.name, port=psel[0], pin=psel[1])
        return self._lookup_ctrlsel(periph_addr, gpios_prop)

    def lookup_ctrlsel_for_pinctrl(self, prop: PinCtrl, psel: NrfPsel) -> Ctrlsel | None:
        """Find the appopriate CTRLSEL value for a given pinctrl."""
        if psel.fun == NrfFun.ASSUMED_GPIO:
            # We map unsupported values to GPIO CTRLSEL
            return CTRLSEL_DEFAULT

        periph_addr = dt_reg_addr(prop.node, secure=True)
        return self._lookup_ctrlsel(periph_addr, psel)

    def _lookup_ctrlsel(
        self,
        periph_addr: int,
        prop_or_psel: NrfPsel | GpiosProp,
    ) -> Ctrlsel | None:
        ctrlsel = None

        if periph_addr in self.ctrlsel_lookup:
            ident_lut = self.ctrlsel_lookup[periph_addr]
            if prop_or_psel in ident_lut:
                ctrlsel = ident_lut[prop_or_psel]
            elif isinstance(prop_or_psel, NrfPsel):
                # Check if this entry is enumerated with "ignored" function
                sub_entry_no_fun = NrfPsel(
                    fun=NrfFun.IGNORE, port=prop_or_psel.port, pin=prop_or_psel.pin
                )
                ctrlsel = ident_lut.get(sub_entry_no_fun, None)

        log.debug(f"periph_addr=0x{periph_addr:09_x}, {prop_or_psel=} -> {ctrlsel=}")

        return ctrlsel


def get_spu_addr_for_periph(periph_addr: int) -> int:
    """Get the address of the SPU instance governing the permissions for the peripheral
    at the given address.
    """
    address = Address(periph_addr)

    # The common rule is that the SPU instance sits at the start of the address space for
    # the bus of the peripheral.
    address.security = True
    address.slave_index = 0
    address.address_space = 0

    # However, some buses are special due to having > 16 slaves and need special handling.
    address.bus = SPU_ADDRESS_BUS_REMAPPING.get(address.bus, address.bus)

    return int(address)


SPU_ADDRESS_BUS_REMAPPING = {
    # Both of these bus IDs represent APB32 and should use the same SPU instance with bus ID 146.
    147: 146,
}


@enum.unique
class DomainId(enum.IntEnum):
    """Domain IDs."""

    RESERVED = 0
    SECURE = 1
    APPLICATION = 2
    RADIOCORE = 3
    CELLCORE = 4
    GLOBALFAST = 12
    GLOBALSLOW = 13
    GLOBAL_ = 14
    GLOBAL = 15

    @property
    def c_enum(self) -> str:
        return f"NRF_DOMAIN_{self.name.upper()}"


@enum.unique
class OwnerId(enum.IntEnum):
    """Owner IDs."""

    NONE = 0
    SECURE = 1
    APPLICATION = 2
    RADIOCORE = 3
    CELL = 4
    SYSCTRL = 8

    @property
    def c_enum(self) -> str:
        return f"NRF_OWNER_{self.name.upper()}"


@enum.unique
class ProcessorId(enum.IntEnum):
    """Processor IDs."""

    SECURE = 1
    APPLICATION = 2
    RADIOCORE = 3
    CELLCORE = 4
    SYSCTRL = 12
    PPR = 13
    FLPR = 14

    @property
    def zephyr_name(self) -> str:
        """Name used in zephyr to denote the processor with this ID."""
        match self:
            case ProcessorId.SECURE:
                return "cpusec"
            case ProcessorId.APPLICATION:
                return "cpuapp"
            case ProcessorId.RADIOCORE:
                return "cpurad"
            case ProcessorId.CELLCORE:
                return "cpucell"
            case ProcessorId.SYSCTRL:
                return "cpusys"
            case ProcessorId.PPR:
                return "cpuppr"
            case ProcessorId.FLPR:
                return "cpuflpr"

    @property
    def default_owner_id(self) -> OwnerId:
        """Default owner ID associated with this ID (the ID used by accesses from the processor)."""
        match self:
            case ProcessorId.SECURE:
                return OwnerId.SECURE
            case ProcessorId.APPLICATION:
                return OwnerId.APPLICATION
            case ProcessorId.RADIOCORE:
                return OwnerId.RADIOCORE
            case ProcessorId.CELLCORE:
                return OwnerId.CELL
            case ProcessorId.SYSCTRL:
                return OwnerId.SYSCTRL
            case ProcessorId.PPR:
                return OwnerId.APPLICATION
            case ProcessorId.FLPR:
                return OwnerId.APPLICATION

    @property
    def c_enum(self) -> str:
        return f"NRF_PROCESSOR_{self.name.upper()}"


@enum.unique
class AddressRegion(enum.IntEnum):
    """Address regions, defined by Address Format of the data sheet."""

    PROGRAM = 0
    DATA = 1
    PERIPHERAL = 2
    EXT_XIP = 3
    EXT_XIP_ENCRYPTED = 4
    STM = 5
    CPU = 7

    @classmethod
    def from_address(cls, address: int) -> AddressRegion:
        """Get the address region of an address."""
        return Address(address).region


# Regions that have domain ID and security fields
HAS_DOMAIN_SECURITY = [
    AddressRegion.PROGRAM,
    AddressRegion.DATA,
    AddressRegion.PERIPHERAL,
    AddressRegion.STM,
]

# Regions that have the peripheral address format
HAS_PERIPH_BITS = [
    AddressRegion.PERIPHERAL,
    AddressRegion.STM,
]


class Address:
    """Helper for working with addresses."""

    def __init__(self, value: int | Address = 0) -> None:
        self._val = int(value)

    def __repr__(self) -> str:
        if self.region in HAS_DOMAIN_SECURITY:
            domain_sec_str = (
                f", domain={self.domain.name} ({int(self.domain)}), security={self.security}"
            )
        else:
            domain_sec_str = ""

        if self.region in HAS_PERIPH_BITS:
            periph_bits_str = (
                f", bus={self.bus} (0b{self.bus:09_b}), "
                f"slave_index={self.slave_index} (0b{self.slave_index:09_b})"
            )
        else:
            periph_bits_str = ""

        field_str = (
            f"region={self.region.name} ({int(self.region)}){domain_sec_str}{periph_bits_str}, "
            f"address_space=0x{self.address_space:_x}"
        )

        return f"{type(self).__name__}({field_str})"

    def __str__(self) -> str:
        return repr(self)

    def as_secure(self) -> Address:
        addr = Address(self)
        addr.security = True
        return addr

    @property
    def region(self) -> AddressRegion:
        """Address region."""
        return AddressRegion(get_field(self._val, ADDRESS_REGION_POS, ADDRESS_REGION_MASK))

    @region.setter
    def region(self, new: int) -> None:
        self._val = update_field(self._val, new, ADDRESS_REGION_POS, ADDRESS_REGION_MASK)

    @property
    def security(self) -> bool:
        """Address security (only present in some regions)."""
        self._check_has_security()
        return bool(get_field(self._val, ADDRESS_SECURITY_POS, ADDRESS_SECURITY_MASK))

    @security.setter
    def security(self, new: bool) -> None:
        self._check_has_security()
        self._val = update_field(self._val, int(new), ADDRESS_SECURITY_POS, ADDRESS_SECURITY_MASK)

    def _check_has_security(self) -> None:
        self._check_region_has_field(HAS_DOMAIN_SECURITY, "security bit")

    @property
    def domain(self) -> DomainId:
        """Address domain ID (only present in some regions)."""
        self._check_has_domain_id()
        return DomainId(get_field(self._val, ADDRESS_DOMAIN_POS, ADDRESS_DOMAIN_MASK))

    @domain.setter
    def domain(self, new: DomainId | int) -> None:
        self._check_has_domain_id()
        self._val = update_field(self._val, new, ADDRESS_DOMAIN_POS, ADDRESS_DOMAIN_MASK)

    def _check_has_domain_id(self) -> None:
        self._check_region_has_field(HAS_DOMAIN_SECURITY, "domain ID")

    @property
    def bus(self) -> int:
        """Bus ID (only present in some regions)."""
        self._check_has_bus()
        return get_field(self._val, ADDRESS_BUS_POS, ADDRESS_BUS_MASK)

    @bus.setter
    def bus(self, new: int) -> None:
        self._check_has_bus()
        self._val = update_field(self._val, new, ADDRESS_BUS_POS, ADDRESS_BUS_MASK)

    def _check_has_bus(self) -> None:
        self._check_region_has_field(HAS_PERIPH_BITS, "Peripheral/APB bus number")

    @property
    def slave_index(self) -> int:
        """Slave index (only present in some regions)."""
        self._check_has_slave_index()
        return get_field(self._val, ADDRESS_SLAVE_POS, ADDRESS_SLAVE_MASK)

    @slave_index.setter
    def slave_index(self, new: int) -> None:
        self._check_has_slave_index()
        self._val = update_field(self._val, new, ADDRESS_SLAVE_POS, ADDRESS_SLAVE_MASK)

    def _check_has_slave_index(self) -> None:
        self._check_region_has_field(HAS_PERIPH_BITS, "Peripheral/APB slave index")

    @property
    def address_space(self) -> int:
        """Internal address space address (semantics depend on the region)."""
        match self.region:
            case AddressRegion.PROGRAM | AddressRegion.DATA:
                return get_field(self._val, ADDRESS_SPACE_POS, ADDRESS_PROGRAM_DATA_SPACE_MASK)
            case AddressRegion.PERIPHERAL | AddressRegion.STM:
                return get_field(self._val, ADDRESS_SPACE_POS, ADDRESS_PERIPHERAL_SPACE_MASK)
            case _:
                return get_field(self._val, ADDRESS_SPACE_POS, ADDRESS_DEFAULT_SPACE_MASK)

    @address_space.setter
    def address_space(self, new: int) -> None:
        match self.region:
            case AddressRegion.PROGRAM | AddressRegion.DATA:
                self._val = update_field(
                    self._val, new, ADDRESS_SPACE_POS, ADDRESS_PROGRAM_DATA_SPACE_MASK
                )
            case AddressRegion.PERIPHERAL | AddressRegion.STM:
                self._val = update_field(
                    self._val, new, ADDRESS_SPACE_POS, ADDRESS_PERIPHERAL_SPACE_MASK
                )
            case _:
                self._val = update_field(
                    self._val, new, ADDRESS_SPACE_POS, ADDRESS_DEFAULT_SPACE_MASK
                )

    def _check_region_has_field(self, valid_regions: list[AddressRegion], field_name: str) -> None:
        if self.region not in valid_regions:
            raise ValueError(f"{field_name} is not defined for address region {self.region.name}")

    def __eq__(self, other) -> bool:
        return int(self) == int(other)

    def __lt__(self, other: Address | int) -> bool:
        return int(self) < int(other)

    def __int__(self) -> int:
        return self._val


ADDRESS_REGION_POS = 29
ADDRESS_REGION_MASK = 0x7 << ADDRESS_REGION_POS
ADDRESS_SECURITY_POS = 28
ADDRESS_SECURITY_MASK = 0x1 << ADDRESS_SECURITY_POS
ADDRESS_DOMAIN_POS = 24
ADDRESS_DOMAIN_MASK = 0xF << ADDRESS_DOMAIN_POS
ADDRESS_BUS_POS = 16
ADDRESS_BUS_MASK = 0xFF << ADDRESS_BUS_POS
ADDRESS_SLAVE_POS = 12
ADDRESS_SLAVE_MASK = 0xF << ADDRESS_SLAVE_POS
ADDRESS_PERIPHID_POS = 12
ADDRESS_PERIPHID_MASK = 0x7FF << ADDRESS_PERIPHID_POS
ADDRESS_SPACE_POS = 0
ADDRESS_PROGRAM_DATA_SPACE_MASK = 0xFF_FFFF
ADDRESS_PERIPHERAL_SPACE_MASK = 0xFFF
ADDRESS_DEFAULT_SPACE_MASK = 0x1FFF_FFFF


def peripheral_id_get(periph_address: int) -> int:
    """Get the peripheral ID of a peripheral address."""
    return get_field(periph_address, ADDRESS_PERIPHID_POS, ADDRESS_PERIPHID_MASK)


def get_field(value: int, field_pos: int, field_mask: int) -> int:
    """Get the value of a field in a bitfield."""
    return (value & field_mask) >> field_pos


def update_field(value: int, field_new: int, field_pos: int, field_mask: int) -> int:
    """Update a field in a bitfield."""
    return (value & ~field_mask) | ((field_new << field_pos) & field_mask)

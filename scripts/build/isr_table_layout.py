# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Describe ISR input placement independently of the linker script format."""

from dataclasses import dataclass, field


@dataclass(frozen=True)
class InputSection:
    name: str
    offset: int
    size: int
    alignment: int
    symbol: str

    def __post_init__(self):
        if any(char in self.name for char in ('"', ';', '\n', '\r', '\x00')):
            raise ValueError(f"Invalid ISR input section name: {self.name!r}")
        if self.offset < 0 or self.size <= 0:
            raise ValueError(f"Invalid ISR input extent: {self.name}")
        if self.alignment <= 0 or self.alignment & (self.alignment - 1):
            raise ValueError(f"Invalid ISR input alignment: {self.name}")
        if self.offset % self.alignment:
            raise ValueError(f"Misaligned ISR input: {self.name}")


@dataclass
class Table:
    section: str
    symbol: str
    alignment: int
    size: int
    inputs: list[InputSection] = field(default_factory=list)

    @property
    def end_symbol(self):
        return f"{self.symbol}_end"

    @property
    def alignment_section(self):
        return f".isr_align.{self.section}"

    @property
    def alignment_symbol(self):
        return f"z_isr_align_{self.symbol}"

    def write_alignment_source(self, stream, *, writable):
        """Raise GNU output alignment without requiring a C extension elsewhere."""
        storage = "" if writable else "const "
        stream.write(
            "#if defined(TOOLCHAIN_HAS_ZLA) && TOOLCHAIN_HAS_ZLA\n"
            f"static {storage}uint8_t {self.alignment_symbol}[0] "
            f"__aligned({self.alignment}) __used __noasan\n"
            f'Z_GENERIC_SECTION_LITERAL("{self.alignment_section}");\n'
            "#endif\n"
        )

    def validate(self):
        if self.alignment <= 0 or self.alignment & (self.alignment - 1):
            raise ValueError(f"Invalid ISR table alignment: {self.section}")
        if self.size < 0:
            raise ValueError(f"Invalid ISR table size: {self.section}")
        cursor = 0
        names = set()
        for item in self.inputs:
            if item.alignment > self.alignment:
                raise ValueError(f"ISR input alignment exceeds its table: {item.name}")
            if item.offset != cursor:
                raise ValueError(f"Non-contiguous {self.section} at {item.name}: {cursor}")
            if item.name in names:
                raise ValueError(f"Repeated ISR input section: {item.name}")
            names.add(item.name)
            cursor += item.size
        if cursor != self.size:
            raise ValueError(f"Incorrect {self.section} size: {cursor}, expected {self.size}")

    def write_ld(self, stream):
        self.validate()
        stream.write(
            f'KEEP(*("{self.alignment_section}"))\n'
            f". = ALIGN({self.alignment});\n{self.symbol} = .;\n"
        )
        for item in self.inputs:
            stream.write(
                f'ASSERT(. - {self.symbol} == {item.offset}, "ISR input offset: {item.symbol}");\n'
                f"{item.symbol} = .;\n"
                f'KEEP(*("{item.name}"))\n'
                f'{item.symbol}_end = .;\n'
                f'ASSERT({item.symbol}_end - {item.symbol} == {item.size}, '
                f'"ISR input size: {item.symbol}");\n'
            )
        stream.write(
            f"{self.end_symbol} = .;\n"
            f'ASSERT({self.end_symbol} - {self.symbol} == {self.size}, '
            f'"ISR table size: {self.symbol}");\n'
        )

    def write_cmake(self, stream, *, section_override=None, base_offset=0, priority_base=None):
        self.validate()
        records = [
            {
                "offset": 0,
                "symbols": self.symbol,
                "alignment": self.alignment,
                "input_section": self.alignment_section,
            }
        ]
        for item in self.inputs:
            records.append(
                {
                    "offset": item.offset,
                    "symbols": f"{item.symbol};{item.symbol}_end",
                    "input_section": item.name,
                    "size": item.size,
                    "alignment": item.alignment,
                }
            )
        records.append({"offset": self.size, "symbols": self.end_symbol})
        for index, record in enumerate(records):
            record["offset"] += base_offset
            self._write_setting(
                stream,
                output_section=section_override,
                priority=None if priority_base is None else priority_base + index,
                **record,
            )

    def _write_setting(
        self,
        stream,
        *,
        offset,
        symbols,
        input_section=None,
        size=0,
        alignment=1,
        output_section=None,
        priority=None,
    ):
        fields = [
            "SECTION",
            output_section or self.section,
            "OFFSET",
            str(offset),
            "ALIGN",
            str(alignment),
            "KEEP",
            "TRUE",
            "MIN_SIZE",
            str(size),
            "MAX_SIZE",
            str(size),
        ]
        if input_section is not None:
            fields.extend(("INPUT", input_section))
        if priority is not None:
            fields.extend(("PRIO", str(priority)))
        fields.extend(("SYMBOLS", *symbols.split(";")))
        record = r"\;".join(fields)
        stream.write(f'list(APPEND SECTION_SETTINGS "{{{record}}}")\n')

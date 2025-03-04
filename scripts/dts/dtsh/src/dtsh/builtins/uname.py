# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree shell built-in "uname".

FIXME: Missing unit tests and examples: tests/test_dtsh_builtin_uname.py
"""


from typing import Optional, List, Mapping, Sequence

from enum import Enum

from rich.text import Text

from dtsh.autocomp import RlStateEnum
from dtsh.config import DTShConfig
from dtsh.hwm import DTShBoard
from dtsh.dts import DTS
from dtsh.io import DTShOutput
from dtsh.rl import DTShReadline
from dtsh.shell import (
    DTSh,
    DTShFlag,
    DTShArg,
    DTShCommand,
    DTShError,
    DTShUsageError,
)
from dtsh.shellutils import DTShFlagPager

from dtsh.rich.modelview import (
    BoardModelView,
    KernelModelView,
    FirmwareModelView,
    FormBoardInfo,
    FormSoCInfo,
    FormKernelInfo,
    FormFirmwareInfo,
)
from dtsh.rich.shellutils import DTShFlagLongList
from dtsh.rich.text import TextUtil
from dtsh.rich.tui import HeadingsContentWriter

_dtshconf: DTShConfig = DTShConfig.getinstance()


class UnameMode(Enum):
    """Command mode."""

    POSIX_LIKE = 1
    SUMMARY = 2


class DTShBuiltinUname(DTShCommand):
    """Devicetree shell built-in "uname"."""

    POSIX_OPTIONS: str = "-mpro"

    # Command mode.
    _mode: UnameMode = UnameMode.POSIX_LIKE

    # Parsed POSIX-like options.
    # Populated by parse_argv().
    _posix_options: List["UnamePosixOption"] = []

    # Summary mode.
    _summary_sections: List["UnameSummarySection"] = []

    def __init__(self) -> None:
        super().__init__(
            "uname",
            "print system information (board, SoC, kernel)",
            [
                UnamePosixFlagMachine(),
                UnamePosixFlagProcessor(),
                UnamePosixFlagKernel(),
                UnamePosixFlagFirmware(),
                UnameFlagAll(),
                DTShFlagLongList(),
                UnameFlagShowYAML(),
                UnameFlagSummary(),
                DTShFlagPager(),
                UnameArgSummaryFmt(),
            ],
            None,
        )

    @property
    def flag_summary(self) -> bool:
        """Commodity for UnameFlagSummary."""
        return self.with_flag(UnameFlagSummary)

    @property
    def flag_all(self) -> bool:
        """Commodity for UnameFlagAll."""
        return self.with_flag(UnameFlagAll)

    @property
    def flag_longfmt(self) -> bool:
        """Commodity for DTShFlagLongList."""
        return self.with_flag(DTShFlagLongList)

    @property
    def flag_yaml(self) -> bool:
        """Commodity for DTShFlagShowYAML."""
        return self.with_flag(UnameFlagShowYAML)

    @property
    def arg_summary(self) -> "UnameArgSummaryFmt":
        """Commodity for UnameArgSummaryFmt."""
        return self.with_arg(UnameArgSummaryFmt)

    def parse_argv(self, argv: Sequence[str]) -> None:
        """Overrides DTShCommand.parse_argv()."""
        super().parse_argv(argv)

        user_posix_opts: List[UnamePosixOption] = self._posix_user_opts()

        if self.flag_summary or self.arg_summary.isset:
            self._mode = UnameMode.SUMMARY

            if self.flag_all:
                raise DTShUsageError(self, "option -a not allowed in summary")
            if self.flag_longfmt:
                raise DTShUsageError(self, "option -l not allowed in summary")
            if user_posix_opts:
                raise DTShUsageError(
                    self,
                    f"options {DTShBuiltinUname.POSIX_OPTIONS} not allowed in summary",
                )

            if self.flag_summary:
                self._summary_sections = self._summary_all_sections()
            else:
                self._summary_sections = self.arg_summary.sections

            # End of Summary mode.
            return

        self._mode = UnameMode.POSIX_LIKE
        if self.flag_yaml:
            raise DTShUsageError(self, "option -Y allowed only in summary")

        if self.flag_all:
            if user_posix_opts:
                raise DTShUsageError(
                    self,
                    f"options {DTShBuiltinUname.POSIX_OPTIONS} not allowed with '-a'",
                )
            self._posix_options = self._posix_all_opts()
        else:
            self._posix_options = user_posix_opts or self._posix_default_opts()

    def execute(self, argv: Sequence[str], sh: DTSh, out: DTShOutput) -> None:
        """Overrides DTShCommand.execute()."""
        super().execute(argv, sh, out)
        dts: DTS = sh.dt.dts

        if self.with_flag(DTShFlagPager):
            out.pager_enter()

        if self._mode == UnameMode.SUMMARY:
            self._uname_summary(dts, out)
        else:
            self._uname_posix(dts, out)

        if self.with_flag(DTShFlagPager):
            out.pager_exit()

    def _uname_posix(self, dts: DTS, out: DTShOutput) -> None:
        after_sth: bool = False
        for posix_opt in self._posix_options:
            if after_sth:
                self._posix_write_dash(out)
            else:
                after_sth = True

            view: Optional[str | Text] = posix_opt.render(
                dts, self.flag_longfmt
            )
            if view:
                out.write(view, end=None)
            else:
                self._posix_write_placeholder(out)

        out.write()

    def _uname_summary(self, dts: DTS, out: DTShOutput) -> None:
        writer: HeadingsContentWriter = HeadingsContentWriter()
        for section in self._summary_sections:
            # compact etc here
            view = section.get_section(dts, compact=not self.flag_yaml)
            if view:
                writer.write_section(view, out)

    def _posix_user_opts(self) -> List["UnamePosixOption"]:
        return [opt for opt in self._posix_all_opts() if opt.isset]

    def _posix_all_opts(self) -> List["UnamePosixOption"]:
        return list(
            opt for opt in self._options if isinstance(opt, UnamePosixOption)
        )

    def _posix_default_opts(self) -> List["UnamePosixOption"]:
        return list(
            opt
            for opt in self._options
            if isinstance(opt, UnamePosixFlagMachine)
        )

    def _summary_all_sections(self) -> List["UnameSummarySection"]:
        return list(UnameArgSummaryFmt.ALL.values())

    def _posix_write_placeholder(self, out: DTShOutput) -> None:
        if self.flag_longfmt:
            out.write(TextUtil.mk_apologies("unavailable"), end=None)
        else:
            out.write("unavailable", end=None)

    def _posix_write_dash(self, out: DTShOutput) -> None:
        out.write(f" {_dtshconf.wchar_dash} ", end=None)


class UnameFlagShowYAML(DTShFlag):
    """Whether to show YAML files content."""

    BRIEF = "show YAML files content"
    SHORTNAME = "Y"
    LONGNAME = "show-yaml"


class UnamePosixOption(DTShFlag):
    """Base flag for uname POSIX-like behavior.

    NOTE: Although it may seem out of place, these DTSh flags can render
    the corresponding POSIX-like output of the command.
    This will largely simplify the implementation of 'uname -mrpol'.
    """

    def render(
        self,
        dts: DTS,
        flag_longfmt: bool,
    ) -> Optional[str | Text]:
        """Render the corresponding POSIX-like output.

        Args:
            dts: DTS, Zephyr kernel and hardware model, application firmware.
            flag_longfmt: Whether the long format flag is set (aka rich output).
        """
        if flag_longfmt:
            return self.render_rich(dts)
        return self.render_raw(dts)

    def render_raw(  # pylint: disable=useless-return
        self,
        dts: DTS,
    ) -> Optional[str]:
        """Render POSIX-like output as strings."""
        del dts  # Unused in base class.
        return None

    def render_rich(  # pylint: disable=useless-return
        self,
        dts: DTS,
    ) -> Optional[Text]:
        """Render POSIX-like output as rich views."""
        del dts  # Unused in base class.
        return None


class UnamePosixFlagMachine(UnamePosixOption):
    """Print the machine hardware name.

    Format: "<board> <long name>"

    <board>: HWMv2 board name or BOARD target
    <long name>: full name retrieved from the board metadata (HWMv2 only),
        or name retrieved from the runner metadata.
    """

    BRIEF = "print the machine hardware name"
    SHORTNAME = "m"
    LONGNAME = "machine"

    def render_raw(
        self,
        dts: DTS,
    ) -> Optional[str]:
        """Overrides UnamePosixFlag.render_raw()."""
        if not dts.board:
            return None
        board: DTShBoard = dts.board

        # HWMv2 board name or BOARD target.
        board_name: str = board.target
        # Full name retrieved from the board metadata (HWMv2 only)
        # or name retrieved from the runner metadata.
        fullname: Optional[str] = board.full_name or board.runner_name
        return " ".join(s for s in (board_name, fullname) if s)

    def render_rich(
        self,
        dts: DTS,
    ) -> Optional[Text]:
        """Overrides UnamePosixFlag.render_rich()."""
        if not dts.board:
            return None
        board: DTShBoard = dts.board

        txt_parts: List[Optional[Text]] = []
        if board.hwm == DTShBoard.HWM.V2:
            txt_parts.append(BoardModelView.mk_board_v2(board))
            txt_parts.append(BoardModelView.mk_full_name(board))
        else:
            txt_parts.append(BoardModelView.mk_board(board))
            txt_parts.append(BoardModelView.mk_runner_name(board))

        return TextUtil.join(" ", (txt for txt in txt_parts if txt))


class UnamePosixFlagProcessor(UnamePosixOption):
    """Print SoC (and other qualifiers) information.

    Format: "<arch> <proc>"

    <arch>: Architecture assumed by the test runner.
    <proc>: SoC and other qualifiers (HWMv2),
        or board type assumed by the test runner.
    """

    BRIEF = "print the processor (SoC)"
    SHORTNAME = "p"
    LONGNAME = "processor"

    def render_raw(
        self,
        dts: DTS,
    ) -> Optional[str]:
        """Overrides UnamePosixFlag.render_raw()."""
        if not dts.board:
            return None
        board: DTShBoard = dts.board

        if board.hwm == DTShBoard.HWM.V2:
            return board.qualifiers

        arch_type: Sequence[Optional[str]] = (
            board.runner_metadata.run_arch,
            board.runner_metadata.run_type,
        )
        return " ".join(s for s in arch_type if s)

    def render_rich(
        self,
        dts: DTS,
    ) -> Optional[Text]:
        """Overrides UnamePosixFlag.render_rich()."""
        if not dts.board:
            return None
        board: DTShBoard = dts.board

        if board.hwm == DTShBoard.HWM.V2:
            return BoardModelView.mk_qualifiers(board)
        return BoardModelView.mk_runner_arch_type(board)


class UnamePosixFlagKernel(UnamePosixOption):
    """Print Zephyr kernel and HWM versions.

    Format: "Zephyr-RTOS <release> <hwm>
    """

    BRIEF = "print Zephyr and HWM versions"
    SHORTNAME = "r"
    LONGNAME = "kernel-version"

    def render_raw(
        self,
        dts: DTS,
    ) -> str:
        """Overrides UnamePosixFlag.render_raw()."""
        kernel_rev: Optional[str] = dts.get_zephyr_head()
        kernel: str = (
            f"Zephyr-RTOS {kernel_rev}" if kernel_rev else "Zephyr-RTOS"
        )
        hwm_version: str = (
            dts.board.hwm.version
            if dts.board
            else DTShBoard.HWM.UNKNOWN.version
        )
        return " ".join((kernel, hwm_version))

    def render_rich(
        self,
        dts: DTS,
    ) -> Text:
        """Overrides UnamePosixFlag.render_rich()."""
        txt_parts: List[Optional[Text]] = [
            TextUtil.mk_text("Zephyr-RTOS"),
            KernelModelView.mk_kernel_version(dts),
        ]
        if dts.board:
            txt_parts.append(BoardModelView.mk_hwm(dts.board.hwm))
        return TextUtil.join(" ", (txt for txt in txt_parts if txt))


class UnamePosixFlagFirmware(UnamePosixOption):
    """Print firmware information.

    Format: <fw name> <fw version> (<toolchain variant>)
    """

    BRIEF = "print the firmware"
    SHORTNAME = "o"
    LONGNAME = "--firmware"

    def render_raw(
        self,
        dts: DTS,
    ) -> Optional[str]:
        """Overrides UnamePosixFlag.render_raw()."""
        parts: List[str] = []
        if dts.fw_name:
            parts.append(dts.fw_name)
        if dts.fw_version:
            parts.append(dts.fw_version)
        if parts:
            return " ".join(parts)
        return None

    def render_rich(
        self,
        dts: DTS,
    ) -> Optional[Text]:
        """Overrides UnamePosixFlag.render_rich()."""
        txt_parts: List[Text] = [
            txt
            for txt in (
                FirmwareModelView.mk_fw_name(dts),
                FirmwareModelView.mk_fw_version(dts),
            )
            if txt
        ]
        if txt_parts:
            return TextUtil.join(" ", txt_parts)
        return None


class UnameSummarySection:
    """Summary section for the command argument."""

    _title: str
    _brief: str

    def __init__(self, title: str, brief: str) -> None:
        self._title = title
        self._brief = brief

    @property
    def title(self) -> str:
        return self._title

    @property
    def brief(self) -> str:
        return self._brief

    def get_section(
        self,
        dts: DTS,
        /,
        *,
        compact: bool = True,
        expand_included: bool = False,
    ) -> HeadingsContentWriter.Section:
        content: HeadingsContentWriter.ContentType = self.mk_content(
            dts, compact=compact, expand_included=expand_included
        ) or TextUtil.mk_apologies("unavailable")
        return HeadingsContentWriter.Section(self._title, content)

    def mk_content(
        self,
        dts: DTS,
        /,
        *,
        compact: bool = True,
        expand_included: bool = False,
    ) -> Optional[HeadingsContentWriter.ContentType]:
        # Unused in base class.
        del dts
        del compact
        del expand_included


class UnameSummaryBoard(UnameSummarySection):
    """Board information section."""

    def __init__(self) -> None:
        super().__init__("Board", "machine hardware information")

    def mk_content(
        self,
        dts: DTS,
        /,
        *,
        compact: bool = True,
        expand_included: bool = False,
    ) -> Optional[HeadingsContentWriter.ContentType]:
        if not dts.board:
            return None
        return FormBoardInfo.create(
            dts.board,
            dts.zephyr_base,
            compact=compact,
            expand_included=expand_included,
        )


class UnameSummaryProcessor(UnameSummarySection):
    """Board information section."""

    def __init__(self) -> None:
        super().__init__("SoC & CPUs", "processor information (SoC)")

    def mk_content(
        self,
        dts: DTS,
        /,
        *,
        compact: bool = True,
        expand_included: bool = False,
    ) -> Optional[HeadingsContentWriter.ContentType]:
        if not dts.board:
            return None
        return FormSoCInfo.create(
            dts.board,
            dts.zephyr_base,
            compact=compact,
            expand_included=expand_included,
        )


class UnameSummaryKernel(UnameSummarySection):
    """Kernel information section."""

    def __init__(self) -> None:
        super().__init__("Kernel", "Zepphyr kernel")

    def mk_content(
        self,
        dts: DTS,
        /,
        *,
        compact: bool = True,
        expand_included: bool = False,
    ) -> Optional[HeadingsContentWriter.ContentType]:
        return FormKernelInfo(dts)


class UnameSummaryFirmware(UnameSummarySection):
    """Firmware information section."""

    def __init__(self) -> None:
        super().__init__("Firmware", "Application")

    def mk_content(
        self,
        dts: DTS,
        /,
        *,
        compact: bool = True,
        expand_included: bool = False,
    ) -> Optional[HeadingsContentWriter.ContentType]:
        return FormFirmwareInfo(dts)


class UnameArgSummaryFmt(DTShArg):
    """Argument to set the summary output format (sections)."""

    BRIEF = "summary output format"
    LONGNAME = "summary"

    ALL: Mapping[str, UnameSummarySection] = {
        "m": UnameSummaryBoard(),
        "p": UnameSummaryProcessor(),
        "r": UnameSummaryKernel(),
        "o": UnameSummaryFirmware(),
    }

    @staticmethod
    def parse_sections(fmt: str) -> List[UnameSummarySection]:
        """Parse format string into sections.

        Args:
            fmt: A format string.

        Returns:
            A list of sections.

        Raises:
            DTShError: Invalid format string.
        """
        try:
            return [UnameArgSummaryFmt.ALL[spec] for spec in fmt]
        except KeyError as e:
            raise DTShError(f"invalid format specifier: '{e}'") from e

    @staticmethod
    def is_valid(fmt: str) -> bool:
        """Validate format string.

        Args:
            fmt: A format string.

        Returns:
            False if the format string is invalid.
        """
        return all(spec in UnameArgSummaryFmt.ALL for spec in fmt)

    # Parsed sections.
    _sections: List[UnameSummarySection] = []

    def __init__(self) -> None:
        super().__init__(argname="fmt")

    @property
    def sections(self) -> List[UnameSummarySection]:
        """The parsed sections."""
        return self._sections

    def reset(self) -> None:
        """Overrides DTShOption.reset()."""
        super().reset()
        self._sections.clear()

    def parsed(self, value: Optional[str] = None) -> None:
        """Overrides DTShArg.parsed()."""
        super().parsed(value)
        if self._raw:
            self._sections = UnameArgSummaryFmt.parse_sections(self._raw)

    def autocomp(self, txt: str, sh: DTSh) -> List[DTShReadline.CompleterState]:
        """Overrides DTShArg.autocomp().

        Auto-complete argument with format specifiers.
        """
        if not UnameArgSummaryFmt.is_valid(txt):
            # Don't complete when current format string is invalid.
            return []
        # Answer all available cells but those already set.
        return sorted(
            [
                RlStateEnum(key, section.brief)
                for key, section in UnameArgSummaryFmt.ALL.items()
                if key not in txt
            ],
            key=lambda x: x.rlstr.lower(),
        )


class UnameFlagAll(DTShFlag):
    """Flag"""

    BRIEF = "print all"
    SHORTNAME = "a"
    LONGNAME = "all"


class UnameFlagSummary(DTShFlag):
    """Print hardware and system summary."""

    BRIEF = "print hardware and system summary"
    SHORTNAME = "S"

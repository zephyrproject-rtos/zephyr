# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree shell built-in "uname".

FIXME: Missing unit tests and examples: tests/test_dtsh_builtin_uname.py
"""


from typing import Optional, Union, List, Sequence

from rich.console import RenderableType
from rich.style import StyleType
from rich.text import Text

from dtsh.config import DTShConfig, ActionableType
from dtsh.dts import DTS
from dtsh.io import DTShOutput
from dtsh.shell import DTSh, DTShCommand, DTShFlag, DTShUsageError
from dtsh.shellutils import DTShFlagPager

from dtsh.rich.modelview import FormLayout, ViewYAMLFile, HeadingsContentWriter
from dtsh.rich.shellutils import DTShFlagLongList
from dtsh.rich.text import TextUtil
from dtsh.rich.theme import DTShTheme
from dtsh.rich.tui import GridLayout, RenderableError

_dtshconf: DTShConfig = DTShConfig.getinstance()


class DTShFlagMachine(DTShFlag):
    """Flag"""

    BRIEF = "print the board name"
    SHORTNAME = "m"
    LONGNAME = "machine"


class DTShFlagKernelInfo(DTShFlag):
    """Flag"""

    BRIEF = "print Zephyr environment"
    SHORTNAME = "o"
    LONGNAME = "operating-system"


class DTShFlagBoardInfo(DTShFlag):
    """Flag
    From BOARD_DIR/board.yml.
    """

    BRIEF = "print board information"
    SHORTNAME = "i"
    LONGNAME = "board"


class DTShFlagSoCInfo(DTShFlag):
    """Flag
    From BOARD_DIR/board.yml.
    """

    BRIEF = "print SoC information"
    SHORTNAME = "n"
    LONGNAME = "soc"


class DTShFlagAll(DTShFlag):
    """Flag"""

    BRIEF = "print system summary"
    SHORTNAME = "a"
    LONGNAME = "all"


class DTShBuiltinUname(DTShCommand):
    """Devicetree shell built-in "uname"."""

    def __init__(self) -> None:
        super().__init__(
            "uname",
            "print system information (kernel, board, SoC)",
            [
                DTShFlagMachine(),
                DTShFlagKernelInfo(),
                DTShFlagBoardInfo(),
                DTShFlagSoCInfo(),
                DTShFlagAll(),
                DTShFlagLongList(),
                DTShFlagPager(),
            ],
            None,
        )

    def parse_argv(self, argv: Sequence[str]) -> None:
        """Overrides DTShCommand.parse_argv()."""
        super().parse_argv(argv)

        cnt_fields: int = [
            self.with_flag(DTShFlagMachine),
            self.with_flag(DTShFlagKernelInfo),
            self.with_flag(DTShFlagBoardInfo),
            self.with_flag(DTShFlagSoCInfo),
        ].count(True)

        if cnt_fields > 1:
            raise DTShUsageError(
                self, "only one option among '-moin' is allowed"
            )
        if self.with_flag(DTShFlagAll) and cnt_fields:
            raise DTShUsageError(self, "options '-moin' not allowed with '-a'")

    def execute(self, argv: Sequence[str], sh: DTSh, out: DTShOutput) -> None:
        """Overrides DTShCommand.execute()."""
        super().execute(argv, sh, out)

        if self.with_flag(DTShFlagPager):
            out.pager_enter()

        if self.with_flag(DTShFlagAll):
            # "uname -a"
            self._uname_all(sh.dt.dts, out)
        elif self.with_flag(DTShFlagMachine):
            # "uname -m"
            self._uname_machine(sh.dt.dts, out)
        elif self.with_flag(DTShFlagKernelInfo):
            # "uname -o"
            self._uname_kernel(sh.dt.dts, out)
        elif self.with_flag(DTShFlagBoardInfo):
            # "uname -i"
            self._uname_board(sh.dt.dts, out)
        elif self.with_flag(DTShFlagSoCInfo):
            # "uname -n"
            self._uname_soc(sh.dt.dts, out)
        else:
            # "uname"
            self._uname_default(sh.dt.dts, out)

        if self.with_flag(DTShFlagPager):
            out.pager_exit()

    def _uname_machine(self, dts: DTS, out: DTShOutput) -> None:
        if self.with_flag(DTShFlagLongList):
            self._uname_machine_long(dts, out)
        else:
            self._uname_machine_raw(dts, out)

    def _uname_machine_raw(self, dts: DTS, out: DTShOutput) -> None:
        if dts.board:
            out.write(dts.board)

    def _uname_machine_long(self, dts: DTS, out: DTShOutput) -> None:
        if dts.board:
            out.write(TextUtil.mk_text(dts.board, DTShTheme.STYLE_INF_BOARD))
        else:
            out.write(TextUtil.mk_apologies("unknown board"))

    def _uname_kernel(self, dts: DTS, out: DTShOutput) -> None:
        if self.with_flag(DTShFlagLongList):
            self._uname_kernel_long(dts, out)
        else:
            self._uname_kernel_raw(dts, out)

    def _uname_kernel_raw(self, dts: DTS, out: DTShOutput) -> None:
        out.write("Zephyr-RTOS", end=None)
        kernel_rev = dts.get_zephyr_head()
        if kernel_rev:
            out.write(f" {kernel_rev}", end=None)
        if dts.toolchain_variant:
            toolchain = (
                "Zephyr SDK"
                if dts.toolchain_variant == "zephyr"
                else dts.toolchain_variant
            )
            out.write(f" ({toolchain})", end=None)
        out.write()

    def _uname_kernel_long(self, dts: DTS, out: DTShOutput) -> None:
        if dts.zephyr_base:
            out.write(FormOperatingSystem(dts))
        else:
            out.write(TextUtil.mk_apologies("system information unavailable"))

    def _uname_board(self, dts: DTS, out: DTShOutput) -> None:
        if self.with_flag(DTShFlagLongList):
            self._uname_board_long(dts, out)
        else:
            self._uname_board_raw(dts, out)

    def _uname_board_raw(self, dts: DTS, out: DTShOutput) -> None:
        # Raw output: "uname -i" is the same as "uname -m"
        self._uname_machine(dts, out)

    def _uname_board_long(self, dts: DTS, out: DTShOutput) -> None:
        if dts.board:
            # May raise RenderableError.
            form = FormBoardInfo(dts, self).view_or_fail(out)
            out.write(form)
        else:
            out.write(TextUtil.mk_apologies("board information unavailable"))

    def _uname_soc(self, dts: DTS, out: DTShOutput) -> None:
        if self.with_flag(DTShFlagLongList):
            self._uname_soc_long(dts, out)
        else:
            self._uname_soc_raw(dts, out)

    def _uname_soc_raw(self, dts: DTS, out: DTShOutput) -> None:
        soc = dts.soc
        if soc:
            out.write(soc)

    def _uname_soc_long(self, dts: DTS, out: DTShOutput) -> None:
        if dts.board:
            # May raise RenderableError.
            form = FormSoCInfo(dts, self).view_or_fail(out)
            out.write(form)
        else:
            out.write(TextUtil.mk_apologies("SoC information unavailable"))

    def _uname_default(self, dts: DTS, out: DTShOutput) -> None:
        if self.with_flag(DTShFlagLongList):
            self._uname_default_long(dts, out)
        else:
            self._uname_default_raw(dts, out)

    def _uname_default_raw(self, dts: DTS, out: DTShOutput) -> None:
        out.write("Zephyr-RTOS", end=None)
        kernel_rel = dts.get_zephyr_head()
        if kernel_rel:
            out.write(f" {kernel_rel}", end=None)
        if dts.board:
            out.write(f" - {dts.board}", end=None)
        out.write()

    def _uname_default_long(self, dts: DTS, out: DTShOutput) -> None:
        out.write(
            TextUtil.mk_text("Zephyr-RTOS", DTShTheme.STYLE_INF_ZEPHYR_KERNEL),
            end=None,
        )
        kernel_rel = dts.get_zephyr_head()
        if kernel_rel:
            out.write(" ", end=None)
            out.write(
                TextUtil.mk_text(
                    kernel_rel, style=DTShTheme.STYLE_INF_ZEPHYR_KERNEL
                ),
                end=None,
            )
        if dts.board:
            out.write(f" {_dtshconf.wchar_dash} ", end=None)
            out.write(
                TextUtil.mk_text(dts.board, style=DTShTheme.STYLE_INF_BOARD),
                end=None,
            )
        out.write()

    def _uname_all(self, dts: DTS, out: DTShOutput) -> None:
        if self.with_flag(DTShFlagLongList):
            self._uname_all_long(dts, out)
        else:
            self._uname_all_raw(dts, out)

    def _uname_all_raw(self, dts: DTS, out: DTShOutput) -> None:
        # Raw output: "uname -a" is the same as "uname"
        self._uname_default_raw(dts, out)

    def _uname_all_long(self, dts: DTS, out: DTShOutput) -> None:
        sections: List[HeadingsContentWriter.Section] = []
        sections.append(
            HeadingsContentWriter.Section(
                "Kernel",
                1,
                FormOperatingSystem(dts)
                if dts.zephyr_base
                else TextUtil.mk_apologies("system information unavailable"),
            )
        )
        sections.append(
            # May raise RenderableError.
            HeadingsContentWriter.Section(
                "Board",
                1,
                FormBoardInfo(dts, self).view_or_fail(out)
                if dts.board
                else TextUtil.mk_apologies("board information unavailable"),
            )
        )
        sections.append(
            # May raise RenderableError.
            HeadingsContentWriter.Section(
                "SoC",
                1,
                FormSoCInfo(dts, self).view_or_fail(out)
                if dts.board
                else TextUtil.mk_apologies("SoC information unavailable"),
            )
        )

        HeadingsContentWriter().write_sections(sections, out)


class FormOperatingSystem(FormLayout):
    """Operating system info (rich)."""

    def __init__(
        self,
        dts: DTS,
        placeholder: Union[str, Text] = "",
        label_style: Optional[StyleType] = None,
        link_type: Optional[ActionableType] = None,
    ) -> None:
        """Initialize view.

        Args:
            dts: Devicetree source information.
              ZEPHYR_BASE must be available.
            placeholder: Placeholder for missing contents.
              Defaults to an empty string.
            label_style: Style to use for the form's labels.
              Defaults to configured preference.
            link_type: Link type to use for contents.
              Defaults to configured preference.
        """
        super().__init__(placeholder, label_style, link_type)
        self._dts = dts
        self._init_zephyr_base()
        self._init_zephyr_kernel()
        self._init_toolchain()
        self._init_devicetree()
        self._init_bindings()
        self._init_vendors_file()
        self._init_fw_app()

    def _init_zephyr_base(self) -> None:
        if not self._dts.zephyr_base:
            raise ValueError()

        self.add_content(
            "ZEPHYR_BASE",
            self.mk_content(
                self._dts.zephyr_base,
                style=DTShTheme.STYLE_INF_ZEPHYR_BASE,
                uri=self._dts.zephyr_base,
            ),
        )

    def _init_zephyr_kernel(self) -> None:
        kernel_rel = self._dts.get_zephyr_head() or ""
        self.add_content(
            "Release",
            self.mk_content(
                f"Zephyr-RTOS {kernel_rel}",
                style=DTShTheme.STYLE_INF_ZEPHYR_KERNEL,
            )
            if kernel_rel
            else None,
        )

    def _init_devicetree(self) -> None:
        self.add_content(
            "Devicetree",
            self.mk_content(
                self._nickpath(self._dts.path),
                style=DTShTheme.STYLE_INF_DEVICETREE,
                uri=self._dts.path,
            ),
        )

    def _init_toolchain(self) -> None:
        tv: Optional[Text] = None
        variant = self._dts.toolchain_variant
        if variant:
            tv = self.mk_content(
                "Zephyr SDK" if variant == "zephyr" else variant,
                style=DTShTheme.STYLE_INF_TOOLCHAIN,
                uri=self._dts.toolchain_dir,
            )
        self.add_content("Toolchain", tv)

    def _init_fw_app(self) -> None:
        img = self._dts.fw_name
        self.add_content(
            "Application",
            self.mk_content(
                img,
                style=DTShTheme.STYLE_INF_APPLICATION,
                uri=self._dts.app_source_dir,
            )
            if img
            else None,
        )

    def _init_bindings(self) -> None:
        grid = GridLayout()
        for binding_dir in self._dts.bindings_search_path:
            grid.add_row(
                self.mk_content(
                    self._nickpath(binding_dir),
                    style=DTShTheme.STYLE_INF_BINDINGS,
                    uri=binding_dir,
                )
            )
        self.add_content("Bindings", grid)

    def _init_vendors_file(self) -> None:
        vendors_file = self._dts.vendors_file
        self.add_content(
            "Vendors file",
            self.mk_content(
                self._nickpath(vendors_file),
                style=DTShTheme.STYLE_INF_VENDORS,
                uri=vendors_file,
            )
            if vendors_file
            else None,
        )

    def _nickpath(self, path: str) -> str:
        # We know ZEPHYR_BASE is available.
        return path.replace(self._dts.zephyr_base, "ZEPHYR_BASE")  # type: ignore


class FormBoardInfo(FormLayout):
    """Hardware info (rich)."""

    _dts: DTS
    _cmd: DTShCommand
    # Rendering error.
    _rerror: Optional[RenderableError]

    def __init__(
        self,
        dts: DTS,
        cmd: DTShCommand,
        placeholder: Union[str, Text] = "",
        label_style: Optional[StyleType] = None,
        link_type: Optional[ActionableType] = None,
    ) -> None:
        """Initialize view.

        Args:
            dts: Devicetree source information.
              BOARD must be available.
            cmd: The running command.
            placeholder: Placeholder for missing contents.
              Defaults to an empty string.
            label_style: Style to use for the form's labels.
              Defaults to configured preference.
            link_type: Link type to use for contents.
              Defaults to configured preference.
        """
        super().__init__(placeholder, label_style, link_type)
        self._dts = dts
        self._cmd = cmd
        self._rerror = None
        self._init_board()
        self._init_board_dir()
        try:
            self._init_board_dts()
            self._init_board_yaml()
        except RenderableError as e:
            # Rendering failed.
            self._rerror = e

    def view_or_fail(self, out: DTShOutput) -> RenderableType:
        """Get final view or fail on rendering error.

        Args:
            out: Where to warn the user if the rendering failed.
        """
        if self._rerror:
            self._rerror.warn_and_forward(
                self._cmd, "failed to retrieve board information", out
            )
        return self

    def _init_board(self) -> None:
        if not self._dts.board:
            raise ValueError()

        self.add_content(
            "Board",
            self.mk_content(self._dts.board, style=DTShTheme.STYLE_INF_BOARD),
        )

    def _init_board_dir(self) -> None:
        board_dir = self._dts.board_dir
        self.add_content(
            "BOARD_DIR",
            self.mk_content(
                board_dir,
                style=DTShTheme.STYLE_LINK_LOCAL,
                uri=board_dir,
            )
            if board_dir
            else None,
        )

    def _init_board_dts(self) -> None:
        board_dts = self._dts.board_file
        self.add_content(
            "DTS",
            self.mk_content(
                self._nickpath(board_dts),
                style=DTShTheme.STYLE_INF_BOARD_FILE,
                uri=board_dts,
            )
            if board_dts
            else None,
        )

    def _init_board_yaml(self) -> None:
        board_yaml = self._dts.board_yaml
        if board_yaml:
            self.add_content(
                "YAML",
                ViewYAMLFile.create(
                    board_yaml,
                    self._dts.yamlfs,
                    is_binding=False,
                    expand_includes=True,
                ),
            )

    def _nickpath(self, path: str) -> str:
        # We know BOARD_DIR is available.
        return path.replace(self._dts.board_dir, "BOARD_DIR")  # type: ignore


class FormSoCInfo(FormLayout):
    """SoC info (rich)."""

    _dts: DTS
    _cmd: DTShCommand
    # Rendering error.
    _rerror: Optional[RenderableError]

    def __init__(
        self,
        dts: DTS,
        cmd: DTShCommand,
        placeholder: Union[str, Text] = "",
        label_style: Optional[StyleType] = None,
        link_type: Optional[ActionableType] = None,
    ) -> None:
        """Initialize view.

        Args:
            dts: Devicetree source information.
              BOARD must be available.
            cmd: The running command.
            placeholder: Placeholder for missing contents.
              Defaults to an empty string.
            label_style: Style to use for the form's labels.
              Defaults to configured preference.
            link_type: Link type to use for contents.
              Defaults to configured preference.
        """
        super().__init__(placeholder, label_style, link_type)
        self._dts = dts
        self._cmd = cmd
        self._rerror = None
        self._init_soc()
        self._init_soc_dir()
        self._init_soc_svd()
        try:
            self._init_soc_yml()
        except RenderableError as e:
            # Rendering failed.
            self._rerror = e

    def view_or_fail(self, out: DTShOutput) -> RenderableType:
        """Get final view or fail on rendering error.

        Args:
            out: Where to warn the user if the rendering failed.
        """
        if self._rerror:
            self._rerror.warn_and_forward(
                self._cmd, "failed to retrieve SoC information", out
            )
        return self

    def _init_soc(self) -> None:
        if not self._dts.board:
            raise ValueError()

        soc = self._dts.soc
        self.add_content(
            "SoC",
            self.mk_content(soc, style=DTShTheme.STYLE_INF_SOC)
            if soc
            else None,
        )

    def _init_soc_dir(self) -> None:
        soc_dir = self._dts.soc_dir
        self.add_content(
            "Files",
            self.mk_content(
                self._nickpath(soc_dir),
                style=DTShTheme.STYLE_LINK_LOCAL,
                uri=soc_dir,
            )
            if soc_dir
            else None,
        )

    def _init_soc_svd(self) -> None:
        soc_svd = self._dts.soc_svd
        self.add_content(
            "SVD",
            self.mk_content(
                soc_svd,
                style=DTShTheme.STYLE_INF_SOC_SVD,
                uri=soc_svd,
            )
            if soc_svd
            else None,
        )

    def _init_soc_yml(self) -> None:
        soc_yml = self._dts.soc_yml
        if soc_yml:
            self.add_content(
                "YAML",
                ViewYAMLFile.create(
                    soc_yml,
                    self._dts.yamlfs,
                    is_binding=False,
                    expand_includes=True,
                ),
            )

    def _nickpath(self, path: str) -> str:
        if self._dts.zephyr_base:
            return path.replace(self._dts.zephyr_base, "ZEPHYR_BASE")
        return path

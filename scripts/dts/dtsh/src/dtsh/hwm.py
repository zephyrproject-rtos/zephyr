# Copyright (c) 2024 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Facade to the Zephyr Hardware models.

Relies on external CMake cache variables to retrieve the HWM
used at build-time when the DTS was generated.

The HWM version is determined via simple tricks:
- if the board target (aka BOARD) contains some "/", it's HWMv2
- assuming we found a valid board directory (aka BOARD_DIR),
  if it contains a board.yml meta-data file it's HWMv2, otherwise it's HWMv1
- then, if we found a SoC directory (aka SOC_FULL_DIR) it's HWMv2

When the hardware model has not been identified, HWMv1 is assumed.

Paths returned by the API may not point to an existing file or directory:
- it's either the value of CMake variable, e.g. BOARD_DIR or SOC_SVD_FILE
- or derived from such a variable, e.g. BOARD_DIR/board.yml
- when multiple derivations are possible (HWMv2), e.g. when the SoC part
  can be omitted, the API will try to find a file that actually exists,
  and return the last path tested, successful or not

See also: tests/test_dtsh_hwm.py
"""

from typing import Any, Optional, Tuple, List, Dict, Mapping

from enum import Enum
from pathlib import Path

import re
import sys
import yaml

from dtsh.utils import CMakeCache


class HWMetaData:
    """Commodity for the simple YAML metadata we'll need here."""

    Content = Dict[Any, Any]
    """Commodity type hint for YAML files content."""

    # Absolute path to metadata file.
    _path: Path

    # Content as text.
    _text: str

    # Structured YAML content.
    _raw: Dict[Any, Any]

    def __init__(self, path: Path) -> None:
        """Initialize metadata.

        Args:
            path: Path to YAML file.
        """
        self._raw = {}
        self._path = path
        if self._path.is_file():
            try:
                with open(path.absolute(), "r", encoding="utf-8") as f:
                    self._text = f.read().strip()
                    self._raw = yaml.safe_load(self._text)
            except (OSError, yaml.YAMLError) as e:
                # Something odd happened.
                print(f"DTShSimpleMetaData(): {e}", file=sys.stderr)

    @property
    def path(self) -> Path:
        """Metadata file path."""
        return self._path

    @property
    def text(self) -> str:
        """YAML file content as text."""
        return self._text

    @property
    def raw(self) -> Content:
        """Structured YAML content."""
        return self._raw


class BoardMetadata(HWMetaData):
    """Board metadata (HWMv2 only).

    Content of: ${BOARD_DIR}/board.yml).
    """

    _raw_board: HWMetaData.Content

    def __init__(self, path: Path) -> None:
        super().__init__(path)
        self._raw_board = self._raw.get("board", {})

    @property
    def full_name(self) -> Optional[str]:
        """Board full name."""
        if "full_name" in self._raw_board:
            return str(self._raw_board["full_name"])
        return None

    @property
    def socs(self) -> List[str]:
        """SoC names."""
        socs = self._raw_board.get("socs", [])
        return [str(soc["name"]) for soc in socs if "name" in soc]

    @property
    def vendor(self) -> Optional[str]:
        """Vendor prefix."""
        if "vendor" in self._raw_board:
            return str(self._raw_board["vendor"])
        return None

    @property
    def revisions(self) -> List[str]:
        """Available board revisions."""
        revisions: HWMetaData.Content = self._raw_board.get("revision", {}).get(
            "revisions", []
        )
        return [str(rev["name"]) for rev in revisions if rev in "name"]

    @property
    def default_revision(self) -> Optional[str]:
        """Default board revision."""
        revision: HWMetaData.Content = self._raw_board.get("revision", {})
        return str(revision["default"]) if "default" in revision else None


class TestRunnerMetadata(HWMetaData):
    """Test runner (Twister) metadata.

    Content of:
    HWMv1: ${BOARD_DIR}/${BOARD}.yaml
    HWMv2: ${BOARD_DIR}/<board_name>_<qualifiers>.yaml
    """

    @property
    def name(self) -> Optional[str]:
        """Board name used by the test runner."""
        return str(self._raw["name"]) if "name" in self._raw else None

    @property
    def vendor(self) -> Optional[str]:
        """Vendor metadata (vendor prefix)."""
        return str(self._raw["vendor"]) if "vendor" in self._raw else None

    @property
    def run_arch(self) -> Optional[str]:
        """Architecture metadata (arm, xtensa, etc)."""
        return str(self._raw["arch"]) if "arch" in self._raw else None

    @property
    def run_type(self) -> Optional[str]:
        """Target type metadata (mcu, native or qemu)."""
        return str(self._raw["type"]) if "type" in self._raw else None


class DTShBoard:
    """Facade to Zephyr HWMv1 and HWMv2."""

    class HWM(Enum):
        """Hardware Model version."""

        UNKNOWN = "HWM unknown"
        V1 = "HWMv1"
        V2 = "HWMv2"

        @property
        def version(self) -> str:
            """HWM version string."""
            return self.value

    Qualifiers = Tuple[
        str, Optional[str], Optional[str], Optional[str], Optional[str]
    ]
    """Commodity for (name, version, soc, cpus, variant)."""

    RE_BOARD_HWM2 = re.compile(
        r"^(?P<name>[^@/\s]+)(?P<version>@[\d.]+)?(?P<soc>/[^/\s]*)?(?P<cpus>/[^/\s]+)?(?P<variant>/[^/\s]+)?$"
    )
    """Regular expression matching the HWMv2 BOARD target format.

    See:
    - https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html#board-terminology
    """

    # CMake cache variable identifying the board as a target
    # for the build-system.
    # This API will fail if unavailable
    CMAKE_BOARD = "BOARD"
    CMAKE_CACHED_BOARD = "CACHED_BOARD"

    # CMake cache variable giving the path to the main board directory.
    # for the build-system.
    # This API will fail if unavailable.
    CMAKE_BOARD_DIR = "BOARD_DIR"

    # CMake cache variable giving the path to SoC SVD (optional).
    CMAKE_SOC_SVD = "SOC_SVD_FILE"

    # CMake cache variable giving the path to the main SoC directory.
    # NOTE: Seems to be defined fo HWMv2 only (cached by soc_v2.cmake,
    # not by soc_v1.cmake).
    CMAKE_SOC_DIR = "SOC_FULL_DIR"

    # CMake cache variable giving the selected shield.
    CMAKE_SHIELD = "SHIELD"
    CMAKE_CACHED_SHIELD = "CACHED_SHIELD"

    @staticmethod
    def v2_parse_board(target: str) -> "DTShBoard.Qualifiers":
        """Parse HWMv2 BOARDs.

        Args:
            HWMv2 build target.

        Returns:
            (name, version, soc, cpus, variant)

        Raises:
            BoardError: Malformed BOARD target.
        """
        matched = DTShBoard.RE_BOARD_HWM2.match(target)
        if not matched:
            # The RE pattern should at least match a name.
            raise ValueError(target)

        tokens: Mapping[str, str] = matched.groupdict()
        name, version, soc, cpus, variant = (
            tokens["name"],
            tokens["version"],
            tokens["soc"],
            tokens["cpus"],
            tokens["variant"],
        )

        if version:
            # Trim leading "@".
            version = version[1:]
        if soc:
            # Trim leading "/",
            # may give an empty string when SoC is omitted (unique)
            soc = soc[1:]
        if cpus:
            # Trim leading "/".
            cpus = cpus[1:]
        if variant:
            # Trim leading "/".
            variant = variant[1:]

        return (name, version, soc, cpus, variant)

    @staticmethod
    def get_instance(cmake_cache: CMakeCache) -> Optional["DTShBoard"]:
        """Commodity for instantiating a board.

        Args:
            cmake_cache: The CMake cache to search.

        Returns:
            A board instance, or None if instantiation failed.
        """
        try:
            return DTShBoard(cmake_cache)
        except KeyError as e:
            print(f"dubious CMake cache: {e}", file=sys.stderr)
        except ValueError as e:
            print(f"dubious board: {e}", file=sys.stderr)
        return None

    # BOARD value used by Zephyr build-system.
    _target: str

    # Main board directory.
    # CMake: ${BOARD_DIR}
    _board_dir: Path

    # Zephyr Hardware Model.
    _hwm: HWM

    # HW description (Devicetree).
    # HWMv1: ${BOARD_DIR}/${BOARD}.dts
    # HWMv2: ${BOARD_DIR}/<board_name>_<qualifiers>.dts
    _dts: Path

    # Selected shield
    _shield: Optional[str] = None

    # YAML file with miscellaneous metadata used by the Test Runner (Twister).
    # HWMv1: ${BOARD_DIR}/${BOARD}.yaml
    # HWMv2: ${BOARD_DIR}/<board_name>_<qualifiers>.yaml
    _runner_metadata: TestRunnerMetadata

    # CMSIS-SVD file
    _soc_svd: Optional[Path] = None

    # YAML file describing the high-level meta data of the board.
    # HWMv2 only: ${BOARD_DIR}/board.yml
    _board_metadata: Optional[BoardMetadata] = None

    # HWMv2 only.
    _name: Optional[str] = None
    _revision: Optional[str] = None
    _soc: Optional[str] = None
    _cpus: Optional[str] = None
    _variant: Optional[str] = None
    # SoC directory (SOC_FULL_DIR defined only for HWMv2)
    _soc_dir: Optional[Path] = None
    # ${SOC_FULL_DIR}/soc.yml
    _soc_metadata: Optional[HWMetaData] = None

    def __init__(self, cmake_cache: CMakeCache) -> None:
        """Initialize board from CMake cache variables.

        Variables BOARD and BOARD_DIR must be defined.

        Args:
            cmake_cache: The CMake cache to search.
        """
        # Definitions independent from the HWM.
        self._target = self._init_board(cmake_cache)
        self._board_dir = self._init_board_dir(cmake_cache)
        self._soc_svd = self._get_soc_svd(cmake_cache)
        self._shield = self._get_shield(cmake_cache)

        self._hwm = self._get_hwm(cmake_cache)
        if self._hwm == DTShBoard.HWM.V2:
            self._init_board_v2(cmake_cache)
        else:
            self._init_board_v1()

    @property
    def target(self) -> str:
        """Board target used by the build-system (aka BOARD)."""
        return self._target

    @property
    def board_dir(self) -> Path:
        """Main board directory used by the build-system (aka BOARD_DIR)."""
        return self._board_dir

    @property
    def hwm(self) -> HWM:
        """Hardware Model version."""
        return self._hwm

    @property
    def dts_file(self) -> Path:
        """Absolute path to the board DTS file.

        HWMv1: ${BOARD_DIR}/board.dts
        HWMv2: ${BOARD_DIR}/<board>_<qualifiers>.dts
        """
        return self._dts

    @property
    def shield(self) -> Optional[str]:
        """Selected shield."""
        return self._shield

    @property
    def soc_svd(self) -> Optional[Path]:
        """Absolute path to the SoC SVD (SOC_SVD_FILE)."""
        return self._soc_svd

    @property
    def runner_metadata(self) -> TestRunnerMetadata:
        """YAML metadata used by the Test Runner (Twister).

        HWMv1: ${BOARD_DIR}/{BOARD}.yaml
        HWMv2: ${BOARD_DIR}/<board>_<qualifiers>.yaml
        """
        return self._runner_metadata

    @property
    def board_metadata(self) -> Optional[BoardMetadata]:
        """YAML metadata describing the high-level meta data
        of the board.

        HWMv2 only: ${BOARD_DIR}/board.yml
        """
        return self._board_metadata

    @property
    def soc_metadata(self) -> Optional[HWMetaData]:
        """YAML metadata describing the SoC.

        HWMv2 only: ${SOC_FULL_DIR}/soc.yml
        """
        return self._soc_metadata

    @property
    def full_name(self) -> Optional[str]:
        """Board full name retrieved from the board metadata (HWMv2 only)."""
        return self._board_metadata.full_name if self._board_metadata else None

    @property
    def runner_name(self) -> Optional[str]:
        """Board name retrieved from the test runner metadata."""
        return self._runner_metadata.name

    @property
    def name(self) -> Optional[str]:
        """HWMv2 board name."""
        return self._name

    @property
    def revision(self) -> Optional[str]:
        """HWMv2 board revision.

        If unset, the default revision is retrieved from the board metadata.
        """
        return self._revision

    @property
    def soc(self) -> Optional[str]:
        """HWMv2 SoC.

        If the SoC can't be parsed from the BOARD target,
        assuming the board should then support a unique SoC,
        it's retrieved from the board metadata, if available.
        """
        return self._soc

    @property
    def cpus(self) -> Optional[str]:
        """HWMv2 CPU cluster."""
        return self._cpus

    @property
    def variant(self) -> Optional[str]:
        """HWMv2 variant."""
        return self._variant

    @property
    def soc_dir(self) -> Optional[Path]:
        """Absolute path to the SoC main directory (aka SOC_FULL_DIR)."""
        return self._soc_dir

    @property
    def qualifiers(self) -> str:
        """Coma separated list of the HWMv2 qualifiers."""
        qualifiers: List[str] = [
            q for q in (self._soc, self._cpus, self._variant) if q is not None
        ]
        return ",".join(qualifiers)

    def _get_hwm(self, cmake_cache: CMakeCache) -> HWM:
        if "/" in self._target:
            # HWMv2 target.
            return DTShBoard.HWM.V2

        if self._board_dir.is_dir():
            path = self._board_dir / "board.yml"
            if path.is_file():
                # HWMv2 metadata file.
                return DTShBoard.HWM.V2
            return DTShBoard.HWM.V1

        if DTShBoard.CMAKE_SOC_DIR in cmake_cache:
            # SOC_FULL_DIR defined only in cmake/modules/soc_v2.cmake.
            return DTShBoard.HWM.V2

        return DTShBoard.HWM.UNKNOWN

    def _init_board_v1(self) -> None:
        # ${BOARD_DIR}/${BOARD}.dts
        self._dts = self._board_dir / f"{self._target}.dts"
        # ${BOARD_DIR}/{BOARD}.yaml
        self._runner_metadata = TestRunnerMetadata(
            self._board_dir / f"{self._target}.yaml"
        )

    def _init_board_v2(self, cmake_cache: CMakeCache) -> None:
        # Independent from qualifiers.
        # ${BOARD_DIR}/board.yml
        self._board_metadata = self._v2_get_board_metadata()
        # ${SOC_FULL_DIR}
        self._soc_dir = self._v2_get_soc_dir(cmake_cache)
        # ${SOC_FULL_DIR}/soc.yml
        self._soc_metadata = self._v2_get_soc_metadata()

        (
            self._name,
            self._revision,
            self._soc,
            self._cpus,
            self._variant,
        ) = self._v2_get_board_qualifiers()

        # Depend on qualifiers.
        # ${BOARD_DIR}/<board_name>_<qualifiers>.dts
        self._dts = self._v2_get_dts()
        # ${BOARD_DIR}/<board_name>_<qualifiers>.yaml
        self._runner_metadata = self._v2_get_runner_metadata()

    def _init_board(self, cmake_cache: CMakeCache) -> str:
        board = cmake_cache.getstr(DTShBoard.CMAKE_BOARD) or cmake_cache.getstr(
            DTShBoard.CMAKE_CACHED_BOARD
        )
        if board:
            return board
        raise KeyError("BOARD")

    def _init_board_dir(self, cmake_cache: CMakeCache) -> Path:
        path = cmake_cache.getstr(DTShBoard.CMAKE_BOARD_DIR)
        if path:
            return Path(path).absolute()
        raise KeyError("BOARD_DIR")

    def _get_soc_svd(self, cmake_cache: CMakeCache) -> Optional[Path]:
        path = cmake_cache.getstr(DTShBoard.CMAKE_SOC_SVD)
        if path:
            return Path(path).absolute()
        return None

    def _get_shield(self, cmake_cache: CMakeCache) -> Optional[str]:
        return cmake_cache.getstr(DTShBoard.CMAKE_SHIELD) or cmake_cache.getstr(
            DTShBoard.CMAKE_CACHED_SHIELD
        )

    def _v2_get_soc_dir(self, cmake_cache: CMakeCache) -> Optional[Path]:
        path = cmake_cache.getstr(DTShBoard.CMAKE_SOC_DIR)
        if path:
            return Path(path).absolute()
        return None

    def _v2_get_board_metadata(self) -> BoardMetadata:
        path: Path = self._board_dir / "board.yml"
        return BoardMetadata(path)

    def _v2_get_soc_metadata(
        self,
    ) -> Optional[HWMetaData]:
        if self._soc_dir:
            path = self._soc_dir / "soc.yml"
            return HWMetaData(path)
        return None

    def _v2_get_dts(self) -> Path:
        return self._v2_find_board_file("dts")

    def _v2_get_runner_metadata(self) -> TestRunnerMetadata:
        path: Path = self._v2_find_board_file("yaml")
        return TestRunnerMetadata(path)

    def _v2_get_board_qualifiers(self) -> Qualifiers:
        name: Optional[str] = None
        revision: Optional[str] = None
        soc: Optional[str] = None
        cpus: Optional[str] = None
        variant: Optional[str] = None
        (name, revision, soc, cpus, variant) = DTShBoard.v2_parse_board(
            self._target
        )

        if self._board_metadata:
            if not soc:
                # Unique SoC omitted, trying meta-data file.
                socs: List[str] = self._board_metadata.socs
                if len(socs) == 1:
                    soc = socs[0]
            if not revision:
                revision = self._board_metadata.default_revision

        return (name, revision, soc, cpus, variant)

    def _v2_find_board_file(self, ext: str) -> Path:
        # Find a file with name ${BOARD_DIR}/<board_name>_<qualifiers>.<ext>,
        # but we're unsure about which qualifiers are present.
        qualifiers: str = self._flat_qualifiers()
        if qualifiers:
            # First, try <board_name>_<qualifiers>.<ext> with all qualifiers
            # we have, e.g. <board_name>_<soc>_<cpus>_<variant>.<ext>.
            path = self._board_dir / f"{self._name}_{qualifiers}.{ext}"
            if not path.is_file():
                # Try with the <soc> omitted, possible when the board
                # supports only one SoC.
                qualifiers = self._flat_qualifiers(strip_soc=True)
                if qualifiers:
                    path = self._board_dir / f"{self._name}_{qualifiers}.{ext}"
            if path.is_file():
                return path

        # As last chances:
        # - Try <board_name>.<ext>
        path = self._board_dir / f"{self._name}.{ext}"
        if not path.is_file():
            # - Or simply the BOARD target with the "/" replaced by "_"
            path = self._board_dir / f"{self._target.replace('/', '_')}.{ext}"

        # Return the last path we tried.
        return path

    def _flat_qualifiers(self, strip_soc: bool = False) -> str:
        qualifiers: List[str] = [
            q for q in (self._soc, self._cpus, self._variant) if q is not None
        ]
        if strip_soc and self._soc:
            qualifiers = qualifiers[1:]
        return "_".join(qualifiers)

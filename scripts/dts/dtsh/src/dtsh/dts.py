# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0

"""Devicetree source definition.

A devicetree is fully defined by:
- a DTS file in Devicetree source format (DTSpec 6)
- all of the YAML binding files the DTS recursively depends on
- the Devicetree Specification

This module also introduces a YAML file system API
that should cover the devicetree shell needs:
- access the DT binding files with their base name
- recursively access YAML-included bindings
- the name2path semantic expected for edtlib.Binding objects initialization

Unit tests and examples: tests/test_dtsh_dts.py
"""


from typing import cast, Optional, List, Sequence

import os

from dtsh.hwm import DTShBoard
from dtsh.utils import CMakeCache, GitUtil, YAMLFilesystem, DTShToolchain


class DTS:
    """Devicetree source definition.

    A devicetree source is defined by:
    - a DTS file in Devicetree source format (DTSpec 6)
    - all of the YAML binding files the DTS recursively depends on;
      the list of directories to search for the YAML files is
      termed "bindings search path"
    - the Zephyr hardware model

    When not explicitly set, a default bindings search path
    can be retrieved, or worked out, based on:
    - cached CMake variables: this is the preferred method,
      assuming the cache file was produced by the same build as the DTS file
    - environment variables: less reliable than the CMake cache,
      since their *current* values are not bound to the build that
      produced the DTS file
    - assumptions on files layout, e.g. the CMake cache file is expected
      to be named CMakeCache.txt, and located in the grand-parent directory
      of where the DTS file itself is located (as shown bellow)

    The CMake cache file is searched for according to the typical
    Zephyr build layout:

        build/
        ├── CMakeCache.txt
        └── zephyr/
            └── zephyr.dts

    The hardware model is determined whith a simple heuristic based
    on the board target fomat and the existence of some files or directories.

    NOTE: Many other CMake variables could help us if they were cache,
    e.g. from the cmake/modules/soc_{v1,v2}.cmake files:

        if(HWMv2)
            set(SOC_NAME   ${CONFIG_SOC})
            set(SOC_SERIES ${CONFIG_SOC_SERIES})
            set(SOC_TOOLCHAIN_NAME ${CONFIG_SOC_TOOLCHAIN_NAME})
    """

    # See path().
    _dts_path: str

    # See bindings_search_path().
    _binding_dirs: List[str]

    # See zephyr_base().
    _zephyr_base: Optional[str]

    # See vendors_file().
    _vendors_file: Optional[str]

    _cmake: Optional["CMakeCache"]
    _yamlfs: "YAMLFilesystem"

    # Board/SoC and hardware model.
    _board: Optional[DTShBoard] = None

    # Toolchain used at build-time.
    _toolchain: Optional[DTShToolchain] = None

    def __init__(
        self,
        dts_path: str,
        binding_dirs: Optional[Sequence[str]] = None,
        vendors_file: Optional[str] = None,
    ) -> None:
        """Define a devicetree source.

        Args:
            dts_path: Path to the devicetree source file.
            binding_dirs: The list of directories to search for the
              YAML binding files the DTS depends on.
              If not set, the default bindings search path is assumed.
            vendors_file: Path to a file in vendor-prefixes.txt format.
        """
        self._dts_path = os.path.abspath(dts_path)
        self._cmake = self._init_cmake_cache()
        self._zephyr_base = self._init_zephyr_base()
        self._binding_dirs = self._init_binding_dirs(binding_dirs)
        self._vendors_file = self._init_vendors_file(vendors_file)
        self._yamlfs = YAMLFilesystem(self._binding_dirs)

        if self._cmake:
            self._board = DTShBoard.get_instance(self._cmake)
            self._toolchain = DTShToolchain.get_instance(self._cmake)

    @property
    def path(self) -> str:
        """Path to the DTS file."""
        return self._dts_path

    @property
    def vendors_file(self) -> Optional[str]:
        "Path to the vendors file." ""
        return self._vendors_file

    @property
    def bindings_search_path(self) -> Sequence[str]:
        """Directories where the YAML binding files are located.

        Either:

        - set explicitly when defining the DT source
        - or retrieved from the CMake cached variable CACHED_DTS_ROOT_BINDINGS
        - or worked out (*best effort*) according to "Where bindings are located"

        In the later case, depending on defined environment variables
        or cached CMake variables, the bindings search path should contain
        the dts/bindings sub-directories of:

        - the zephyr repository
        - the application source directory
        - any custom board directory
        - shield directories

        This search path is not expunged from non existing directories:
        it's intended to represent the *considered* directories.

        Refer to zephyr/cmake/modules/dts.cmake for the DTS_ROOT
        and DTS_ROOT_BINDINGS values set at build-time:

            list(APPEND
              DTS_ROOT
              ${APPLICATION_SOURCE_DIR}
              ${BOARD_DIR}
              ${SHIELD_DIRS}
              ${ZEPHYR_BASE}
              )

            foreach(dts_root ${DTS_ROOT})
              set(bindings_path ${dts_root}/dts/bindings)
              if(EXISTS ${bindings_path})
                list(APPEND
                  DTS_ROOT_BINDINGS
                  ${bindings_path}
                  )
              endif()

        """
        return self._binding_dirs

    @property
    def yamlfs(self) -> "YAMLFilesystem":
        """YAML bindings search support."""
        return self._yamlfs

    @property
    def board(self) -> Optional[DTShBoard]:
        """Hardware information (board, SoC, HWM)."""
        return self._board

    @property
    def app_binary_dir(self) -> str:
        """Application binary directory (aka build directory).

        Derived from the DTS file path.
        """
        return os.path.dirname(os.path.dirname(self._dts_path))

    @property
    def app_source_dir(self) -> Optional[str]:
        """Application source directory (aka project directory).

        Retrieved from the CMake cache (APPLICATION_SOURCE_DIR).
        """
        if self._cmake:
            return self._cmake.getstr("APPLICATION_SOURCE_DIR")
        return None

    @property
    def app_conf_file(self) -> Optional[str]:
        """Application configuration file.

        Retrieved from the CMake cache (CONF_FILE),
        """
        if self._cmake:
            return self._cmake.getstr("CONF_FILE")
        return None

    @property
    def fw_name(self) -> Optional[str]:
        """Application name.

        Retrieved from the CMake cache (CMAKE_PROJECT_NAME).
        """
        if self._cmake:
            return self._cmake.getstr("CMAKE_PROJECT_NAME")
        return None

    @property
    def fw_version(self) -> Optional[str]:
        """Application version.

        Retrieved from the CMake cache (CMAKE_PROJECT_VERSION).
        """
        if self._cmake:
            return self._cmake.getstr("CMAKE_PROJECT_VERSION")
        return None

    @property
    def zephyr_base(self) -> Optional[str]:
        """Path to Zephyr repository.

        Either:

        - retrieved from the CMake cache (ZEPHYR_BASE)
        - or retrieved from the shell environment (ZEPHYR_BASE)
        - or set according the expected file layout ZEPHYR_BASE/scripts/dts/dtsh
        - or unvailable
        """
        return self._zephyr_base

    @property
    def toolchain(self) -> Optional[DTShToolchain]:
        """The toolchain used at build-time.

        Retrieved from the CMake cache.
        """
        return self._toolchain

    def get_zephyr_head(self) -> Optional[str]:
        """Retrieve Zephyr repository HEAD version.

        Returns:
            A version string with at least the short commit hash,
            and a tag name if HEAD is tagged.
            Depends on the git command availability.
        """
        if self.zephyr_base:
            git = GitUtil(self.zephyr_base)
            if git.is_available:
                head_short = git.head_get_short()
                head_tag = git.head_get_tag()
                if head_tag:
                    return f"{head_tag} ({head_short})"
                return head_short
        return None

    def _init_cmake_cache(self) -> Optional["CMakeCache"]:
        # Is the CMake cache available at ../CMakeCache.txt from the DTS file ?
        path = os.path.join(self.app_binary_dir, "CMakeCache.txt")
        if os.path.isfile(path):
            return CMakeCache.open(path)
        return None

    def _init_zephyr_base(self) -> Optional[str]:
        if self._cmake:
            zephyr_base = self._cmake.getstr("ZEPHYR_BASE")
        else:
            zephyr_base = os.environ.get("ZEPHYR_BASE")
        if not zephyr_base:
            # DTSh may be distributed with the Zephyr project:
            # test ZEPHYR_BASE/scripts/dts/dtsh/src/dtsh/__file__
            dtshdir = os.path.dirname(
                os.path.dirname(os.path.dirname(__file__))
            )
            testdir = os.path.dirname(os.path.dirname(os.path.dirname(dtshdir)))
            # ZEPHYR_BASE/Kconfig.zephyr should then exist.
            if os.path.isfile(os.path.join(testdir, "Kconfig.zephyr")):
                zephyr_base = testdir

        return zephyr_base

    def _init_vendors_file(self, vendors_file: Optional[str]) -> Optional[str]:
        if not vendors_file:
            if self._zephyr_base:
                # If we have a valid ZEPHYR_BASE, we assume the expected vendors
                # are those defined by Zephyr.
                vendors_file = os.path.join(
                    self._zephyr_base, "dts", "bindings", "vendor-prefixes.txt"
                )
            else:
                # Otherwise, it's very likely the user has explicitly set
                # some binding directories, search them for a vendors file.
                # NOTE: Linux uses a vendor-prefixes.yaml file, we should be
                # able to also load that (requires changes in edtlib.EDT).
                for binding_dir in self._binding_dirs:
                    vndpath = os.path.join(binding_dir, "vendor-prefixes.txt")
                    if os.path.isfile(vndpath):
                        vendors_file = vndpath
        return vendors_file

    def _init_binding_dirs(
        self, binding_dirs: Optional[Sequence[str]]
    ) -> List[str]:
        if binding_dirs:
            binding_dirs = [os.path.abspath(path) for path in binding_dirs]
        else:
            if self._cmake:
                binding_dirs = self._cmake.getstrs("CACHED_DTS_ROOT_BINDINGS")
            if not binding_dirs:
                # Fallback to "Where bindings are located".
                # NOTE: Possible missing DTS roots ?
                #
                # - any directories manually included in the
                #   DTS_ROOT CMake variable
                # - any module that defines a dts_root in its Build settings
                #
                # IIUC, this might be a non-issue, since either the CMake cache:
                # - is available, and we should be able to get all bindings
                #   from the CACHED_DTS_ROOT_BINDINGS value
                # - is unavailable, and we won't access any build settings
                dts_roots: List[Optional[str]] = [
                    self.app_source_dir,
                    self.zephyr_base,
                ]
                if self._board:
                    dts_roots.append(str(self._board.board_dir))

                binding_dirs = [
                    os.path.join(dtsroot, "dts", "bindings")
                    for dtsroot in dts_roots
                    if dtsroot
                ]
        # cast() is required to avoid type hinting error since
        # binding_dirs is first typed as an optional Sequence.
        return cast(List[str], binding_dirs)


class DTSFile:
    """Simple fail-safe wrapper around a DTS file."""

    # Absolute file path.
    _path: str

    # DTS.
    _content: str

    # If set, we've failed to load the DTS file (IO error).
    _lasterr: Optional[OSError]

    def __init__(self, path: str) -> None:
        """Initialize wrapper.

        Open DTS file and read content, no lazy-initialization.

        Args:
            path: Absolute path to the DTS file.
              Invalid path or file will produce an empty content.
        """
        self._path = path
        self._lasterr = None
        self._content = self._init_content()

    @property
    def path(self) -> str:
        """Absolute file path."""
        return self._path

    @property
    def content(self) -> str:
        """Text content, or None if we failed to read the DTS file."""
        return self._content

    @property
    def lasterr(self) -> Optional[OSError]:
        """Last error that happened while loading this DTS file.

        Possible values:

        - None: no error
        - OSError: IO error
        """
        return self._lasterr

    def _init_content(self) -> str:
        try:
            with open(self._path, mode="r", encoding="utf-8") as f:
                return f.read().strip()
        except OSError as e:
            self._lasterr = e
        # Empty content on error.
        return ""

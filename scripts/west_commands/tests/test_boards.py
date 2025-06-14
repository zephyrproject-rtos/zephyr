# Copyright (c) 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys
from unittest import mock

import pytest

# Add the parent directory to sys.path to import the module being tested
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

# Import the module being tested
from boards import Boards


class MockPlatform:
    """Mock for twister platform objects."""

    def __init__(self, name, aliases=None, arch="arm", twister=True, vendor="test_vendor"):
        self.name = name
        self.aliases = aliases or [name]
        self.arch = arch
        self.twister = twister
        self.vendor = vendor

        # Setup board attribute
        self.board = mock.MagicMock()
        self.board.name = name.split('/')[0]
        self.board.full_name = f"{name} Board"
        self.board.revision_default = "0.1.0"
        self.board.dir = f"/path/to/{name}"
        self.board.hwm = "hwm"

        # Setup revisions based on the name
        if "rev" in name:
            # Properly create mock revisions with string name attribute
            self.board.revisions = []
            for i in range(1, 3):
                revision = mock.MagicMock()
                revision.name = f"rev{i}"  # Set name as a string attribute
                self.board.revisions.append(revision)
        else:
            self.board.revisions = []


@pytest.fixture
def mock_generate_platforms():
    """Fixture to create mock platform generator."""
    with mock.patch('boards.generate_platforms') as mock_gen:
        # Create sample test platforms
        platforms = [
            MockPlatform("board1", aliases=["board1", "b1"]),
            MockPlatform("board2", aliases=["board2", "b2"], arch="x86"),
            MockPlatform("board3/rev", aliases=["board3", "b3"], arch="riscv"),
            MockPlatform("board4", aliases=["board4"], twister=False),  # Should be filtered out
            MockPlatform("arduino_uno", aliases=["arduino_uno", "uno"], vendor="arduino"),
            MockPlatform("board5/qualifier", aliases=["board5"], arch="arm"),
        ]
        mock_gen.return_value = platforms
        yield mock_gen


@pytest.fixture
def mock_zephyr_module():
    """Fixture to mock zephyr module parsing."""
    with mock.patch('boards.zephyr_module.parse_modules') as mock_parse:
        mock_module = mock.MagicMock()
        mock_module.meta.get.return_value = {"board_root": "test/board/root"}
        mock_parse.return_value = [mock_module]
        yield mock_parse


@pytest.fixture
def boards_cmd():
    """Fixture to create a Boards command instance with mocked West infrastructure."""
    with mock.patch('boards.Boards.manifest', new_callable=mock.PropertyMock) as mock_manifest:
        mock_manifest_obj = mock.MagicMock()
        mock_manifest.return_value = mock_manifest_obj

        cmd = Boards()
        # Mock other West command infrastructure that might cause recursion
        cmd.config = mock.MagicMock()
        cmd.inf = mock.MagicMock()  # Mock the output method
        cmd.die = mock.MagicMock()  # Mock the die method
        cmd.err = mock.MagicMock()  # Mock the err method

        yield cmd


class TestBoardsCommand:
    """Unit tests for the 'west boards' command."""

    def test_basic_command(self, boards_cmd, mock_generate_platforms, mock_zephyr_module):
        """Test basic board listing functionality."""
        args = mock.MagicMock()
        args.format = "{name}"
        args.name_re = None
        args.arch_roots = []
        args.board_roots = []
        args.soc_roots = []

        # Directly mock the parse_modules call to avoid West manifest issues
        with mock.patch('boards.zephyr_module.parse_modules', return_value=[]):
            boards_cmd.do_run(args, None)

        # Check that inf was called for each valid board
        assert boards_cmd.inf.call_count == 5  # 5 boards with twister=True

        # Check that each board name was output
        expected_boards = ["board1", "board2", "board3", "arduino_uno", "board5"]
        for board in expected_boards:
            boards_cmd.inf.assert_any_call(board)

    def test_name_filter(self, boards_cmd, mock_generate_platforms, mock_zephyr_module):
        """Test filtering boards by name with regex."""
        args = mock.MagicMock()
        args.format = "{name}"
        args.name_re = "arduino"
        args.arch_roots = []
        args.board_roots = []
        args.soc_roots = []

        # Directly mock the parse_modules call to avoid West manifest issues
        with mock.patch('boards.zephyr_module.parse_modules', return_value=[]):
            boards_cmd.do_run(args, None)

        # Should only output arduino boards
        assert boards_cmd.inf.call_count == 1
        boards_cmd.inf.assert_called_once_with("arduino_uno")

    def test_custom_format(self, boards_cmd, mock_generate_platforms, mock_zephyr_module):
        """Test custom format string output."""
        args = mock.MagicMock()
        args.format = "{name}:{vendor}:{arch}"
        args.name_re = None
        args.arch_roots = []
        args.board_roots = []
        args.soc_roots = []

        # Directly mock the parse_modules call to avoid West manifest issues
        with mock.patch('boards.zephyr_module.parse_modules', return_value=[]):
            boards_cmd.do_run(args, None)

        # Check that the format is applied correctly
        boards_cmd.inf.assert_any_call("board1:test_vendor:arm")
        boards_cmd.inf.assert_any_call("board2:test_vendor:x86")
        boards_cmd.inf.assert_any_call("arduino_uno:arduino:arm")

    def test_board_with_revisions(self, boards_cmd, mock_generate_platforms, mock_zephyr_module):
        """Test boards with revisions are shown correctly."""
        args = mock.MagicMock()
        args.format = "{name}:{revisions}"
        args.name_re = None
        args.arch_roots = []
        args.board_roots = []
        args.soc_roots = []

        # Directly mock the parse_modules call to avoid West manifest issues
        with mock.patch('boards.zephyr_module.parse_modules', return_value=[]):
            boards_cmd.do_run(args, None)

        # board3 should have revisions
        boards_cmd.inf.assert_any_call("board3:rev1 rev2")

        # Other boards should have None for revisions
        for board in ["board1", "board2", "arduino_uno", "board5"]:
            boards_cmd.inf.assert_any_call(f"{board}:None")

    def test_invalid_format_string(
        self, boards_cmd, mock_generate_platforms, mock_zephyr_module, capsys
    ):
        """Test handling of invalid format strings."""
        args = mock.MagicMock()
        args.format = "{name}:{invalid}"  # Invalid format key
        args.name_re = None
        args.arch_roots = []
        args.board_roots = []
        args.soc_roots = []

        # Directly mock the parse_modules call to avoid West manifest issues
        with mock.patch('boards.zephyr_module.parse_modules', return_value=[]):
            boards_cmd.do_run(args, None)

        # Check error message was printed
        captured = capsys.readouterr()
        assert "KeyError" in captured.out
        assert "use 'west boards -h'" in captured.out

    def test_no_boards_found(self, boards_cmd, mock_zephyr_module, capsys):
        """Test handling when no boards match criteria."""
        args = mock.MagicMock()
        args.format = "{name}"
        args.name_re = "nonexistent"
        args.arch_roots = []
        args.board_roots = []
        args.soc_roots = []

        # Mock empty platform list and parse_modules to avoid West manifest issues
        with mock.patch('boards.generate_platforms', return_value=[]):  # noqa: SIM117
            with mock.patch('boards.zephyr_module.parse_modules', return_value=[]):
                boards_cmd.do_run(args, None)

        # Check no boards message
        captured = capsys.readouterr()
        assert "no board found" in captured.out

    def test_qualifiers(self, boards_cmd, mock_generate_platforms, mock_zephyr_module):
        """Test board qualifiers are correctly processed."""
        args = mock.MagicMock()
        args.format = "{name}:{qualifiers}"
        args.name_re = None
        args.arch_roots = []
        args.board_roots = []
        args.soc_roots = []

        # Directly mock the parse_modules call to avoid West manifest issues
        with mock.patch('boards.zephyr_module.parse_modules', return_value=[]):
            boards_cmd.do_run(args, None)

        # Check boards with qualifiers
        boards_cmd.inf.assert_any_call("board3:rev")
        boards_cmd.inf.assert_any_call("board5:qualifier")

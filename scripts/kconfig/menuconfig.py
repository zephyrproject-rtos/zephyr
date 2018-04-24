#!/usr/bin/env python3

"""
Overview
========

A curses-based menuconfig implementation. The interface should feel familiar to
people used to mconf ('make menuconfig').

Supports the same keys as mconf, and also supports a set of keybindings
inspired by Vi:

  J/K     : Down/Up
  L       : Enter menu/Toggle item
  H       : Leave menu
  Ctrl-D/U: Page Down/Page Down
  G/End   : Jump to end of list
  g/Home  : Jump to beginning of list

The mconf feature where pressing a key jumps to a menu entry with that
character in it in the current menu isn't supported. A search feature with a
"jump to" function for jumping directly to a particular symbol regardless of
where it is defined will be added later instead.

Space and Enter are "smart" and try to do what you'd expect for the given
menu entry.


Running
=======

menuconfig.py can be run either as a standalone executable or by calling the
menu.menuconfig() function with an existing Kconfig instance. The second option
is a bit inflexible in that it will still load and save .config, etc.

When run in standalone mode, the top-level Kconfig file to load can be passed
as a command-line argument. With no argument, it defaults to "Kconfig".

The KCONFIG_CONFIG environment variable specifies the .config file to load (if
it exists) and save. If KCONFIG_CONFIG is unset, ".config" is used.

$srctree is supported through Kconfiglib.


Other features
==============

  - Seamless terminal resizing

  - No dependencies on *nix, as the 'curses' module is in the Python standard
    library

  - Unicode text entry

  - Improved information screen compared to mconf:

      * Expressions are split up by their top-level &&/|| operands to improve
        readability

      * Undefined symbols in expressions are pointed out

      * Menus and comments have information displays

      * Kconfig definitions are printed


Limitations
===========

  - Python 3 only

    This is mostly due to Python 2 not having curses.get_wch(), which is needed
    for Unicode support.

  - Doesn't work out of the box on Windows

    Has been tested to work with the wheels provided at
    https://www.lfd.uci.edu/~gohlke/pythonlibs/#curses though.
"""


#
# Configuration variables
#

# Number of steps for Page Up/Down to jump
_PG_JUMP = 6

# How far the cursor needs to be from the edge of the window before it starts
# to scroll
_SCROLL_OFFSET = 5

# Minimum width of dialogs that ask for text input
_INPUT_DIALOG_MIN_WIDTH = 30

# Number of arrows pointing up/down to draw when a window is scrolled
_N_SCROLL_ARROWS = 14

# Lines of help text shown at the bottom of the "main" display
_MAIN_HELP_LINES = """
[Space/Enter] Toggle/enter   [ESC] Leave menu   [S] Save
[M] Save minimal config      [?] Symbol info    [Q] Quit (prompts for save)
"""[1:-1].split("\n")

# Lines of help text shown at the bottom of the information display
_INFO_HELP_LINES = """
[ESC/q] Return to menu
"""[1:-1].split("\n")

def _init_styles():
    global _PATH_STYLE
    global _TOP_SEP_STYLE
    global _MENU_LIST_STYLE
    global _MENU_LIST_SEL_STYLE
    global _BOT_SEP_STYLE
    global _HELP_STYLE

    global _DIALOG_FRAME_STYLE
    global _DIALOG_BODY_STYLE
    global _INPUT_FIELD_STYLE

    global _INFO_TOP_LINE_STYLE
    global _INFO_TEXT_STYLE
    global _INFO_BOT_SEP_STYLE
    global _INFO_HELP_STYLE

    # Initialize styles for different parts of the application. The arguments
    # are ordered as follows:
    #
    #   1. Text color
    #   2. Background color
    #   3. Attributes
    #   4. Extra attributes if colors aren't available. The colors will be
    #      ignored in this case, and the attributes from (3.) and (4.) will be
    #      ORed together.

    # A_BOLD tends to produce faint and hard-to-read text on the Windows
    # console, especially with the old color scheme, before the introduction of
    # https://blogs.msdn.microsoft.com/commandline/2017/08/02/updating-the-windows-console-colors/
    BOLD = curses.A_NORMAL if platform.system() == "Windows" else curses.A_BOLD


    # Top row, with menu path
    _PATH_STYLE          = _style(curses.COLOR_BLACK, curses.COLOR_WHITE,  BOLD                              )

    # Separator below menu path, with title and arrows pointing up
    _TOP_SEP_STYLE       = _style(curses.COLOR_BLACK, curses.COLOR_YELLOW, BOLD,            curses.A_STANDOUT)

    # The "main" menu display with the list of symbols, etc.
    _MENU_LIST_STYLE     = _style(curses.COLOR_BLACK, curses.COLOR_WHITE,  curses.A_NORMAL                   )

    # Selected menu entry
    _MENU_LIST_SEL_STYLE = _style(curses.COLOR_WHITE, curses.COLOR_BLUE,   curses.A_NORMAL, curses.A_STANDOUT)

    # Row below menu list, with arrows pointing down
    _BOT_SEP_STYLE       = _style(curses.COLOR_BLACK, curses.COLOR_YELLOW, BOLD,            curses.A_STANDOUT)

    # Help window with keys at the bottom
    _HELP_STYLE          = _style(curses.COLOR_BLACK, curses.COLOR_WHITE,  BOLD                              )


    # Frame around dialog boxes
    _DIALOG_FRAME_STYLE  = _style(curses.COLOR_BLACK, curses.COLOR_YELLOW, BOLD,            curses.A_STANDOUT)

    # Body of dialog boxes
    _DIALOG_BODY_STYLE   = _style(curses.COLOR_WHITE, curses.COLOR_BLACK,  curses.A_NORMAL                   )

    # Text input field in dialog boxes
    _INPUT_FIELD_STYLE   = _style(curses.COLOR_WHITE, curses.COLOR_BLUE,   curses.A_NORMAL, curses.A_STANDOUT)


    # Top line of information display, with title and arrows pointing up
    _INFO_TOP_LINE_STYLE = _style(curses.COLOR_BLACK, curses.COLOR_YELLOW, BOLD,            curses.A_STANDOUT)

    # Main information display window
    _INFO_TEXT_STYLE     = _style(curses.COLOR_BLACK, curses.COLOR_WHITE,  curses.A_NORMAL                   )

    # Separator below information display, with arrows pointing down
    _INFO_BOT_SEP_STYLE  = _style(curses.COLOR_BLACK, curses.COLOR_YELLOW, BOLD,            curses.A_STANDOUT)

    # Help window with keys at the bottom of the information display
    _INFO_HELP_STYLE     = _style(curses.COLOR_BLACK, curses.COLOR_WHITE,  BOLD                              )


#
# Main application
#

from kconfiglib import Kconfig, \
                       Symbol, Choice, MENU, COMMENT, \
                       BOOL, TRISTATE, STRING, INT, HEX, UNKNOWN, \
                       AND, OR, NOT, \
                       expr_value, split_expr, \
                       TRI_TO_STR, TYPE_TO_STR

# We need this double import for the _expr_str() override below
import kconfiglib

import curses
import errno
import locale
import os
import platform
import sys
import textwrap


# Color pairs we've already created, indexed by a
# (<foreground color>, <background color>) tuple
_color_attribs = {}

def _style(fg_color, bg_color, attribs, no_color_extra_attribs=0):
    # Returns an attribute with the specified foreground and background color
    # and the attributes in 'attribs'. Reuses color pairs already created if
    # possible, and creates a new color pair otherwise.
    #
    # Returns 'attribs | no_color_extra_attribs' if colors aren't supported.

    global _color_attribs

    if not curses.has_colors():
        return attribs | no_color_extra_attribs

    if (fg_color, bg_color) not in _color_attribs:
        # Create new color pair. Color pair number 0 is hardcoded and cannot be
        # changed, hence the +1s.
        curses.init_pair(len(_color_attribs) + 1, fg_color, bg_color)
        _color_attribs[(fg_color, bg_color)] = \
            curses.color_pair(len(_color_attribs) + 1)

    return _color_attribs[(fg_color, bg_color)] | attribs

# "Extend" the standard kconfiglib.expr_str() to show values for symbols
# appearing in expressions, for the information display.
#
# This is a bit hacky, but officially supported. It beats having to reimplement
# expression printing just to tweak it a bit.

def _expr_str_val(expr):
    if isinstance(expr, Symbol) and not expr.is_constant and \
       not _is_num(expr.name):
        # Show the values of non-constant (non-quoted) symbols that don't look
        # like numbers. Things like 123 are actually a symbol references, and
        # only work as expected due to undefined symbols getting their name as
        # their value. Showing the symbol value there isn't helpful though.

        if not expr.nodes:
            # Undefined symbol reference
            return "{}(undefined/n)".format(expr.name)

        return '{}(="{}")'.format(expr.name, expr.str_value)

    if isinstance(expr, tuple) and expr[0] == NOT and \
       isinstance(expr[1], Symbol):

        # Put a space after "!" before a symbol, since '! FOO(="y")' makes it
        # clearer than '!FOO(="y")' that "y" is the value of FOO itself
        return "! " + _expr_str(expr[1])

    # We'll end up back in _expr_str_val() when _expr_str_orig() does recursive
    # calls for subexpressions
    return _expr_str_orig(expr)

# Do hacky expr_str() extension. The rest of the code will just call
# _expr_str().
_expr_str_orig = kconfiglib.expr_str
kconfiglib.expr_str = _expr_str_val
_expr_str = _expr_str_val

def menuconfig(kconf):
    """
    Launches the configuration interface, returning after the user exits.

    kconf:
      Kconfig instance to be configured
    """

    globals()["_kconf"] = kconf
    global _config_filename


    _config_filename = os.environ.get("KCONFIG_CONFIG")
    if _config_filename is None:
        _config_filename = ".config"

    if os.path.exists(_config_filename):
        print("Using existing configuration '{}' as base"
              .format(_config_filename))
        _kconf.load_config(_config_filename)
    elif kconf.defconfig_filename is not None:
        print("Using default configuration found in '{}' as base"
              .format(kconf.defconfig_filename))
        _kconf.load_config(kconf.defconfig_filename)
    else:
        print("Using default symbol values as base")


    # We rely on having a selected node
    if not _visible_nodes(_kconf.top_node):
        print("No visible symbols in the top menu -- nothing to configure.\n"
              "Check that environment variables are set properly.")
        return

    # Disable warnings. They get mangled in curses mode, and we deal with
    # errors ourselves.
    _kconf.disable_warnings()

    # Make sure curses uses the encoding specified in the environment
    locale.setlocale(locale.LC_ALL, "")

    # Get rid of the delay between pressing ESC and jumping to the parent menu
    os.environ.setdefault("ESCDELAY", "0")

    # Enter curses mode. _menuconfig() returns a string to print on exit, after
    # curses has been de-initialized.
    print(curses.wrapper(_menuconfig))

# Global variables used below:
#
#   _cur_menu:
#     Menu node of the menu (or menuconfig symbol, or choice) currently being
#     shown
#
#   _visible:
#     List of visible symbols in _cur_menu
#
#   _sel_node_i:
#     Index in _visible of the currently selected node
#
#   _menu_scroll:
#     Index in _visible of the top row of the menu display
#
#   _parent_screen_rows:
#     List/stack of the row numbers that the selections in the parent menus
#     appeared on. This is used to prevent the scrolling from jumping around
#     when going in and out of menus.

def _menuconfig(stdscr):
    # Logic for the "main" display, with the list of symbols, etc.

    globals()["stdscr"] = stdscr

    _init()

    while True:
        _draw_main()
        curses.doupdate()


        c = _get_wch_compat(_menu_win)

        if c == curses.KEY_RESIZE:
            _resize_main()

        if c in (curses.KEY_DOWN, "j", "J"):
            _select_next_menu_entry()

        elif c in (curses.KEY_UP, "k", "K"):
            _select_prev_menu_entry()

        elif c in (curses.KEY_NPAGE, "\x04"):  # Page Down/Ctrl-D
            # Keep it simple. This way we get sane behavior for small windows,
            # etc., for free.
            for _ in range(_PG_JUMP):
                _select_next_menu_entry()

        elif c in (curses.KEY_PPAGE, "\x15"):  # Page Up/Ctrl-U
            for _ in range(_PG_JUMP):
                _select_prev_menu_entry()

        elif c in (curses.KEY_END, "G"):
            _select_last_menu_entry()

        elif c in (curses.KEY_HOME, "g"):
            _select_first_menu_entry()

        elif c in (curses.KEY_RIGHT, " ", "\n", "l", "L"):
            # Do appropriate node action. Only Space is treated specially,
            # preferring to toggle nodes rather than enter menus.

            sel_node = _visible[_sel_node_i]

            if sel_node.is_menuconfig and not \
               (c == " " and _prefer_toggle(sel_node.item)):

                _enter_menu(sel_node)

            else:
                _change_node(sel_node)
                if _is_y_mode_choice_sym(sel_node.item):
                    # Immediately jump to the parent menu after making a choice
                    # selection, like 'make menuconfig' does
                    _leave_menu()

        elif c in (curses.KEY_LEFT, curses.KEY_BACKSPACE, _ERASE_CHAR,
                   "\x1B",  # \x1B = ESC
                   "h", "H"):

            _leave_menu()

        elif c in ("s", "S"):
            _save_dialog(_kconf.write_config, _config_filename,
                         "configuration")

        elif c in ("m", "M"):
            _save_dialog(_kconf.write_min_config, "defconfig",
                         "minimal configuration")

        elif c == "?":
            _display_info(_visible[_sel_node_i])

        elif c in ("q", "Q"):
            while True:
                c = _key_dialog(
                    "Quit",
                    " Save configuration?\n"
                    "\n"
                    "(Y)es  (N)o  (C)ancel",
                    "ync")

                if c is None or c == "c":
                    break

                if c == "y":
                    if _try_save(_kconf.write_config, _config_filename,
                                 "configuration"):

                        return "Configuration saved to '{}'" \
                               .format(_config_filename)

                elif c == "n":
                    return "Configuration was not saved"

def _init():
    # Initializes the main display with the list of symbols, etc. Also does
    # misc. global initialization that needs to happen after initializing
    # curses.

    global _ERASE_CHAR

    global _path_win
    global _top_sep_win
    global _menu_win
    global _bot_sep_win
    global _help_win

    global _parent_screen_rows
    global _cur_menu
    global _visible
    global _sel_node_i
    global _menu_scroll

    # Looking for this in addition to KEY_BACKSPACE (which is unreliable) makes
    # backspace work with TERM=vt100. That makes it likely to work in sane
    # environments.
    #
    # erasechar() returns a 'bytes' object. Since we use get_wch(), we need to
    # decode it. Just give up and avoid crashing if it can't be decoded.
    _ERASE_CHAR = curses.erasechar().decode("utf-8", "ignore")

    _init_styles()

    # Hide the cursor
    _safe_curs_set(0)

    # Initialize windows

    # Top row, with menu path
    _path_win = _styled_win(_PATH_STYLE)

    # Separator below menu path, with title and arrows pointing up
    _top_sep_win = _styled_win(_TOP_SEP_STYLE)

    # List of menu entries with symbols, etc.
    _menu_win = _styled_win(_MENU_LIST_STYLE)
    _menu_win.keypad(True)

    # Row below menu list, with arrows pointing down
    _bot_sep_win = _styled_win(_BOT_SEP_STYLE)

    # Help window with keys at the bottom
    _help_win = _styled_win(_HELP_STYLE)

    # The rows we'd like the nodes in the parent menus to appear on. This
    # prevents the scroll from jumping around when going in and out of menus.
    _parent_screen_rows = []

    # Initial state
    _cur_menu = _kconf.top_node
    _visible = _visible_nodes(_cur_menu)
    _sel_node_i = 0
    _menu_scroll = 0

    # Give windows their initial size
    _resize_main()

def _resize_main():
    # Resizes the "main" display, with the list of menu entries, etc., to a
    # size appropriate for the terminal size

    global _menu_scroll

    screen_height, screen_width = stdscr.getmaxyx()

    _path_win.resize(1, screen_width)
    _top_sep_win.resize(1, screen_width)
    _bot_sep_win.resize(1, screen_width)

    help_win_height = len(_MAIN_HELP_LINES)
    menu_win_height = screen_height - help_win_height - 3

    if menu_win_height >= 1:
        _menu_win.resize(menu_win_height, screen_width)
        _help_win.resize(help_win_height, screen_width)

        _top_sep_win.mvwin(1, 0)
        _menu_win.mvwin(2, 0)
        _bot_sep_win.mvwin(2 + menu_win_height, 0)
        _help_win.mvwin(2 + menu_win_height + 1, 0)
    else:
        # Degenerate case. Give up on nice rendering and just prevent errors.

        menu_win_height = 1

        _menu_win.resize(1, screen_width)
        _help_win.resize(1, screen_width)

        for win in _top_sep_win, _menu_win, _bot_sep_win, _help_win:
            win.mvwin(0, 0)

    # Adjust the scroll so that the selected node is still within the
    # window, if needed
    if _sel_node_i - _menu_scroll >= menu_win_height:
        _menu_scroll = _sel_node_i - menu_win_height + 1

def _menu_win_height():
    # Returns the height of the menu display

    return _menu_win.getmaxyx()[0]

def _max_menu_scroll():
    # Returns the maximum amount the menu display can be scrolled down. We stop
    # scrolling when the bottom node is visible.

    return max(0, len(_visible) - _menu_win_height())

def _prefer_toggle(item):
    # For nodes with menus, determines whether Space should change the value of
    # the node's item or enter its menu. We toggle symbols (which have menus
    # when they're defined with 'menuconfig') and choices that can be in more
    # than one mode (e.g. optional choices). In other cases, we enter the menu.

    return isinstance(item, Symbol) or \
           (isinstance(item, Choice) and len(item.assignable) > 1)

def _enter_menu(menu):
    # Makes 'menu' the currently displayed menu

    global _cur_menu
    global _visible
    global _sel_node_i
    global _menu_scroll

    visible_sub = _visible_nodes(menu)
    # Never enter empty menus. We depend on having a current node.
    if visible_sub:
        # Remember where the current node appears on the screen, so we can try
        # to get it to appear in the same place when we leave the menu
        _parent_screen_rows.append(_sel_node_i - _menu_scroll)

        # Jump into menu
        _cur_menu = menu
        _visible = visible_sub
        _sel_node_i = 0
        _menu_scroll = 0

def _leave_menu():
    # Jumps to the parent menu of the current menu. Does nothing if we're in
    # the top menu.

    global _cur_menu
    global _visible
    global _sel_node_i
    global _menu_scroll

    if _cur_menu is _kconf.top_node:
        return

    # Jump to parent menu
    parent = _parent_menu(_cur_menu)
    _visible = _visible_nodes(parent)
    _sel_node_i = _visible.index(_cur_menu)
    _cur_menu = parent

    # Try to make the menu entry appear on the same row on the screen as it did
    # before we entered the menu
    _menu_scroll = max(_sel_node_i - _parent_screen_rows.pop(), 0)

def _select_next_menu_entry():
    # Selects the menu entry after the current one, adjusting the scroll if
    # necessary. Does nothing if we're already at the last menu entry.

    global _sel_node_i
    global _menu_scroll

    if _sel_node_i < len(_visible) - 1:
        # Jump to the next node
        _sel_node_i += 1

        # If the new node is sufficiently close to the edge of the menu window
        # (as determined by _SCROLL_OFFSET), increase the scroll by one. This
        # gives nice and non-jumpy behavior even when
        # _SCROLL_OFFSET >= _menu_win_height().
        if _sel_node_i >= _menu_scroll + _menu_win_height() - _SCROLL_OFFSET:
            _menu_scroll = min(_menu_scroll + 1, _max_menu_scroll())

def _select_prev_menu_entry():
    # Selects the menu entry before the current one, adjusting the scroll if
    # necessary. Does nothing if we're already at the first menu entry.

    global _sel_node_i
    global _menu_scroll

    if _sel_node_i > 0:
        # Jump to the previous node
        _sel_node_i -= 1

        # See _select_next_menu_entry()
        if _sel_node_i <= _menu_scroll + _SCROLL_OFFSET:
            _menu_scroll = max(_menu_scroll - 1, 0)

def _select_last_menu_entry():
    # Selects the last menu entry in the current menu

    global _sel_node_i
    global _menu_scroll

    _sel_node_i = len(_visible) - 1
    _menu_scroll = _max_menu_scroll()

def _select_first_menu_entry():
    # Selects the first menu entry in the current menu

    global _sel_node_i
    global _menu_scroll

    _sel_node_i = _menu_scroll = 0

def _draw_main():
    # Draws the "main" display, with the list of symbols, the header, and the
    # footer.
    #
    # This could be optimized to only update the windows that have actually
    # changed, but keep it simple for now and let curses sort it out.


    #
    # Update the top row with the menu path
    #

    _path_win.erase()

    # Draw the menu path ("(top menu) -> menu -> submenu -> ...")

    menu_prompts = []

    menu = _cur_menu
    while menu is not _kconf.top_node:
        menu_prompts.insert(0, menu.prompt[0])
        menu = menu.parent

    _safe_addstr(_path_win, 0, 0, "(top menu)")
    for prompt in menu_prompts:
        _safe_addch(_path_win, " ")
        _safe_addch(_path_win, curses.ACS_RARROW)
        _safe_addstr(_path_win, " " + prompt)

    _path_win.noutrefresh()


    #
    # Update the separator row below the menu path
    #

    _top_sep_win.erase()

    # Draw arrows pointing up if the symbol window is scrolled down. Draw them
    # before drawing the title, so the title ends up on top for small windows.
    if _menu_scroll > 0:
        _safe_hline(_top_sep_win, 0, 4, curses.ACS_UARROW, _N_SCROLL_ARROWS)

    # Add the 'mainmenu' text as the title, centered at the top
    _safe_addstr(_top_sep_win,
                 0, (stdscr.getmaxyx()[1] - len(_kconf.mainmenu_text))//2,
                 _kconf.mainmenu_text)

    _top_sep_win.noutrefresh()


    #
    # Update the symbol window
    #

    _menu_win.erase()

    # Draw the _visible nodes starting from index _menu_scroll up to either as
    # many as fit in the window, or to the end of _visible
    for i in range(_menu_scroll,
                   min(_menu_scroll + _menu_win_height(), len(_visible))):

        _safe_addstr(_menu_win, i - _menu_scroll, 0,
                     _node_str(_visible[i]),
                     # Highlight the selected entry
                     _MENU_LIST_SEL_STYLE
                         if i == _sel_node_i else curses.A_NORMAL)

    _menu_win.noutrefresh()


    #
    # Update the bottom separator window
    #

    _bot_sep_win.erase()

    # Draw arrows pointing down if the symbol window is scrolled up
    if _menu_scroll < _max_menu_scroll():
        _safe_hline(_bot_sep_win, 0, 4, curses.ACS_DARROW, _N_SCROLL_ARROWS)

    _bot_sep_win.noutrefresh()


    #
    # Update the help window
    #

    _help_win.erase()

    for i, line in enumerate(_MAIN_HELP_LINES):
        _safe_addstr(_help_win, i, 0, line)

    _help_win.noutrefresh()

def _parent_menu(node):
    # Returns the menu node of the menu that contains 'node'. In addition to
    # proper 'menu's, this might also be a 'menuconfig' symbol or a 'choice'.
    # "Menu" here means a menu in the interface (a list of menu entries).

    menu = node.parent
    while not menu.is_menuconfig:
        menu = menu.parent
    return menu

def _visible_nodes(menu):
    # Returns a list of the nodes in 'menu' (see _parent_menu()) that should be
    # visible in the menu window

    def rec(node):
        res = []

        while node:
            # Show the node if its prompt is visible. For menus, also check
            # 'visible if'.
            if node.prompt and expr_value(node.prompt[1]) and not \
               (node.item == MENU and not expr_value(node.visibility)):
                res.append(node)

                # If a node has children but doesn't have the is_menuconfig
                # flag set, the children come from a submenu created implicitly
                # from dependencies. Show those in this menu too.
                if node.list and not node.is_menuconfig:
                    res.extend(rec(node.list))

            node = node.next

        return res

    return rec(menu.list)

def _change_node(node):
    # Changes the value of the menu node 'node' if it is a symbol. Bools and
    # tristates are toggled, while other symbol types pop up a text entry
    # dialog.

    global _cur_menu
    global _visible
    global _sel_node_i
    global _menu_scroll

    if not isinstance(node.item, (Symbol, Choice)):
        return

    # sc = symbol/choice
    sc = node.item

    if sc.type in (INT, HEX, STRING):
        s = sc.str_value

        while True:
            s = _input_dialog("Value for '{}' ({})".format(
                                  node.prompt[0], TYPE_TO_STR[sc.type]),
                              s, _range_info(sc))

            if s is None:
                break

            if sc.type in (INT, HEX):
                s = s.strip()

                # 'make menuconfig' does this too. Hex values not starting with
                # '0x' are accepted when loading .config files though.
                if sc.type == HEX and not s.startswith(("0x", "0X")):
                    s = "0x" + s

            if _check_validity(sc, s):
                sc.set_value(s)
                break

    elif len(sc.assignable) == 1:
        # Handles choice symbols for choices in y mode, which are a special
        # case: .assignable can be (2,) while .tri_value is 0.
        sc.set_value(sc.assignable[0])

    else:
        # Set the symbol to the value after the current value in
        # sc.assignable, with wrapping
        val_index = sc.assignable.index(sc.tri_value)
        sc.set_value(
            sc.assignable[(val_index + 1) % len(sc.assignable)])

    # Changing the value of the symbol might have changed what items in the
    # current menu are visible. Recalculate the state.

    # Row on the screen the cursor was on
    old_row = _sel_node_i - _menu_scroll

    sel_node = _visible[_sel_node_i]

    # New visible nodes
    _visible = _visible_nodes(_cur_menu)

    # New index of selected node
    _sel_node_i = _visible.index(sel_node)

    # Try to make the cursor stay on the same row in the menu window. This
    # might be impossible if too many nodes have disappeared above the node.
    _menu_scroll = max(_sel_node_i - old_row, 0)

def _input_dialog(title, initial_text, info_text=None):
    # Pops up a dialog that prompts the user for a string
    #
    # title:
    #   Title to display at the top of the dialog window's border
    #
    # initial_text:
    #   Initial text to prefill the input field with
    #
    # info_text:
    #   String to show next to the input field. If None, just the input field
    #   is shown.

    win = _styled_win(_DIALOG_BODY_STYLE)
    win.keypad(True)

    # Give the input dialog its initial size
    _resize_input_dialog(win, title, info_text)

    _safe_curs_set(2)

    # Input field text
    s = initial_text

    # Cursor position
    i = len(initial_text)

    # Horizontal scroll offset
    hscroll = 0

    while True:
        # Width of input field
        edit_width = win.getmaxyx()[1] - 4

        # Adjust horizontal scroll if the cursor would be outside the input
        # field
        if i < hscroll:
            hscroll = i
        elif i >= hscroll + edit_width:
            hscroll = i - edit_width + 1

        # Draw the "main" display with the menu, etc., so that resizing still
        # works properly. This is like a stack of windows, only hardcoded for
        # now.
        _draw_main()

        _draw_input_dialog(win, title, info_text, s, i, hscroll)
        curses.doupdate()


        c = _get_wch_compat(win)

        if c == "\n":
            _safe_curs_set(0)
            return s

        if c == "\x1B":  # \x1B = ESC
            _safe_curs_set(0)
            return None


        if c == curses.KEY_RESIZE:
            # Resize the main display too. The dialog floats above it.
            _resize_main()

            _resize_input_dialog(win, title, info_text)

        elif c == curses.KEY_LEFT:
            if i > 0:
                i -= 1

        elif c == curses.KEY_RIGHT:
            if i < len(s):
                i += 1

        elif c in (curses.KEY_HOME, "\x01"):  # \x01 = CTRL-A
            i = 0

        elif c in (curses.KEY_END, "\x05"):  # \x05 = CTRL-E
            i = len(s)

        elif c in (curses.KEY_BACKSPACE, _ERASE_CHAR):
            if i > 0:
                s = s[:i-1] + s[i:]
                i -= 1

        elif c == curses.KEY_DC:
            s = s[:i] + s[i+1:]

        elif c == "\x0B":  # \x0B = CTRL-K
            s = s[:i]

        elif c == "\x15":  # \x15 = CTRL-U
            s = s[i:]
            i = 0

        elif isinstance(c, str):
            # Insert character

            s = s[:i] + c + s[i:]
            i += 1

def _resize_input_dialog(win, title, info_text):
    # Resizes the input dialog to a size appropriate for the terminal size

    screen_height, screen_width = stdscr.getmaxyx()

    win_height = min(5 if info_text is None else 7, screen_height)

    win_width = max(_INPUT_DIALOG_MIN_WIDTH, len(title) + 4)
    if info_text is not None:
        win_width = max(win_width, len(info_text) + 4)
    win_width = min(win_width, screen_width)

    win.resize(win_height, win_width)
    win.mvwin((screen_height - win_height)//2,
              (screen_width - win_width)//2)

def _draw_input_dialog(win, title, info_text, s, i, hscroll):
    edit_width = win.getmaxyx()[1] - 4

    win.erase()

    _draw_frame(win, title)

    # Note: Perhaps having a separate window for the input field would be nicer
    visible_s = s[hscroll:hscroll + edit_width]
    _safe_addstr(win, 2, 2, visible_s + " "*(edit_width - len(visible_s)),
                 _INPUT_FIELD_STYLE)

    if info_text is not None:
        _safe_addstr(win, 4, 2, info_text)

    _safe_move(win, 2, 2 + i - hscroll)

    win.noutrefresh()

def _save_dialog(save_fn, default_filename, description):
    # Pops up a dialog that prompts the user for where to save a file
    #
    # save_fn:
    #   Function to call with 'filename' to save the file
    #
    # default_filename:
    #   Prefilled filename in the input field
    #
    # description:
    #   String describing the thing being saved

    filename = default_filename
    while True:
        filename = _input_dialog(
            "Filename to save {} to".format(description),
            filename)

        if filename is None or \
           _try_save(save_fn, filename, description):

            return

def _try_save(save_fn, filename, description):
    # Tries to save a file. Pops up an error and returns False on failure.
    #
    # save_fn:
    #   Function to call with 'filename' to save the file
    #
    # description:
    #   String describing the thing being saved

    try:
        save_fn(filename)
        return True
    except OSError as e:
        _error("Error saving {} to '{}'\n\n{} (errno: {})"
               .format(description, e.filename, e.strerror,
                       errno.errorcode[e.errno]))
        return False

def _key_dialog(title, text, keys):
    # Pops up a dialog that can be closed by pressing a key
    #
    # title:
    #   Title to display at the top of the dialog window's border
    #
    # text:
    #   Text to show in the dialog
    #
    # keys:
    #   List of keys that will close the dialog. Other keys (besides ESC) are
    #   ignored. The caller is responsible for providing a hint about which
    #   keys can be pressed in 'text'.
    #
    # Return value:
    #   The key that was pressed to close the dialog. Uppercase characters are
    #   converted to lowercase. ESC will always close the dialog, and returns
    #   None.

    win = _styled_win(_DIALOG_BODY_STYLE)
    win.keypad(True)

    _resize_key_dialog(win, text)

    while True:
        # See _input_dialog()
        _draw_main()

        _draw_key_dialog(win, title, text)
        curses.doupdate()


        c = _get_wch_compat(win)

        if c == "\x1B":  # \x1B = ESC
            return None


        if c == curses.KEY_RESIZE:
            # Resize the main display too. The dialog floats above it.
            _resize_main()

            _resize_key_dialog(win, text)

        elif isinstance(c, str):
            c = c.lower()
            if c in keys:
                return c

def _resize_key_dialog(win, text):
    # Resizes the key dialog to a size appropriate for the terminal size

    screen_height, screen_width = stdscr.getmaxyx()

    lines = text.split("\n")

    win_height = min(len(lines) + 4, screen_height)
    win_width = min(max(len(line) for line in lines) + 4, screen_width)

    win.resize(win_height, win_width)
    win.mvwin((screen_height - win_height)//2,
              (screen_width - win_width)//2)

def _draw_key_dialog(win, title, text):
    win.erase()
    _draw_frame(win, title)

    for i, line in enumerate(text.split("\n")):
        _safe_addstr(win, 2 + i, 2, line)

    win.noutrefresh()

def _error(text):
    # Pops up an error dialog that can be dismissed with Space/Enter/ESC

    _key_dialog("Error", text, " \n")

def _draw_frame(win, title):
    # Draw a frame around the inner edges of 'win', with 'title' at the top

    win_height, win_width = win.getmaxyx()

    win.attron(_DIALOG_FRAME_STYLE)

    # Draw top/bottom edge
    _safe_hline(win,              0, 0, " ", win_width)
    _safe_hline(win, win_height - 1, 0, " ", win_width)

    # Draw left/right edge
    _safe_vline(win, 0,             0, " ", win_height)
    _safe_vline(win, 0, win_width - 1, " ", win_height)

    # Draw title
    _safe_addstr(win, 0, (win_width - len(title))//2, title)

    win.attroff(_DIALOG_FRAME_STYLE)

def _display_info(node):
    # Shows a fullscreen window with information about 'node'

    # Top row, with title and arrows point up
    top_line_win = _styled_win(_INFO_TOP_LINE_STYLE)

    # Text display
    text_win = _styled_win(_INFO_TEXT_STYLE)
    text_win.keypad(True)

    # Bottom separator, with arrows pointing down
    bot_sep_win = _styled_win(_INFO_BOT_SEP_STYLE)

    # Help window with keys at the bottom
    help_win = _styled_win(_INFO_HELP_STYLE)

    # Give windows their initial size
    _resize_info_display(top_line_win, text_win, bot_sep_win, help_win)


    # Get lines of help text
    lines = _info(node).split("\n")

    # Index of first row in 'lines' to show
    scroll = 0

    while True:
        _draw_info_display(node, lines, scroll, top_line_win, text_win,
                           bot_sep_win, help_win)
        curses.doupdate()


        c = _get_wch_compat(text_win)

        if c == curses.KEY_RESIZE:
            # No need to call _resize_main(), because the help window is
            # fullscreen
            _resize_info_display(top_line_win, text_win, bot_sep_win, help_win)

        elif c in (curses.KEY_DOWN, "j", "J"):
            if scroll < _max_info_scroll(text_win, lines):
                scroll += 1

        elif c in (curses.KEY_NPAGE, "\x04"):  # Page Down/Ctrl-D
            scroll = min(scroll + _PG_JUMP, _max_info_scroll(text_win, lines))

        elif c in (curses.KEY_PPAGE, "\x15"):  # Page Up/Ctrl-U
            scroll = max(scroll - _PG_JUMP, 0)

        elif c in (curses.KEY_END, "G"):
            scroll = _max_info_scroll(text_win, lines)

        elif c in (curses.KEY_HOME, "g"):
            scroll = 0

        elif c in (curses.KEY_UP, "k", "K"):
            if scroll > 0:
                scroll -= 1

        elif c in (curses.KEY_LEFT, curses.KEY_BACKSPACE, _ERASE_CHAR,
                   "\x1B",  # \x1B = ESC
                   "q", "Q", "h", "H"):

            # Resize the main display before returning so that it gets the
            # right size in case the terminal was resized while the help
            # display was open
            _resize_main()

            return

def _resize_info_display(top_line_win, text_win, bot_sep_win, help_win):
    # Resizes the help display to a size appropriate for the terminal size

    screen_height, screen_width = stdscr.getmaxyx()

    top_line_win.resize(1, screen_width)
    bot_sep_win.resize(1, screen_width)

    help_win_height = len(_INFO_HELP_LINES)
    text_win_height = screen_height - help_win_height - 2

    if text_win_height >= 1:
        text_win.resize(text_win_height, screen_width)
        help_win.resize(help_win_height, screen_width)

        text_win.mvwin(1, 0)
        bot_sep_win.mvwin(1 + text_win_height, 0)
        help_win.mvwin(1 + text_win_height + 1, 0)
    else:
        # Degenerate case. Give up on nice rendering and just prevent errors.

        text_win.resize(1, screen_width)
        help_win.resize(1, screen_width)

        for win in text_win, bot_sep_win, help_win:
            win.mvwin(0, 0)

def _draw_info_display(node, lines, scroll, top_line_win, text_win,
                       bot_sep_win, help_win):

    text_win_height, text_win_width = text_win.getmaxyx()


    #
    # Update top row
    #

    top_line_win.erase()

    # Draw arrows pointing up if the information window is scrolled down. Draw
    # them before drawing the title, so the title ends up on top for small
    # windows.
    if scroll > 0:
        _safe_hline(top_line_win, 0, 4, curses.ACS_UARROW, _N_SCROLL_ARROWS)


    if isinstance(node.item, Symbol):
        title = "{}{}".format(_kconf.config_prefix, node.item.name)

    elif isinstance(node.item, Choice):
        title = node.item.name or "Choice"

    elif node.item == MENU:
        title = 'menu "{}"'.format(node.prompt[0])

    else:  # node.item == COMMENT
        title = 'comment "{}"'.format(node.prompt[0])


    _safe_addstr(top_line_win, 0, (text_win_width - len(title))//2, title)

    top_line_win.noutrefresh()


    #
    # Update text display
    #

    text_win.erase()

    for i, line in enumerate(lines[scroll:scroll + text_win_height]):
        _safe_addstr(text_win, i, 0, line)

    text_win.noutrefresh()


    #
    # Update bottom separator line
    #

    bot_sep_win.erase()

    # Draw arrows pointing down if the symbol window is scrolled up
    if scroll < _max_info_scroll(text_win, lines):
        _safe_hline(bot_sep_win, 0, 4, curses.ACS_DARROW, _N_SCROLL_ARROWS)

    bot_sep_win.noutrefresh()


    #
    # Update help window at bottom
    #

    help_win.erase()

    for i, line in enumerate(_INFO_HELP_LINES):
        _safe_addstr(help_win, i, 0, line)

    help_win.noutrefresh()

def _max_info_scroll(text_win, lines):
    # Returns the maximum amount the information display can be scrolled down.
    # We stop scrolling when the last line of the help text is visible.

    return max(0, len(lines) - text_win.getmaxyx()[0])

def _info(node):
    # Returns information about the menu node 'node' as a string.
    #
    # The helper functions are responsible for adding newlines. This allows
    # them to return "" if they don't want to add any output.

    if isinstance(node.item, Symbol):
        sym = node.item

        return (
            _prompt_info(sym) +
            "Type: {}\n".format(TYPE_TO_STR[sym.type]) +
            'Value: "{}"\n\n'.format(sym.str_value) +
            _help_info(sym) +
            _direct_dep_info(sym) +
            _defaults_info(sym) +
            _select_imply_info(sym) +
            _loc_info(sym) +
            _kconfig_def_info(sym)
        )

    if isinstance(node.item, Choice):
        choice = node.item

        return (
            _prompt_info(choice) +
            "Type: {}\n".format(TYPE_TO_STR[choice.type]) +
            'Mode: "{}"\n\n'.format(choice.str_value) +
            _help_info(choice) +
            _choice_syms_info(choice) +
            _direct_dep_info(choice) +
            _defaults_info(choice) +
            _loc_info(choice) +
            _kconfig_def_info(choice)
        )

    # node.item in (MENU, COMMENT)
    return "Defined at {}:{}\nMenu: {}\n\n{}" \
           .format(node.filename, node.linenr, _menu_path_info(node),
                   _kconfig_def_info(node))

def _prompt_info(sc):
    # Returns a string listing the prompts of 'sc' (Symbol or Choice)

    s = ""

    for node in sc.nodes:
        if node.prompt:
            s += "Prompt: {}\n".format(node.prompt[0])

    return s

def _choice_syms_info(choice):
    # Returns a string listing the choice symbols in 'choice'. Adds
    # "(selected)" next to the selected one.

    s = "Choice symbols:\n"

    for sym in choice.syms:
        s += "  - " + sym.name
        if sym is choice.selection:
            s += " (selected)"
        s += "\n"

    return s + "\n"

def _help_info(sc):
    # Returns a string with the help text(s) of 'sc' (Symbol or Choice).
    # Symbols and choices defined in multiple locations can have multiple help
    # texts.

    s = ""

    for node in sc.nodes:
        if node.help is not None:
            s += "Help:\n\n{}\n\n" \
                 .format(textwrap.indent(node.help, "  "))

    return s

def _direct_dep_info(sc):
    # Returns a string describing the direct dependencies of 'sc' (Symbol or
    # Choice). The direct dependencies are the OR of the dependencies from each
    # definition location. The dependencies at each definition location come
    # from 'depends on' and dependencies inherited from parent items.

    if sc.direct_dep is _kconf.y:
        return ""

    return 'Direct dependencies (value: "{}"):\n{}\n' \
           .format(TRI_TO_STR[expr_value(sc.direct_dep)],
                   _split_expr_info(sc.direct_dep, 2))

def _defaults_info(sc):
    # Returns a string describing the defaults of 'sc' (Symbol or Choice)

    if not sc.defaults:
        return ""

    s = "Defaults:\n"

    for value, cond in sc.defaults:
        s += "  - "
        if isinstance(sc, Symbol):
            s += _expr_str(value)
        else:
            # Don't print the value next to the symbol name for choice
            # defaults, as it looks a bit confusing
            s += value.name
        s += "\n"

        if cond is not _kconf.y:
            s += '    Condition (value: "{}"):\n{}' \
                 .format(TRI_TO_STR[expr_value(cond)],
                         _split_expr_info(cond, 7))

    return s + "\n"

def _split_expr_info(expr, indent):
    # Returns a string with 'expr' split into its top-level && or || operands,
    # with one operand per line, together with the operand's value. This is
    # usually enough to get something readable for long expressions. A fancier
    # recursive thingy would be possible too.
    #
    # indent:
    #   Number of leading spaces to add before the split expression.

    if len(split_expr(expr, AND)) > 1:
        split_op = AND
        op_str = "&&"
    else:
        split_op = OR
        op_str = "||"

    s = ""
    for i, term in enumerate(split_expr(expr, split_op)):
        s += '{}{} {} (value: "{}")\n' \
             .format(" "*indent,
                     "  " if i == 0 else op_str,
                     _expr_str(term),
                     TRI_TO_STR[expr_value(term)])
    return s

def _select_imply_info(sym):
    # Returns a string with information about which symbols 'select' or 'imply'
    # 'sym'. The selecting/implying symbols are grouped according to which
    # value they select/imply 'sym' to (n/m/y).

    s = ""

    def add_sis(expr, val, title):
        nonlocal s

        # sis = selects/implies
        sis = [si for si in split_expr(expr, OR) if expr_value(si) == val]

        if sis:
            s += title
            for si in sis:
                s += "  - {}\n".format(split_expr(si, AND)[0].name)
            s += "\n"

    if sym.rev_dep is not _kconf.n:
        add_sis(sym.rev_dep, 2, "Symbols currently y-selecting this symbol:\n")
        add_sis(sym.rev_dep, 1, "Symbols currently m-selecting this symbol:\n")
        add_sis(sym.rev_dep, 0, "Symbols currently n-selecting this symbol (no effect):\n")

    if sym.weak_rev_dep is not _kconf.n:
        add_sis(sym.weak_rev_dep, 2, "Symbols currently y-implying this symbol:\n")
        add_sis(sym.weak_rev_dep, 1, "Symbols currently m-implying this symbol:\n")
        add_sis(sym.weak_rev_dep, 0, "Symbols currently n-implying this symbol (no effect):\n")

    return s

def _loc_info(sc):
    # Returns a string with information about where 'sc' (Symbol or Choice) is
    # defined in the Kconfig files. Also includes the menu path leading up to
    # it.

    s = "Definition location{}:\n".format("s" if len(sc.nodes) > 1 else "")

    for node in sc.nodes:
        s += "  - {}:{}\n      Menu: {}\n" \
             .format(node.filename, node.linenr, _menu_path_info(node))

    return s + "\n"

def _kconfig_def_info(item):
    # Returns a string with the definition of 'item' in Kconfig syntax

    return "Kconfig definition (with propagated dependencies):\n\n" + \
           textwrap.indent(str(item).expandtabs(), "  ")

def _menu_path_info(node):
    # Returns a string describing the menu path leading up to 'node'

    path = ""

    menu = node.parent
    while menu is not _kconf.top_node:
        path = " -> " + menu.prompt[0] + path
        menu = menu.parent

    return "(top menu)" + path

def _styled_win(style):
    # Returns a new curses window with background 'style' and space as the fill
    # character. The initial dimensions are (1, 1), so the window needs to be
    # sized and positioned separately.

    win = curses.newwin(1, 1)
    win.bkgdset(" ", style)
    return win

def _node_str(node):
    # Returns the complete menu entry text for a menu node.
    #
    # Example return value: "[*] Support for X"

    # Calculate the indent to print the item with by checking how many levels
    # above it the closest 'menuconfig' item is (this includes menus and
    # choices as well as menuconfig symbols)
    indent = 0
    parent = node.parent
    while not parent.is_menuconfig:
        indent += 2
        parent = parent.parent

    # This approach gives nice alignment for empty string symbols ("()  Foo")
    s = "{:{}} ".format(_value_str(node), 3 + indent)

    if node.prompt:
        if node.item == COMMENT:
            s += "*** {} ***".format(node.prompt[0])
        else:
            s += node.prompt[0]

        if isinstance(node.item, Symbol):
            sym = node.item

            # Print "(NEW)" next to symbols without a user value (from e.g. a
            # .config), but skip it for choice symbols in choices in y mode
            if sym.user_value is None and \
               not (sym.choice and sym.choice.tri_value == 2):

                s += " (NEW)"

        if isinstance(node.item, Choice) and node.item.tri_value == 2:
            # Print the prompt of the selected symbol after the choice for
            # choices in y mode
            sym = node.item.selection
            if sym:
                for node_ in sym.nodes:
                    if node_.prompt:
                        s += " ({})".format(node_.prompt[0])

    # Print "--->" next to nodes that have menus that can potentially be
    # entered. Add "(empty)" if the menu is empty. We don't allow those to be
    # entered.
    if node.is_menuconfig:
        s += "  --->" if _visible_nodes(node) else "  ---> (empty)"

    return s

def _value_str(node):
    # Returns the value part ("[*]", "<M>", "(foo)" etc.) of a menu node

    item = node.item

    if item in (MENU, COMMENT):
        return ""

    # Wouldn't normally happen, and generates a warning
    if item.type == UNKNOWN:
        return ""

    if item.type in (STRING, INT, HEX):
        return "({})".format(item.str_value)

    # BOOL or TRISTATE

    if _is_y_mode_choice_sym(item):
        return "(X)" if item.choice.selection is item else "( )"

    tri_val_str = (" ", "M", "*")[item.tri_value]

    if len(item.assignable) == 1:
        # Pinned to a single value
        return "" if isinstance(item, Choice) else "-{}-".format(tri_val_str)

    if item.type == BOOL:
        return "[{}]".format(tri_val_str)

    if item.type == TRISTATE:
        if item.assignable == (1, 2):
            return "{{{}}}".format(tri_val_str)  # { }/{M}/{*}
        return "<{}>".format(tri_val_str)

def _is_y_mode_choice_sym(item):
    # The choice mode is an upper bound on the visibility of choice symbols, so
    # we can check the choice symbols' own visibility to see if the choice is
    # in y mode
    return isinstance(item, Symbol) and item.choice and item.visibility == 2

def _check_validity(sym, s):
    # Returns True if the string 's' is a well-formed value for 'sym'.
    # Otherwise, displays an error and returns False.

    if sym.type not in (INT, HEX):
        # Anything goes for non-int/hex symbols
        return True

    base = 10 if sym.type == INT else 16

    try:
        int(s, base)
    except ValueError:
        _error("'{}' is a malformed {} value"
               .format(s, TYPE_TO_STR[sym.type]))
        return False

    for low_sym, high_sym, cond in sym.ranges:
        if expr_value(cond):
            low = int(low_sym.str_value, base)
            val = int(s, base)
            high = int(high_sym.str_value, base)

            if not low <= val <= high:
                _error("{} is outside the range {}-{}"
                       .format(s, low_sym.str_value, high_sym.str_value))

                return False

            break

    return True

def _range_info(sym):
    # Returns a string with information about the valid range for the symbol
    # 'sym', or None if 'sym' isn't an int/hex symbol

    if sym.type not in (INT, HEX):
        return None

    for low, high, cond in sym.ranges:
        if expr_value(cond):
            return "Range: {}-{}".format(low.str_value, high.str_value)

    return "No range constraints."

def _is_num(name):
    # Heuristic to see if a symbol name looks like a number, for nicer output
    # when printing expressions. Things like 16 are actually symbol names, only
    # they get their name as their value when the symbol is undefined.

    try:
        int(name, 10)
        return True

    except ValueError:
        if not name.startswith(("0x", "0X")):
            return False

        try:
            int(name, 16)
            return True

        except ValueError:
            return False

def _get_wch_compat(win):
    # Decent resizing behavior on PDCurses requires calling resize_term(0, 0)
    # after receiving KEY_RESIZE, while NCURSES (usually) handles terminal
    # resizing automatically in get(_w)ch() (see the end of the
    # resizeterm(3NCURSES) man page).
    #
    # resize_term(0, 0) reliably fails and does nothing on NCURSES, so this
    # hack gives NCURSES/PDCurses compatibility for resizing. I don't know
    # whether it would cause trouble for other implementations.

    c = win.get_wch()
    if c == curses.KEY_RESIZE:
        try:
            curses.resize_term(0, 0)
        except curses.error:
            pass

    return c

# Ignore exceptions from some functions that might fail, e.g. for small
# windows. They usually do reasonable things anyway.

def _safe_curs_set(visibility):
    try:
        curses.curs_set(visibility)
    except curses.error:
        pass

def _safe_addstr(win, *args):
    try:
        win.addstr(*args)
    except curses.error:
        pass

def _safe_addch(win, *args):
    try:
        win.addch(*args)
    except curses.error:
        pass

def _safe_hline(win, *args):
    try:
        win.hline(*args)
    except curses.error:
        pass

def _safe_vline(win, *args):
    try:
        win.vline(*args)
    except curses.error:
        pass

def _safe_move(win, *args):
    try:
        win.move(*args)
    except curses.error:
        pass

if __name__ == "__main__":
    if len(sys.argv) > 2:
        print("usage: {} [Kconfig]".format(sys.argv[0]), file=sys.stderr)
        sys.exit(1)

    menuconfig(Kconfig("Kconfig" if len(sys.argv) < 2 else sys.argv[1]))

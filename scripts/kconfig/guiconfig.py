#!/usr/bin/env python3

# Copyright (c) 2019, Nordic Semiconductor ASA and Ulf Magnusson
# SPDX-License-Identifier: ISC

# _load_images() builds names dynamically to avoid having to give them twice
# (once for the variable and once for the filename). This forces consistency
# too.
#
# pylint: disable=undefined-variable

"""
Overview
========

A Tkinter-based menuconfig implementation, based around a treeview control and
a help display. The interface should feel familiar to people used to qconf
('make xconfig'). Compatible with both Python 2 and Python 3.

The display can be toggled between showing the full tree and showing just a
single menu (like menuconfig.py). Only single-menu mode distinguishes between
symbols defined with 'config' and symbols defined with 'menuconfig'.

A show-all mode is available that shows invisible items in red.

Supports both mouse and keyboard controls. The following keyboard shortcuts are
available:

  Ctrl-S   : Save configuration
  Ctrl-O   : Open configuration
  Ctrl-A   : Toggle show-all mode
  Ctrl-N   : Toggle show-name mode
  Ctrl-M   : Toggle single-menu mode
  Ctrl-F, /: Open jump-to dialog
  ESC      : Close

Running
=======

guiconfig.py can be run either as a standalone executable or by calling the
menuconfig() function with an existing Kconfig instance. The second option is a
bit inflexible in that it will still load and save .config, etc.

When run in standalone mode, the top-level Kconfig file to load can be passed
as a command-line argument. With no argument, it defaults to "Kconfig".

The KCONFIG_CONFIG environment variable specifies the .config file to load (if
it exists) and save. If KCONFIG_CONFIG is unset, ".config" is used.

When overwriting a configuration file, the old version is saved to
<filename>.old (e.g. .config.old).

$srctree is supported through Kconfiglib.
"""

# Note: There's some code duplication with menuconfig.py below, especially for
# the help text. Maybe some of it could be moved into kconfiglib.py or a shared
# helper script, but OTOH it's pretty nice to have things standalone and
# customizable.

import errno
import os
import re
import sys

_PY2 = sys.version_info[0] < 3

if _PY2:
    # Python 2
    from Tkinter import *
    import ttk
    import tkFont as font
    import tkFileDialog as filedialog
    import tkMessageBox as messagebox
else:
    # Python 3
    from tkinter import *
    import tkinter.ttk as ttk
    import tkinter.font as font
    from tkinter import filedialog, messagebox

from kconfiglib import Symbol, Choice, MENU, COMMENT, MenuNode, \
                       BOOL, TRISTATE, STRING, INT, HEX, \
                       AND, OR, \
                       expr_str, expr_value, split_expr, \
                       standard_sc_expr_str, \
                       TRI_TO_STR, TYPE_TO_STR, \
                       standard_kconfig, standard_config_filename


# If True, use GIF image data embedded in this file instead of separate GIF
# files. See _load_images().
_USE_EMBEDDED_IMAGES = True


# Help text for the jump-to dialog
_JUMP_TO_HELP = """\
Type one or more strings/regexes and press Enter to list items that match all
of them. Python's regex flavor is used (see the 're' module). Double-clicking
an item will jump to it. Item values can be toggled directly within the dialog.\
"""


def _main():
    menuconfig(standard_kconfig(__doc__))


# Global variables used below:
#
#   _root:
#     The Toplevel instance for the main window
#
#   _tree:
#     The Treeview in the main window
#
#   _jump_to_tree:
#     The Treeview in the jump-to dialog. None if the jump-to dialog isn't
#     open. Doubles as a flag.
#
#   _jump_to_matches:
#     List of Nodes shown in the jump-to dialog
#
#   _menupath:
#     The Label that shows the menu path of the selected item
#
#   _backbutton:
#     The button shown in single-menu mode for jumping to the parent menu
#
#   _status_label:
#     Label with status text shown at the bottom of the main window
#     ("Modified", "Saved to ...", etc.)
#
#   _id_to_node:
#     We can't use Node objects directly as Treeview item IDs, so we use their
#     id()s instead. This dictionary maps Node id()s back to Nodes. (The keys
#     are actually str(id(node)), just to simplify lookups.)
#
#   _cur_menu:
#     The current menu. Ignored outside single-menu mode.
#
#   _show_all_var/_show_name_var/_single_menu_var:
#     Tkinter Variable instances bound to the corresponding checkboxes
#
#   _show_all/_single_menu:
#     Plain Python bools that track _show_all_var and _single_menu_var, to
#     speed up and simplify things a bit
#
#   _conf_filename:
#     File to save the configuration to
#
#   _minconf_filename:
#     File to save minimal configurations to
#
#   _conf_changed:
#     True if the configuration has been changed. If False, we don't bother
#     showing the save-and-quit dialog.
#
#     We reset this to False whenever the configuration is saved.
#
#   _*_img:
#     PhotoImage instances for images


def menuconfig(kconf):
    """
    Launches the configuration interface, returning after the user exits.

    kconf:
      Kconfig instance to be configured
    """
    global _kconf
    global _conf_filename
    global _minconf_filename
    global _jump_to_tree
    global _cur_menu

    _kconf = kconf

    _jump_to_tree = None

    _create_id_to_node()

    _create_ui()

    # Filename to save configuration to
    _conf_filename = standard_config_filename()

    # Load existing configuration and check if it's outdated
    _set_conf_changed(_load_config())

    # Filename to save minimal configuration to
    _minconf_filename = "defconfig"

    # Current menu in single-menu mode
    _cur_menu = _kconf.top_node

    # Any visible items in the top menu?
    if not _shown_menu_nodes(kconf.top_node):
        # Nothing visible. Start in show-all mode and try again.
        _show_all_var.set(True)
        if not _shown_menu_nodes(kconf.top_node):
            # Give up and show an error. It's nice to be able to assume that
            # the tree is non-empty in the rest of the code.
            _root.wait_visibility()
            messagebox.showerror(
                "Error",
                "Empty configuration -- nothing to configure.\n\n"
                "Check that environment variables are set properly.")
            _root.destroy()
            return

    # Build the initial tree
    _update_tree()

    # Select the first item and focus the Treeview, so that keyboard controls
    # work immediately
    _select(_tree, _tree.get_children()[0])
    _tree.focus_set()

    # Make geometry information available for centering the window. This
    # indirectly creates the window, so hide it so that it's never shown at the
    # old location.
    _root.withdraw()
    _root.update_idletasks()

    # Center the window
    _root.geometry("+{}+{}".format(
        (_root.winfo_screenwidth() - _root.winfo_reqwidth())//2,
        (_root.winfo_screenheight() - _root.winfo_reqheight())//2))

    # Show it
    _root.deiconify()

    # Prevent the window from being automatically resized. Otherwise, it
    # changes size when scrollbars appear/disappear before the user has
    # manually resized it.
    _root.geometry(_root.geometry())

    _root.mainloop()


def _load_config():
    # Loads any existing .config file. See the Kconfig.load_config() docstring.
    #
    # Returns True if .config is missing or outdated. We always prompt for
    # saving the configuration in that case.

    print(_kconf.load_config())
    if not os.path.exists(_conf_filename):
        # No .config
        return True

    return _needs_save()


def _needs_save():
    # Returns True if a just-loaded .config file is outdated (would get
    # modified when saving)

    if _kconf.missing_syms:
        # Assignments to undefined symbols in the .config
        return True

    for sym in _kconf.unique_defined_syms:
        if sym.user_value is None:
            if sym.config_string:
                # Unwritten symbol
                return True
        elif sym.orig_type in (BOOL, TRISTATE):
            if sym.tri_value != sym.user_value:
                # Written bool/tristate symbol, new value
                return True
        elif sym.str_value != sym.user_value:
            # Written string/int/hex symbol, new value
            return True

    # No need to prompt for save
    return False


def _create_id_to_node():
    global _id_to_node

    _id_to_node = {str(id(node)): node for node in _kconf.node_iter()}


def _create_ui():
    # Creates the main window UI

    global _root
    global _tree

    # Create the root window. This initializes Tkinter and makes e.g.
    # PhotoImage available, so do it early.
    _root = Tk()

    _load_images()
    _init_misc_ui()
    _fix_treeview_issues()

    _create_top_widgets()
    # Create the pane with the Kconfig tree and description text
    panedwindow, _tree = _create_kconfig_tree_and_desc(_root)
    panedwindow.grid(column=0, row=1, sticky="nsew")
    _create_status_bar()

    _root.columnconfigure(0, weight=1)
    # Only the pane with the Kconfig tree and description grows vertically
    _root.rowconfigure(1, weight=1)

    # Start with show-name disabled
    _do_showname()

    _tree.bind("<Left>", _tree_left_key)
    _tree.bind("<Right>", _tree_right_key)
    # Note: Binding this for the jump-to tree as well would cause issues due to
    # the Tk bug mentioned in _tree_open()
    _tree.bind("<<TreeviewOpen>>", _tree_open)
    # add=True to avoid overriding the description text update
    _tree.bind("<<TreeviewSelect>>", _update_menu_path, add=True)

    _root.bind("<Control-s>", _save)
    _root.bind("<Control-o>", _open)
    _root.bind("<Control-a>", _toggle_showall)
    _root.bind("<Control-n>", _toggle_showname)
    _root.bind("<Control-m>", _toggle_tree_mode)
    _root.bind("<Control-f>", _jump_to_dialog)
    _root.bind("/", _jump_to_dialog)
    _root.bind("<Escape>", _on_quit)


def _load_images():
    # Loads GIF images, creating the global _*_img PhotoImage variables.
    # Base64-encoded images embedded in this script are used if
    # _USE_EMBEDDED_IMAGES is True, and separate image files in the same
    # directory as the script otherwise.
    #
    # Using a global variable indirectly prevents the image from being
    # garbage-collected. Passing an image to a Tkinter function isn't enough to
    # keep it alive.

    def load_image(name, data):
        var_name = "_{}_img".format(name)

        if _USE_EMBEDDED_IMAGES:
            globals()[var_name] = PhotoImage(data=data, format="gif")
        else:
            globals()[var_name] = PhotoImage(
                file=os.path.join(os.path.dirname(__file__), name + ".gif"),
                format="gif")

    # Note: Base64 data can be put on the clipboard with
    #   $ base64 -w0 foo.gif | xclip

    load_image("icon", "R0lGODlhMAAwAPEDAAAAAADQAO7u7v///yH5BAUKAAMALAAAAAAwADAAAAL/nI+gy+2Pokyv2jazuZxryQjiSJZmyXxHeLbumH6sEATvW8OLNtf5bfLZRLFITzgEipDJ4mYxYv6A0ubuqYhWk66tVTE4enHer7jcKvt0LLUw6P45lvEprT6c0+v7OBuqhYdHohcoqIbSAHc4ljhDwrh1UlgSydRCWWlp5wiYZvmSuSh4IzrqV6p4cwhkCsmY+nhK6uJ6t1mrOhuJqfu6+WYiCiwl7HtLjNSZZZis/MeM7NY3TaRKS40ooDeoiVqIultsrav92bi9c3a5KkkOsOJZpSS99m4k/0zPng4Gks9JSbB+8DIcoQfnjwpZCHv5W+ip4aQrKrB0uOikYhiMCBw1/uPoQUMBADs=")
    load_image("n_bool", "R0lGODdhEAAQAPAAAAgICP///ywAAAAAEAAQAAACIISPacHtvp5kcb5qG85hZ2+BkyiRF8BBaEqtrKkqslEAADs=")
    load_image("y_bool", "R0lGODdhEAAQAPEAAAgICADQAP///wAAACwAAAAAEAAQAAACMoSPacLtvlh4YrIYsst2cV19AvaVF9CUXBNJJoum7ymrsKuCnhiupIWjSSjAFuWhSCIKADs=")
    load_image("n_tri", "R0lGODlhEAAQAPD/AAEBAf///yH5BAUKAAIALAAAAAAQABAAAAInlI+pBrAKQnCPSUlXvFhznlkfeGwjKZhnJ65h6nrfi6h0st2QXikFADs=")
    load_image("m_tri", "R0lGODlhEAAQAPEDAAEBAeQMuv///wAAACH5BAUKAAMALAAAAAAQABAAAAI5nI+pBrAWAhPCjYhiAJQCnWmdoElHGVBoiK5M21ofXFpXRIrgiecqxkuNciZIhNOZFRNI24PhfEoLADs=")
    load_image("y_tri", "R0lGODlhEAAQAPEDAAICAgDQAP///wAAACH5BAUKAAMALAAAAAAQABAAAAI0nI+pBrAYBhDCRRUypfmergmgZ4xjMpmaw2zmxk7cCB+pWiVqp4MzDwn9FhGZ5WFjIZeGAgA7")
    load_image("m_my", "R0lGODlhEAAQAPEDAAAAAOQMuv///wAAACH5BAUKAAMALAAAAAAQABAAAAI5nIGpxiAPI2ghxFinq/ZygQhc94zgZopmOLYf67anGr+oZdp02emfV5n9MEHN5QhqICETxkABbQ4KADs=")
    load_image("y_my", "R0lGODlhEAAQAPH/AAAAAADQAAPRA////yH5BAUKAAQALAAAAAAQABAAAAM+SArcrhCMSSuIM9Q8rxxBWIXawIBkmWonupLd565Um9G1PIs59fKmzw8WnAlusBYR2SEIN6DmAmqBLBxYSAIAOw==")
    load_image("n_locked", "R0lGODlhEAAQAPABAAAAAP///yH5BAUKAAEALAAAAAAQABAAAAIgjB8AyKwN04pu0vMutpqqz4Hih4ydlnUpyl2r23pxUAAAOw==")
    load_image("m_locked", "R0lGODlhEAAQAPD/AAAAAOQMuiH5BAUKAAIALAAAAAAQABAAAAIylC8AyKwN04ohnGcqqlZmfXDWI26iInZoyiore05walolV39ftxsYHgL9QBBMBGFEFAAAOw==")
    load_image("y_locked", "R0lGODlhEAAQAPD/AAAAAADQACH5BAUKAAIALAAAAAAQABAAAAIylC8AyKzNgnlCtoDTwvZwrHydIYpQmR3KWq4uK74IOnp0HQPmnD3cOVlUIAgKsShkFAAAOw==")
    load_image("not_selected", "R0lGODlhEAAQAPD/AAAAAP///yH5BAUKAAIALAAAAAAQABAAAAIrlA2px6IBw2IpWglOvTYhzmUbGD3kNZ5QqrKn2YrqigCxZoMelU6No9gdCgA7")
    load_image("selected", "R0lGODlhEAAQAPD/AAAAAP///yH5BAUKAAIALAAAAAAQABAAAAIzlA2px6IBw2IpWglOvTah/kTZhimASJomiqonlLov1qptHTsgKSEzh9H8QI0QzNPwmRoFADs=")
    load_image("edit", "R0lGODlhEAAQAPIFAAAAAKOLAMuuEPvXCvrxvgAAAAAAAAAAACH5BAUKAAUALAAAAAAQABAAAANCWLqw/gqMBp8cszJxcwVC2FEOEIAi5kVBi3IqWZhuCGMyfdpj2e4pnK+WAshmvxeAcETWlsxPkkBtsqBMa8TIBSQAADs=")


def _fix_treeview_issues():
    # Fixes some Treeview issues

    global _treeview_rowheight

    style = ttk.Style()

    # The treeview rowheight isn't adjusted automatically on high-DPI displays,
    # so do it ourselves. The font will probably always be TkDefaultFont, but
    # play it safe and look it up.

    _treeview_rowheight = font.Font(font=style.lookup("Treeview", "font")) \
        .metrics("linespace") + 2

    style.configure("Treeview", rowheight=_treeview_rowheight)

    # Work around regression in https://core.tcl.tk/tk/tktview?name=509cafafae,
    # which breaks tag background colors

    for option in "foreground", "background":
        # Filter out any styles starting with ("!disabled", "!selected", ...).
        # style.map() returns an empty list for missing options, so this should
        # be future-safe.
        style.map(
            "Treeview",
            **{option: [elm for elm in style.map("Treeview", query_opt=option)
                        if elm[:2] != ("!disabled", "!selected")]})


def _init_misc_ui():
    # Does misc. UI initialization, like setting the title, icon, and theme

    _root.title(_kconf.mainmenu_text)
    # iconphoto() isn't available in Python 2's Tkinter
    _root.tk.call("wm", "iconphoto", _root._w, "-default", _icon_img)
    # Reducing the width of the window to 1 pixel makes it move around, at
    # least on GNOME. Prevent weird stuff like that.
    _root.minsize(128, 128)
    _root.protocol("WM_DELETE_WINDOW", _on_quit)

    # Use the 'clam' theme on *nix if it's available. It looks nicer than the
    # 'default' theme.
    if _root.tk.call("tk", "windowingsystem") == "x11":
        style = ttk.Style()
        if "clam" in style.theme_names():
            style.theme_use("clam")


def _create_top_widgets():
    # Creates the controls above the Kconfig tree in the main window

    global _show_all_var
    global _show_name_var
    global _single_menu_var
    global _menupath
    global _backbutton

    topframe = ttk.Frame(_root)
    topframe.grid(column=0, row=0, sticky="ew")

    ttk.Button(topframe, text="Save", command=_save) \
        .grid(column=0, row=0, sticky="ew", padx=".05c", pady=".05c")

    ttk.Button(topframe, text="Save as...", command=_save_as) \
        .grid(column=1, row=0, sticky="ew")

    ttk.Button(topframe, text="Save minimal (advanced)...",
               command=_save_minimal) \
        .grid(column=2, row=0, sticky="ew", padx=".05c")

    ttk.Button(topframe, text="Open...", command=_open) \
        .grid(column=3, row=0)

    ttk.Button(topframe, text="Jump to...", command=_jump_to_dialog) \
        .grid(column=4, row=0, padx=".05c")

    _show_name_var = BooleanVar()
    ttk.Checkbutton(topframe, text="Show name", command=_do_showname,
                    variable=_show_name_var) \
        .grid(column=0, row=1, sticky="nsew", padx=".05c", pady="0 .05c",
              ipady=".2c")

    _show_all_var = BooleanVar()
    ttk.Checkbutton(topframe, text="Show all", command=_do_showall,
                    variable=_show_all_var) \
        .grid(column=1, row=1, sticky="nsew", pady="0 .05c")

    # Allow the show-all and single-menu status to be queried via plain global
    # Python variables, which is faster and simpler

    def show_all_updated(*_):
        global _show_all
        _show_all = _show_all_var.get()

    _trace_write(_show_all_var, show_all_updated)
    _show_all_var.set(False)

    _single_menu_var = BooleanVar()
    ttk.Checkbutton(topframe, text="Single-menu mode", command=_do_tree_mode,
                    variable=_single_menu_var) \
        .grid(column=2, row=1, sticky="nsew", padx=".05c", pady="0 .05c")

    _backbutton = ttk.Button(topframe, text="<--", command=_leave_menu,
                             state="disabled")
    _backbutton.grid(column=0, row=4, sticky="nsew", padx=".05c", pady="0 .05c")

    def tree_mode_updated(*_):
        global _single_menu
        _single_menu = _single_menu_var.get()

        if _single_menu:
            _backbutton.grid()
        else:
            _backbutton.grid_remove()

    _trace_write(_single_menu_var, tree_mode_updated)
    _single_menu_var.set(False)

    # Column to the right of the buttons that the menu path extends into, so
    # that it can grow wider than the buttons
    topframe.columnconfigure(5, weight=1)

    _menupath = ttk.Label(topframe)
    _menupath.grid(column=0, row=3, columnspan=6, sticky="w", padx="0.05c",
                   pady="0 .05c")


def _create_kconfig_tree_and_desc(parent):
    # Creates a Panedwindow with a Treeview that shows Kconfig nodes and a Text
    # that shows a description of the selected node. Returns a tuple with the
    # Panedwindow and the Treeview. This code is shared between the main window
    # and the jump-to dialog.

    panedwindow = ttk.Panedwindow(parent, orient=VERTICAL)

    tree_frame, tree = _create_kconfig_tree(panedwindow)
    desc_frame, desc = _create_kconfig_desc(panedwindow)

    panedwindow.add(tree_frame, weight=1)
    panedwindow.add(desc_frame)

    def tree_select(_):
        # The Text widget does not allow editing the text in its disabled
        # state. We need to temporarily enable it.
        desc["state"] = "normal"

        sel = tree.selection()
        if not sel:
            desc.delete("1.0", "end")
            desc["state"] = "disabled"
            return

        # Text.replace() is not available in Python 2's Tkinter
        desc.delete("1.0", "end")
        desc.insert("end", _info_str(_id_to_node[sel[0]]))

        desc["state"] = "disabled"

    tree.bind("<<TreeviewSelect>>", tree_select)
    tree.bind("<1>", _tree_click)
    tree.bind("<Double-1>", _tree_double_click)
    tree.bind("<Return>", _tree_enter)
    tree.bind("<KP_Enter>", _tree_enter)
    tree.bind("<space>", _tree_toggle)
    tree.bind("n", _tree_set_val(0))
    tree.bind("m", _tree_set_val(1))
    tree.bind("y", _tree_set_val(2))

    return panedwindow, tree


def _create_kconfig_tree(parent):
    # Creates a Treeview for showing Kconfig nodes

    frame = ttk.Frame(parent)

    tree = ttk.Treeview(frame, selectmode="browse", height=20,
                        columns=("name",))
    tree.heading("#0", text="Option", anchor="w")
    tree.heading("name", text="Name", anchor="w")

    tree.tag_configure("n-bool", image=_n_bool_img)
    tree.tag_configure("y-bool", image=_y_bool_img)
    tree.tag_configure("m-tri", image=_m_tri_img)
    tree.tag_configure("n-tri", image=_n_tri_img)
    tree.tag_configure("m-tri", image=_m_tri_img)
    tree.tag_configure("y-tri", image=_y_tri_img)
    tree.tag_configure("m-my", image=_m_my_img)
    tree.tag_configure("y-my", image=_y_my_img)
    tree.tag_configure("n-locked", image=_n_locked_img)
    tree.tag_configure("m-locked", image=_m_locked_img)
    tree.tag_configure("y-locked", image=_y_locked_img)
    tree.tag_configure("not-selected", image=_not_selected_img)
    tree.tag_configure("selected", image=_selected_img)
    tree.tag_configure("edit", image=_edit_img)
    tree.tag_configure("invisible", foreground="red")

    tree.grid(column=0, row=0, sticky="nsew")

    _add_vscrollbar(frame, tree)

    frame.columnconfigure(0, weight=1)
    frame.rowconfigure(0, weight=1)

    # Create items for all menu nodes. These can be detached/moved later.
    # Micro-optimize this a bit.
    insert = tree.insert
    id_ = id
    Symbol_ = Symbol
    for node in _kconf.node_iter():
        item = node.item
        insert("", "end", iid=id_(node),
               values=item.name if item.__class__ is Symbol_ else "")

    return frame, tree


def _create_kconfig_desc(parent):
    # Creates a Text for showing the description of the selected Kconfig node

    frame = ttk.Frame(parent)

    desc = Text(frame, height=12, wrap="none", borderwidth=0,
                state="disabled")
    desc.grid(column=0, row=0, sticky="nsew")

    # Work around not being to Ctrl-C/V text from a disabled Text widget, with a
    # tip found in https://stackoverflow.com/questions/3842155/is-there-a-way-to-make-the-tkinter-text-widget-read-only
    desc.bind("<1>", lambda _: desc.focus_set())

    _add_vscrollbar(frame, desc)

    frame.columnconfigure(0, weight=1)
    frame.rowconfigure(0, weight=1)

    return frame, desc


def _add_vscrollbar(parent, widget):
    # Adds a vertical scrollbar to 'widget' that's only shown as needed

    vscrollbar = ttk.Scrollbar(parent, orient="vertical",
                               command=widget.yview)
    vscrollbar.grid(column=1, row=0, sticky="ns")

    def yscrollcommand(first, last):
        # Only show the scrollbar when needed. 'first' and 'last' are
        # strings.
        if float(first) <= 0.0 and float(last) >= 1.0:
            vscrollbar.grid_remove()
        else:
            vscrollbar.grid()

        vscrollbar.set(first, last)

    widget["yscrollcommand"] = yscrollcommand


def _create_status_bar():
    # Creates the status bar at the bottom of the main window

    global _status_label

    _status_label = ttk.Label(_root, anchor="e", padding="0 0 0.4c 0")
    _status_label.grid(column=0, row=3, sticky="ew")


def _set_status(s):
    # Sets the text in the status bar to 's'

    _status_label["text"] = s


def _set_conf_changed(changed):
    # Updates the status re. whether there are unsaved changes

    global _conf_changed

    _conf_changed = changed
    if changed:
        _set_status("Modified")


def _update_tree():
    # Updates the Kconfig tree in the main window by first detaching all nodes
    # and then updating and reattaching them. The tree structure might have
    # changed.

    # If a selected/focused item is detached and later reattached, it stays
    # selected/focused. That can give multiple selections even though
    # selectmode=browse. Save and later restore the selection and focus as a
    # workaround.
    old_selection = _tree.selection()
    old_focus = _tree.focus()

    # Detach all tree items before re-stringing them. This is relatively fast,
    # luckily.
    _tree.detach(*_id_to_node.keys())

    if _single_menu:
        _build_menu_tree()
    else:
        _build_full_tree(_kconf.top_node)

    _tree.selection_set(old_selection)
    _tree.focus(old_focus)


def _build_full_tree(menu):
    # Updates the tree starting from menu.list, in full-tree mode. To speed
    # things up, only open menus are updated. The menu-at-a-time logic here is
    # to deal with invisible items that can show up outside show-all mode (see
    # _shown_full_nodes()).

    for node in _shown_full_nodes(menu):
        _add_to_tree(node, _kconf.top_node)

        # _shown_full_nodes() includes nodes from menus rooted at symbols, so
        # we only need to check "real" menus/choices here
        if node.list and not isinstance(node.item, Symbol):
            if _tree.item(id(node), "open"):
                _build_full_tree(node)
            else:
                # We're just probing here, so _shown_menu_nodes() will work
                # fine, and might be a bit faster
                shown = _shown_menu_nodes(node)
                if shown:
                    # Dummy element to make the open/closed toggle appear
                    _tree.move(id(shown[0]), id(shown[0].parent), "end")


def _shown_full_nodes(menu):
    # Returns the list of menu nodes shown in 'menu' (a menu node for a menu)
    # for full-tree mode. A tricky detail is that invisible items need to be
    # shown if they have visible children.

    def rec(node):
        res = []

        while node:
            if _visible(node) or _show_all:
                res.append(node)
                if node.list and isinstance(node.item, Symbol):
                    # Nodes from menu created from dependencies
                    res += rec(node.list)

            elif node.list and isinstance(node.item, Symbol):
                # Show invisible symbols (defined with either 'config' and
                # 'menuconfig') if they have visible children. This can happen
                # for an m/y-valued symbol with an optional prompt
                # ('prompt "foo" is COND') that is currently disabled.
                shown_children = rec(node.list)
                if shown_children:
                    res.append(node)
                    res += shown_children

            node = node.next

        return res

    return rec(menu.list)


def _build_menu_tree():
    # Updates the tree in single-menu mode. See _build_full_tree() as well.

    for node in _shown_menu_nodes(_cur_menu):
        _add_to_tree(node, _cur_menu)


def _shown_menu_nodes(menu):
    # Used for single-menu mode. Similar to _shown_full_nodes(), but doesn't
    # include children of symbols defined with 'menuconfig'.

    def rec(node):
        res = []

        while node:
            if _visible(node) or _show_all:
                res.append(node)
                if node.list and not node.is_menuconfig:
                    res += rec(node.list)

            elif node.list and isinstance(node.item, Symbol):
                shown_children = rec(node.list)
                if shown_children:
                    # Invisible item with visible children
                    res.append(node)
                    if not node.is_menuconfig:
                        res += shown_children

            node = node.next

        return res

    return rec(menu.list)


def _visible(node):
    # Returns True if the node should appear in the menu (outside show-all
    # mode)

    return node.prompt and expr_value(node.prompt[1]) and not \
        (node.item == MENU and not expr_value(node.visibility))


def _add_to_tree(node, top):
    # Adds 'node' to the tree, at the end of its menu. We rely on going through
    # the nodes linearly to get the correct order. 'top' holds the menu that
    # corresponds to the top-level menu, and can vary in single-menu mode.

    parent = node.parent
    _tree.move(id(node), "" if parent is top else id(parent), "end")
    _tree.item(
        id(node),
        text=_node_str(node),
        # The _show_all test avoids showing invisible items in red outside
        # show-all mode, which could look confusing/broken. Invisible symbols
        # are shown outside show-all mode if an invisible symbol has visible
        # children in an implicit menu.
        tags=_img_tag(node) if _visible(node) or not _show_all else
            _img_tag(node) + " invisible")


def _node_str(node):
    # Returns the string shown to the right of the image (if any) for the node

    if node.prompt:
        if node.item == COMMENT:
            s = "*** {} ***".format(node.prompt[0])
        else:
            s = node.prompt[0]

        if isinstance(node.item, Symbol):
            sym = node.item

            # Print "(NEW)" next to symbols without a user value (from e.g. a
            # .config), but skip it for choice symbols in choices in y mode,
            # and for symbols of UNKNOWN type (which generate a warning though)
            if sym.user_value is None and sym.type and not \
                (sym.choice and sym.choice.tri_value == 2):

                s += " (NEW)"

    elif isinstance(node.item, Symbol):
        # Symbol without prompt (can show up in show-all)
        s = "<{}>".format(node.item.name)

    else:
        # Choice without prompt. Use standard_sc_expr_str() so that it shows up
        # as '<choice (name if any)>'.
        s = standard_sc_expr_str(node.item)


    if isinstance(node.item, Symbol):
        sym = node.item
        if sym.orig_type == STRING:
            s += ": " + sym.str_value
        elif sym.orig_type in (INT, HEX):
            s = "({}) {}".format(sym.str_value, s)

    elif isinstance(node.item, Choice) and node.item.tri_value == 2:
        # Print the prompt of the selected symbol after the choice for
        # choices in y mode
        sym = node.item.selection
        if sym:
            for sym_node in sym.nodes:
                # Use the prompt used at this choice location, in case the
                # choice symbol is defined in multiple locations
                if sym_node.parent is node and sym_node.prompt:
                    s += " ({})".format(sym_node.prompt[0])
                    break
            else:
                # If the symbol isn't defined at this choice location, then
                # just use whatever prompt we can find for it
                for sym_node in sym.nodes:
                    if sym_node.prompt:
                        s += " ({})".format(sym_node.prompt[0])
                        break

    # In single-menu mode, print "--->" next to nodes that have menus that can
    # potentially be entered. Print "----" if the menu is empty. We don't allow
    # those to be entered.
    if _single_menu and node.is_menuconfig:
        s += "  --->" if _shown_menu_nodes(node) else "  ----"

    return s


def _img_tag(node):
    # Returns the tag for the image that should be shown next to 'node', or the
    # empty string if it shouldn't have an image

    item = node.item

    if item in (MENU, COMMENT) or not item.orig_type:
        return ""

    if item.orig_type in (STRING, INT, HEX):
        return "edit"

    # BOOL or TRISTATE

    if _is_y_mode_choice_sym(item):
        # Choice symbol in y-mode choice
        return "selected" if item.choice.selection is item else "not-selected"

    if len(item.assignable) <= 1:
        # Pinned to a single value
        return "" if isinstance(item, Choice) else item.str_value + "-locked"

    if item.type == BOOL:
        return item.str_value + "-bool"

    # item.type == TRISTATE
    if item.assignable == (1, 2):
        return item.str_value + "-my"
    return item.str_value + "-tri"


def _is_y_mode_choice_sym(item):
    # The choice mode is an upper bound on the visibility of choice symbols, so
    # we can check the choice symbols' own visibility to see if the choice is
    # in y mode
    return isinstance(item, Symbol) and item.choice and item.visibility == 2


def _tree_click(event):
    # Click on the Kconfig Treeview

    tree = event.widget
    if tree.identify_element(event.x, event.y) == "image":
        item = tree.identify_row(event.y)
        # Select the item before possibly popping up a dialog for
        # string/int/hex items, so that its help is visible
        _select(tree, item)
        _change_node(_id_to_node[item], tree.winfo_toplevel())
        return "break"


def _tree_double_click(event):
    # Double-click on the Kconfig treeview

    # Do an extra check to avoid weirdness when double-clicking in the tree
    # heading area
    if not _in_heading(event):
        return _tree_enter(event)


def _in_heading(event):
    # Returns True if 'event' took place in the tree heading

    tree = event.widget
    return hasattr(tree, "identify_region") and \
        tree.identify_region(event.x, event.y) in ("heading", "separator")


def _tree_enter(event):
    # Enter press or double-click within the Kconfig treeview. Prefer to
    # open/close/enter menus, but toggle the value if that's not possible.

    tree = event.widget
    sel = tree.focus()
    if sel:
        node = _id_to_node[sel]

        if tree.get_children(sel):
            _tree_toggle_open(sel)
        elif _single_menu_mode_menu(node, tree):
            _enter_menu_and_select_first(node)
        else:
            _change_node(node, tree.winfo_toplevel())

        return "break"


def _tree_toggle(event):
    # Space press within the Kconfig treeview. Prefer to toggle the value, but
    # open/close/enter the menu if that's not possible.

    tree = event.widget
    sel = tree.focus()
    if sel:
        node = _id_to_node[sel]

        if _changeable(node):
            _change_node(node, tree.winfo_toplevel())
        elif _single_menu_mode_menu(node, tree):
            _enter_menu_and_select_first(node)
        elif tree.get_children(sel):
            _tree_toggle_open(sel)

        return "break"


def _tree_left_key(_):
    # Left arrow key press within the Kconfig treeview

    if _single_menu:
        # Leave the current menu in single-menu mode
        _leave_menu()
        return "break"

    # Otherwise, default action


def _tree_right_key(_):
    # Right arrow key press within the Kconfig treeview

    sel = _tree.focus()
    if sel:
        node = _id_to_node[sel]
        # If the node can be entered in single-menu mode, do it
        if _single_menu_mode_menu(node, _tree):
            _enter_menu_and_select_first(node)
            return "break"

    # Otherwise, default action


def _single_menu_mode_menu(node, tree):
    # Returns True if single-menu mode is on and 'node' is an (interface)
    # menu that can be entered

    return _single_menu and tree is _tree and node.is_menuconfig and \
           _shown_menu_nodes(node)


def _changeable(node):
    # Returns True if 'node' is a Symbol/Choice whose value can be changed

    sc = node.item

    if not isinstance(sc, (Symbol, Choice)):
        return False

    # This will hit for invisible symbols, which appear in show-all mode and
    # when an invisible symbol has visible children (which can happen e.g. for
    # symbols with optional prompts)
    if not (node.prompt and expr_value(node.prompt[1])):
        return False

    return sc.orig_type in (STRING, INT, HEX) or len(sc.assignable) > 1 \
           or _is_y_mode_choice_sym(sc)


def _tree_toggle_open(item):
    # Opens/closes the Treeview item 'item'

    if _tree.item(item, "open"):
        _tree.item(item, open=False)
    else:
        node = _id_to_node[item]
        if not isinstance(node.item, Symbol):
            # Can only get here in full-tree mode
            _build_full_tree(node)
        _tree.item(item, open=True)


def _tree_set_val(tri_val):
    def tree_set_val(event):
        # n/m/y press within the Kconfig treeview

        # Sets the value of the currently selected item to 'tri_val', if that
        # value can be assigned

        sel = event.widget.focus()
        if sel:
            sc = _id_to_node[sel].item
            if isinstance(sc, (Symbol, Choice)) and tri_val in sc.assignable:
                _set_val(sc, tri_val)

    return tree_set_val


def _tree_open(_):
    # Lazily populates the Kconfig tree when menus are opened in full-tree mode

    if _single_menu:
        # Work around https://core.tcl.tk/tk/tktview?name=368fa4561e
        # ("ttk::treeview open/closed indicators can be toggled while hidden").
        # Clicking on the hidden indicator will call _build_full_tree() in
        # single-menu mode otherwise.
        return

    node = _id_to_node[_tree.focus()]
    # _shown_full_nodes() includes nodes from menus rooted at symbols, so we
    # only need to check "real" menus and choices here
    if not isinstance(node.item, Symbol):
        _build_full_tree(node)


def _update_menu_path(_):
    # Updates the displayed menu path when nodes are selected in the Kconfig
    # treeview

    sel = _tree.selection()
    _menupath["text"] = _menu_path_info(_id_to_node[sel[0]]) if sel else ""


def _item_row(item):
    # Returns the row number 'item' appears on within the Kconfig treeview,
    # starting from the top of the tree. Used to preserve scrolling.
    #
    # ttkTreeview.c in the Tk sources defines a RowNumber() function that does
    # the same thing, but it's not exposed.

    row = 0

    while True:
        prev = _tree.prev(item)
        if prev:
            item = prev
            row += _n_rows(item)
        else:
            item = _tree.parent(item)
            if not item:
                return row
            row += 1


def _n_rows(item):
    # _item_row() helper. Returns the number of rows occupied by 'item' and #
    # its children.

    rows = 1

    if _tree.item(item, "open"):
        for child in _tree.get_children(item):
            rows += _n_rows(child)

    return rows


def _attached(item):
    # Heuristic for checking if a Treeview item is attached. Doesn't seem to be
    # good APIs for this. Might fail for super-obscure cases with tiny trees,
    # but you'd just get a small scroll mess-up.

    return bool(_tree.next(item) or _tree.prev(item) or _tree.parent(item))


def _change_node(node, parent):
    # Toggles/changes the value of 'node'. 'parent' is the parent window
    # (either the main window or the jump-to dialog), in case we need to pop up
    # a dialog.

    if not _changeable(node):
        return

    # sc = symbol/choice
    sc = node.item

    if sc.type in (INT, HEX, STRING):
        s = _set_val_dialog(node, parent)

        # Tkinter can return 'unicode' strings on Python 2, which Kconfiglib
        # can't deal with. UTF-8-encode the string to work around it.
        if _PY2 and isinstance(s, unicode):
            s = s.encode("utf-8", "ignore")

        if s is not None:
            _set_val(sc, s)

    elif len(sc.assignable) == 1:
        # Handles choice symbols for choices in y mode, which are a special
        # case: .assignable can be (2,) while .tri_value is 0.
        _set_val(sc, sc.assignable[0])

    else:
        # Set the symbol to the value after the current value in
        # sc.assignable, with wrapping
        val_index = sc.assignable.index(sc.tri_value)
        _set_val(sc, sc.assignable[(val_index + 1) % len(sc.assignable)])


def _set_val(sc, val):
    # Wrapper around Symbol/Choice.set_value() for updating the menu state and
    # _conf_changed

    # Use the string representation of tristate values. This makes the format
    # consistent for all symbol types.
    if val in TRI_TO_STR:
        val = TRI_TO_STR[val]

    if val != sc.str_value:
        sc.set_value(val)
        _set_conf_changed(True)

        # Update the tree and try to preserve the scroll. Do a cheaper variant
        # than in the show-all case, that might mess up the scroll slightly in
        # rare cases, but is fast and flicker-free.

        stayput = _loc_ref_item()  # Item to preserve scroll for
        old_row = _item_row(stayput)

        _update_tree()

        # If the reference item disappeared (can happen if the change was done
        # from the jump-to dialog), then avoid messing with the scroll and hope
        # for the best
        if _attached(stayput):
            _tree.yview_scroll(_item_row(stayput) - old_row, "units")

        if _jump_to_tree:
            _update_jump_to_display()


def _set_val_dialog(node, parent):
    # Pops up a dialog for setting the value of the string/int/hex
    # symbol at node 'node'. 'parent' is the parent window.

    def ok(_=None):
        # No 'nonlocal' in Python 2
        global _entry_res

        s = entry.get()
        if sym.type == HEX and not s.startswith(("0x", "0X")):
            s = "0x" + s

        if _check_valid(dialog, entry, sym, s):
            _entry_res = s
            dialog.destroy()

    def cancel(_=None):
        global _entry_res
        _entry_res = None
        dialog.destroy()

    sym = node.item

    dialog = Toplevel(parent)
    dialog.title("Enter {} value".format(TYPE_TO_STR[sym.type]))
    dialog.resizable(False, False)
    dialog.transient(parent)
    dialog.protocol("WM_DELETE_WINDOW", cancel)

    ttk.Label(dialog, text=node.prompt[0] + ":") \
        .grid(column=0, row=0, columnspan=2, sticky="w", padx=".3c",
              pady=".2c .05c")

    entry = ttk.Entry(dialog, width=30)
    # Start with the previous value in the editbox, selected
    entry.insert(0, sym.str_value)
    entry.selection_range(0, "end")
    entry.grid(column=0, row=1, columnspan=2, sticky="ew", padx=".3c")
    entry.focus_set()

    range_info = _range_info(sym)
    if range_info:
        ttk.Label(dialog, text=range_info) \
            .grid(column=0, row=2, columnspan=2, sticky="w", padx=".3c",
                  pady=".2c 0")

    ttk.Button(dialog, text="OK", command=ok) \
        .grid(column=0, row=4 if range_info else 3, sticky="e", padx=".3c",
              pady=".4c")

    ttk.Button(dialog, text="Cancel", command=cancel) \
        .grid(column=1, row=4 if range_info else 3, padx="0 .3c")

    # Give all horizontal space to the grid cell with the OK button, so that
    # Cancel moves to the right
    dialog.columnconfigure(0, weight=1)

    _center_on_root(dialog)

    # Hack to scroll the entry so that the end of the text is shown, from
    # https://stackoverflow.com/questions/29334544/why-does-tkinters-entry-xview-moveto-fail.
    # Related Tk ticket: https://core.tcl.tk/tk/info/2513186fff
    def scroll_entry(_):
        _root.update_idletasks()
        entry.unbind("<Expose>")
        entry.xview_moveto(1)
    entry.bind("<Expose>", scroll_entry)

    # The dialog must be visible before we can grab the input
    dialog.wait_visibility()
    dialog.grab_set()

    dialog.bind("<Return>", ok)
    dialog.bind("<KP_Enter>", ok)
    dialog.bind("<Escape>", cancel)

    # Wait for the user to be done with the dialog
    parent.wait_window(dialog)

    # Regrab the input in the parent
    parent.grab_set()

    return _entry_res


def _center_on_root(dialog):
    # Centers 'dialog' on the root window. It often ends up at some bad place
    # like the top-left corner of the screen otherwise. See the menuconfig()
    # function, which has similar logic.

    dialog.withdraw()
    _root.update_idletasks()

    dialog_width = dialog.winfo_reqwidth()
    dialog_height = dialog.winfo_reqheight()

    screen_width = _root.winfo_screenwidth()
    screen_height = _root.winfo_screenheight()

    x = _root.winfo_rootx() + (_root.winfo_width() - dialog_width)//2
    y = _root.winfo_rooty() + (_root.winfo_height() - dialog_height)//2

    # Clamp so that no part of the dialog is outside the screen
    if x + dialog_width > screen_width:
        x = screen_width - dialog_width
    elif x < 0:
        x = 0
    if y + dialog_height > screen_height:
        y = screen_height - dialog_height
    elif y < 0:
        y = 0

    dialog.geometry("+{}+{}".format(x, y))

    dialog.deiconify()


def _check_valid(dialog, entry, sym, s):
    # Returns True if the string 's' is a well-formed value for 'sym'.
    # Otherwise, pops up an error and returns False.

    if sym.type not in (INT, HEX):
        # Anything goes for non-int/hex symbols
        return True

    base = 10 if sym.type == INT else 16
    try:
        int(s, base)
    except ValueError:
        messagebox.showerror(
            "Bad value",
            "'{}' is a malformed {} value".format(
                s, TYPE_TO_STR[sym.type]),
            parent=dialog)
        entry.focus_set()
        return False

    for low_sym, high_sym, cond in sym.ranges:
        if expr_value(cond):
            low_s = low_sym.str_value
            high_s = high_sym.str_value

            if not int(low_s, base) <= int(s, base) <= int(high_s, base):
                messagebox.showerror(
                    "Value out of range",
                    "{} is outside the range {}-{}".format(s, low_s, high_s),
                    parent=dialog)
                entry.focus_set()
                return False

            break

    return True


def _range_info(sym):
    # Returns a string with information about the valid range for the symbol
    # 'sym', or None if 'sym' doesn't have a range

    if sym.type in (INT, HEX):
        for low, high, cond in sym.ranges:
            if expr_value(cond):
                return "Range: {}-{}".format(low.str_value, high.str_value)

    return None


def _save(_=None):
    # Tries to save the configuration

    if _try_save(_kconf.write_config, _conf_filename, "configuration"):
        _set_conf_changed(False)

    _tree.focus_set()


def _save_as():
    # Pops up a dialog for saving the configuration to a specific location

    global _conf_filename

    filename = _conf_filename
    while True:
        filename = filedialog.asksaveasfilename(
            title="Save configuration as",
            initialdir=os.path.dirname(filename),
            initialfile=os.path.basename(filename),
            parent=_root)

        if not filename:
            break

        if _try_save(_kconf.write_config, filename, "configuration"):
            _conf_filename = filename
            break

    _tree.focus_set()


def _save_minimal():
    # Pops up a dialog for saving a minimal configuration (defconfig) to a
    # specific location

    global _minconf_filename

    filename = _minconf_filename
    while True:
        filename = filedialog.asksaveasfilename(
            title="Save minimal configuration as",
            initialdir=os.path.dirname(filename),
            initialfile=os.path.basename(filename),
            parent=_root)

        if not filename:
            break

        if _try_save(_kconf.write_min_config, filename,
                     "minimal configuration"):

            _minconf_filename = filename
            break

    _tree.focus_set()


def _open(_=None):
    # Pops up a dialog for loading a configuration

    global _conf_filename

    if _conf_changed and \
        not messagebox.askokcancel(
            "Unsaved changes",
            "You have unsaved changes. Load new configuration anyway?"):

        return

    filename = _conf_filename
    while True:
        filename = filedialog.askopenfilename(
            title="Open configuration",
            initialdir=os.path.dirname(filename),
            initialfile=os.path.basename(filename),
            parent=_root)

        if not filename:
            break

        if _try_load(filename):
            # Maybe something fancier could be done here later to try to
            # preserve the scroll

            _conf_filename = filename
            _set_conf_changed(_needs_save())

            if _single_menu and not _shown_menu_nodes(_cur_menu):
                # Turn on show-all if we're in single-menu mode and would end
                # up with an empty menu
                _show_all_var.set(True)

            _update_tree()

            break

    _tree.focus_set()


def _toggle_showname(_):
    # Toggles show-name mode on/off

    _show_name_var.set(not _show_name_var.get())
    _do_showname()


def _do_showname():
    # Updates the UI for the current show-name setting

    # Columns do not automatically shrink/expand, so we have to update
    # column widths ourselves

    tree_width = _tree.winfo_width()

    if _show_name_var.get():
        _tree["displaycolumns"] = ("name",)
        _tree["show"] = "tree headings"
        name_width = tree_width//3
        _tree.column("#0", width=max(tree_width - name_width, 1))
        _tree.column("name", width=name_width)
    else:
        _tree["displaycolumns"] = ()
        _tree["show"] = "tree"
        _tree.column("#0", width=tree_width)

    _tree.focus_set()


def _toggle_showall(_):
    # Toggles show-all mode on/off

    _show_all_var.set(not _show_all)
    _do_showall()


def _do_showall():
    # Updates the UI for the current show-all setting

    # Don't allow turning off show-all if we'd end up with no visible nodes
    if _nothing_shown():
        _show_all_var.set(True)
        return

    # Save scroll information. old_scroll can end up negative here, if the
    # reference item isn't shown (only invisible items on the screen, and
    # show-all being turned off).

    stayput = _vis_loc_ref_item()
    # Probe the middle of the first row, to play it safe. identify_row(0) seems
    # to return the row before the top row.
    old_scroll = _item_row(stayput) - \
        _item_row(_tree.identify_row(_treeview_rowheight//2))

    _update_tree()

    if _show_all:
        # Deep magic: Unless we call update_idletasks(), the scroll adjustment
        # below is restricted to the height of the old tree, instead of the
        # height of the new tree. Since the tree with show-all on is guaranteed
        # to be taller, and we want the maximum range, we only call it when
        # turning show-all on.
        #
        # Strictly speaking, something similar ought to be done when changing
        # symbol values, but it causes annoying flicker, and in 99% of cases
        # things work anyway there (with usually minor scroll mess-ups in the
        # 1% case).
        _root.update_idletasks()

    # Restore scroll
    _tree.yview(_item_row(stayput) - old_scroll)

    _tree.focus_set()


def _nothing_shown():
    # _do_showall() helper. Returns True if no nodes would get
    # shown with the current show-all setting. Also handles the
    # (obscure) case when there are no visible nodes in the entire
    # tree, meaning guiconfig was automatically started in
    # show-all mode, which mustn't be turned off.

    return not _shown_menu_nodes(
        _cur_menu if _single_menu else _kconf.top_node)


def _toggle_tree_mode(_):
    # Toggles single-menu mode on/off

    _single_menu_var.set(not _single_menu)
    _do_tree_mode()


def _do_tree_mode():
    # Updates the UI for the current tree mode (full-tree or single-menu)

    loc_ref_node = _id_to_node[_loc_ref_item()]

    if not _single_menu:
        # _jump_to() -> _enter_menu() already updates the tree, but
        # _jump_to() -> load_parents() doesn't, because it isn't always needed.
        # We always need to update the tree here, e.g. to add/remove "--->".
        _update_tree()

    _jump_to(loc_ref_node)
    _tree.focus_set()


def _enter_menu_and_select_first(menu):
    # Enters the menu 'menu' and selects the first item. Used in single-menu
    # mode.

    _enter_menu(menu)
    _select(_tree, _tree.get_children()[0])


def _enter_menu(menu):
    # Enters the menu 'menu'. Used in single-menu mode.

    global _cur_menu

    _cur_menu = menu
    _update_tree()

    _backbutton["state"] = "disabled" if menu is _kconf.top_node else "normal"


def _leave_menu():
    # Leaves the current menu. Used in single-menu mode.

    global _cur_menu

    if _cur_menu is not _kconf.top_node:
        old_menu = _cur_menu

        _cur_menu = _parent_menu(_cur_menu)
        _update_tree()

        _select(_tree, id(old_menu))

        if _cur_menu is _kconf.top_node:
            _backbutton["state"] = "disabled"

    _tree.focus_set()


def _select(tree, item):
    # Selects, focuses, and see()s 'item' in 'tree'

    tree.selection_set(item)
    tree.focus(item)
    tree.see(item)


def _loc_ref_item():
    # Returns a Treeview item that can serve as a reference for the current
    # scroll location. We try to make this item stay on the same row on the
    # screen when updating the tree.

    # If the selected item is visible, use that
    sel = _tree.selection()
    if sel and _tree.bbox(sel[0]):
        return sel[0]

    # Otherwise, use the middle item on the screen. If it doesn't exist, the
    # tree is probably really small, so use the first item in the entire tree.
    return _tree.identify_row(_tree.winfo_height()//2) or \
        _tree.get_children()[0]


def _vis_loc_ref_item():
    # Like _loc_ref_item(), but finds a visible item around the reference item.
    # Used when changing show-all mode, where non-visible (red) items will
    # disappear.

    item = _loc_ref_item()

    vis_before = _vis_before(item)
    if vis_before and _tree.bbox(vis_before):
        return vis_before

    vis_after = _vis_after(item)
    if vis_after and _tree.bbox(vis_after):
        return vis_after

    return vis_before or vis_after


def _vis_before(item):
    # _vis_loc_ref_item() helper. Returns the first visible (not red) item,
    # searching backwards from 'item'.

    while item:
        if not _tree.tag_has("invisible", item):
            return item

        prev = _tree.prev(item)
        item = prev if prev else _tree.parent(item)

    return None


def _vis_after(item):
    # _vis_loc_ref_item() helper. Returns the first visible (not red) item,
    # searching forwards from 'item'.

    while item:
        if not _tree.tag_has("invisible", item):
            return item

        next = _tree.next(item)
        if next:
            item = next
        else:
            item = _tree.parent(item)
            if not item:
                break
            item = _tree.next(item)

    return None


def _on_quit(_=None):
    # Called when the user wants to exit

    if not _conf_changed:
        _quit("No changes to save (for '{}')".format(_conf_filename))
        return

    while True:
        ync = messagebox.askyesnocancel("Quit", "Save changes?")
        if ync is None:
            return

        if not ync:
            _quit("Configuration ({}) was not saved".format(_conf_filename))
            return

        if _try_save(_kconf.write_config, _conf_filename, "configuration"):
            # _try_save() already prints the "Configuration saved to ..."
            # message
            _quit()
            return


def _quit(msg=None):
    # Quits the application

    # Do not call sys.exit() here, in case we're being run from a script
    _root.destroy()
    if msg:
        print(msg)


def _try_save(save_fn, filename, description):
    # Tries to save a configuration file. Pops up an error and returns False on
    # failure.
    #
    # save_fn:
    #   Function to call with 'filename' to save the file
    #
    # description:
    #   String describing the thing being saved

    try:
        # save_fn() returns a message to print
        msg = save_fn(filename)
        _set_status(msg)
        print(msg)
        return True
    except EnvironmentError as e:
        messagebox.showerror(
            "Error saving " + description,
            "Error saving {} to '{}': {} (errno: {})"
            .format(description, e.filename, e.strerror,
                    errno.errorcode[e.errno]))
        return False


def _try_load(filename):
    # Tries to load a configuration file. Pops up an error and returns False on
    # failure.
    #
    # filename:
    #   Configuration file to load

    try:
        msg = _kconf.load_config(filename)
        _set_status(msg)
        print(msg)
        return True
    except EnvironmentError as e:
        messagebox.showerror(
            "Error loading configuration",
            "Error loading '{}': {} (errno: {})"
            .format(filename, e.strerror, errno.errorcode[e.errno]))
        return False


def _jump_to_dialog(_=None):
    # Pops up a dialog for jumping directly to a particular node. Symbol values
    # can also be changed within the dialog.
    #
    # Note: There's nothing preventing this from doing an incremental search
    # like menuconfig.py does, but currently it's a bit jerky for large Kconfig
    # trees, at least when inputting the beginning of the search string. We'd
    # need to somehow only update the tree items that are shown in the Treeview
    # to fix it.

    global _jump_to_tree

    def search(_=None):
        _update_jump_to_matches(msglabel, entry.get())

    def jump_to_selected(event=None):
        # Jumps to the selected node and closes the dialog

        # Ignore double clicks on the image and in the heading area
        if event and (tree.identify_element(event.x, event.y) == "image" or
                      _in_heading(event)):
            return

        sel = tree.selection()
        if not sel:
            return

        node = _id_to_node[sel[0]]

        if node not in _shown_menu_nodes(_parent_menu(node)):
            _show_all_var.set(True)
            if not _single_menu:
                # See comment in _do_tree_mode()
                _update_tree()

        _jump_to(node)

        dialog.destroy()

    def tree_select(_):
        jumpto_button["state"] = "normal" if tree.selection() else "disabled"


    dialog = Toplevel(_root)
    dialog.geometry("+{}+{}".format(
        _root.winfo_rootx() + 50, _root.winfo_rooty() + 50))
    dialog.title("Jump to symbol/choice/menu/comment")
    dialog.minsize(128, 128)  # See _create_ui()
    dialog.transient(_root)

    ttk.Label(dialog, text=_JUMP_TO_HELP) \
        .grid(column=0, row=0, columnspan=2, sticky="w", padx=".1c",
              pady=".1c")

    entry = ttk.Entry(dialog)
    entry.grid(column=0, row=1, sticky="ew", padx=".1c", pady=".1c")
    entry.focus_set()

    entry.bind("<Return>", search)
    entry.bind("<KP_Enter>", search)

    ttk.Button(dialog, text="Search", command=search) \
        .grid(column=1, row=1, padx="0 .1c", pady="0 .1c")

    msglabel = ttk.Label(dialog)
    msglabel.grid(column=0, row=2, sticky="w", pady="0 .1c")

    panedwindow, tree = _create_kconfig_tree_and_desc(dialog)
    panedwindow.grid(column=0, row=3, columnspan=2, sticky="nsew")

    # Clear tree
    tree.set_children("")

    _jump_to_tree = tree

    jumpto_button = ttk.Button(dialog, text="Jump to selected item",
                               state="disabled", command=jump_to_selected)
    jumpto_button.grid(column=0, row=4, columnspan=2, sticky="ns", pady=".1c")

    dialog.columnconfigure(0, weight=1)
    # Only the pane with the Kconfig tree and description grows vertically
    dialog.rowconfigure(3, weight=1)

    # See the menuconfig() function
    _root.update_idletasks()
    dialog.geometry(dialog.geometry())

    # The dialog must be visible before we can grab the input
    dialog.wait_visibility()
    dialog.grab_set()

    tree.bind("<Double-1>", jump_to_selected)
    tree.bind("<Return>", jump_to_selected)
    tree.bind("<KP_Enter>", jump_to_selected)
    # add=True to avoid overriding the description text update
    tree.bind("<<TreeviewSelect>>", tree_select, add=True)

    dialog.bind("<Escape>", lambda _: dialog.destroy())

    # Wait for the user to be done with the dialog
    _root.wait_window(dialog)

    _jump_to_tree = None

    _tree.focus_set()


def _update_jump_to_matches(msglabel, search_string):
    # Searches for nodes matching the search string and updates
    # _jump_to_matches. Puts a message in 'msglabel' if there are no matches,
    # or regex errors.

    global _jump_to_matches

    _jump_to_tree.selection_set(())

    try:
        # We could use re.IGNORECASE here instead of lower(), but this is
        # faster for regexes like '.*debug$' (though the '.*' is redundant
        # there). Those probably have bad interactions with re.search(), which
        # matches anywhere in the string.
        regex_searches = [re.compile(regex).search
                          for regex in search_string.lower().split()]
    except re.error as e:
        msg = "Bad regular expression"
        # re.error.msg was added in Python 3.5
        if hasattr(e, "msg"):
            msg += ": " + e.msg
        msglabel["text"] = msg
        # Clear tree
        _jump_to_tree.set_children("")
        return

    _jump_to_matches = []
    add_match = _jump_to_matches.append

    for node in _sorted_sc_nodes():
        # Symbol/choice
        sc = node.item

        for search in regex_searches:
            # Both the name and the prompt might be missing, since
            # we're searching both symbols and choices

            # Does the regex match either the symbol name or the
            # prompt (if any)?
            if not (sc.name and search(sc.name.lower()) or
                    node.prompt and search(node.prompt[0].lower())):

                # Give up on the first regex that doesn't match, to
                # speed things up a bit when multiple regexes are
                # entered
                break

        else:
            add_match(node)

    # Search menus and comments

    for node in _sorted_menu_comment_nodes():
        for search in regex_searches:
            if not search(node.prompt[0].lower()):
                break
        else:
            add_match(node)

    msglabel["text"] = "" if _jump_to_matches else "No matches"

    _update_jump_to_display()

    if _jump_to_matches:
        item = id(_jump_to_matches[0])
        _jump_to_tree.selection_set(item)
        _jump_to_tree.focus(item)


def _update_jump_to_display():
    # Updates the images and text for the items in _jump_to_matches, and sets
    # them as the items of _jump_to_tree

    # Micro-optimize a bit
    item = _jump_to_tree.item
    id_ = id
    node_str = _node_str
    img_tag = _img_tag
    visible = _visible
    for node in _jump_to_matches:
        item(id_(node),
             text=node_str(node),
             tags=img_tag(node) if visible(node) else
                 img_tag(node) + " invisible")

    _jump_to_tree.set_children("", *map(id, _jump_to_matches))


def _jump_to(node):
    # Jumps directly to 'node' and selects it

    if _single_menu:
        _enter_menu(_parent_menu(node))
    else:
        _load_parents(node)

    _select(_tree, id(node))


# Obscure Python: We never pass a value for cached_nodes, and it keeps pointing
# to the same list. This avoids a global.
def _sorted_sc_nodes(cached_nodes=[]):
    # Returns a sorted list of symbol and choice nodes to search. The symbol
    # nodes appear first, sorted by name, and then the choice nodes, sorted by
    # prompt and (secondarily) name.

    if not cached_nodes:
        # Add symbol nodes
        for sym in sorted(_kconf.unique_defined_syms,
                          key=lambda sym: sym.name):
            # += is in-place for lists
            cached_nodes += sym.nodes

        # Add choice nodes

        choices = sorted(_kconf.unique_choices,
                         key=lambda choice: choice.name or "")

        cached_nodes += sorted(
            [node for choice in choices for node in choice.nodes],
            key=lambda node: node.prompt[0] if node.prompt else "")

    return cached_nodes


def _sorted_menu_comment_nodes(cached_nodes=[]):
    # Returns a list of menu and comment nodes to search, sorted by prompt,
    # with the menus first

    if not cached_nodes:
        def prompt_text(mc):
            return mc.prompt[0]

        cached_nodes += sorted(_kconf.menus, key=prompt_text)
        cached_nodes += sorted(_kconf.comments, key=prompt_text)

    return cached_nodes


def _load_parents(node):
    # Menus are lazily populated as they're opened in full-tree mode, but
    # jumping to an item needs its parent menus to be populated. This function
    # populates 'node's parents.

    # Get all parents leading up to 'node', sorted with the root first
    parents = []
    cur = node.parent
    while cur is not _kconf.top_node:
        parents.append(cur)
        cur = cur.parent
    parents.reverse()

    for i, parent in enumerate(parents):
        if not _tree.item(id(parent), "open"):
            # Found a closed menu. Populate it and all the remaining menus
            # leading up to 'node'.
            for parent in parents[i:]:
                # We only need to populate "real" menus/choices. Implicit menus
                # are populated when their parents menus are entered.
                if not isinstance(parent.item, Symbol):
                    _build_full_tree(parent)
            return


def _parent_menu(node):
    # Returns the menu node of the menu that contains 'node'. In addition to
    # proper 'menu's, this might also be a 'menuconfig' symbol or a 'choice'.
    # "Menu" here means a menu in the interface.

    menu = node.parent
    while not menu.is_menuconfig:
        menu = menu.parent
    return menu


def _trace_write(var, fn):
    # Makes fn() be called whenever the Tkinter Variable 'var' changes value

    # trace_variable() is deprecated according to the docstring,
    # which recommends trace_add()
    if hasattr(var, "trace_add"):
        var.trace_add("write", fn)
    else:
        var.trace_variable("w", fn)


def _info_str(node):
    # Returns information about the menu node 'node' as a string.
    #
    # The helper functions are responsible for adding newlines. This allows
    # them to return "" if they don't want to add any output.

    if isinstance(node.item, Symbol):
        sym = node.item

        return (
            _name_info(sym) +
            _help_info(sym) +
            _direct_dep_info(sym) +
            _defaults_info(sym) +
            _select_imply_info(sym) +
            _kconfig_def_info(sym)
        )

    if isinstance(node.item, Choice):
        choice = node.item

        return (
            _name_info(choice) +
            _help_info(choice) +
            'Mode: {}\n\n'.format(choice.str_value) +
            _choice_syms_info(choice) +
            _direct_dep_info(choice) +
            _defaults_info(choice) +
            _kconfig_def_info(choice)
        )

    # node.item in (MENU, COMMENT)
    return _kconfig_def_info(node)


def _name_info(sc):
    # Returns a string with the name of the symbol/choice. Choices are shown as
    # <choice (name if any)>.

    return (sc.name if sc.name else standard_sc_expr_str(sc)) + "\n\n"


def _value_info(sym):
    # Returns a string showing 'sym's value

    # Only put quotes around the value for string symbols
    return "Value: {}\n".format(
        '"{}"'.format(sym.str_value)
        if sym.orig_type == STRING
        else sym.str_value)


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
            s += node.help + "\n\n"

    return s


def _direct_dep_info(sc):
    # Returns a string describing the direct dependencies of 'sc' (Symbol or
    # Choice). The direct dependencies are the OR of the dependencies from each
    # definition location. The dependencies at each definition location come
    # from 'depends on' and dependencies inherited from parent items.

    return "" if sc.direct_dep is _kconf.y else \
        'Direct dependencies (={}):\n{}\n' \
        .format(TRI_TO_STR[expr_value(sc.direct_dep)],
                _split_expr_info(sc.direct_dep, 2))


def _defaults_info(sc):
    # Returns a string describing the defaults of 'sc' (Symbol or Choice)

    if not sc.defaults:
        return ""

    s = "Default"
    if len(sc.defaults) > 1:
        s += "s"
    s += ":\n"

    for val, cond in sc.orig_defaults:
        s += "  - "
        if isinstance(sc, Symbol):
            s += _expr_str(val)

            # Skip the tristate value hint if the expression is just a single
            # symbol. _expr_str() already shows its value as a string.
            #
            # This also avoids showing the tristate value for string/int/hex
            # defaults, which wouldn't make any sense.
            if isinstance(val, tuple):
                s += '  (={})'.format(TRI_TO_STR[expr_value(val)])
        else:
            # Don't print the value next to the symbol name for choice
            # defaults, as it looks a bit confusing
            s += val.name
        s += "\n"

        if cond is not _kconf.y:
            s += "    Condition (={}):\n{}" \
                 .format(TRI_TO_STR[expr_value(cond)],
                         _split_expr_info(cond, 4))

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
        s += "{}{} {}".format(indent*" ",
                              "  " if i == 0 else op_str,
                              _expr_str(term))

        # Don't bother showing the value hint if the expression is just a
        # single symbol. _expr_str() already shows its value.
        if isinstance(term, tuple):
            s += "  (={})".format(TRI_TO_STR[expr_value(term)])

        s += "\n"

    return s


def _select_imply_info(sym):
    # Returns a string with information about which symbols 'select' or 'imply'
    # 'sym'. The selecting/implying symbols are grouped according to which
    # value they select/imply 'sym' to (n/m/y).

    def sis(expr, val, title):
        # sis = selects/implies
        sis = [si for si in split_expr(expr, OR) if expr_value(si) == val]
        if not sis:
            return ""

        res = title
        for si in sis:
            res += "  - {}\n".format(split_expr(si, AND)[0].name)
        return res + "\n"

    s = ""

    if sym.rev_dep is not _kconf.n:
        s += sis(sym.rev_dep, 2,
                 "Symbols currently y-selecting this symbol:\n")
        s += sis(sym.rev_dep, 1,
                 "Symbols currently m-selecting this symbol:\n")
        s += sis(sym.rev_dep, 0,
                 "Symbols currently n-selecting this symbol (no effect):\n")

    if sym.weak_rev_dep is not _kconf.n:
        s += sis(sym.weak_rev_dep, 2,
                 "Symbols currently y-implying this symbol:\n")
        s += sis(sym.weak_rev_dep, 1,
                 "Symbols currently m-implying this symbol:\n")
        s += sis(sym.weak_rev_dep, 0,
                 "Symbols currently n-implying this symbol (no effect):\n")

    return s


def _kconfig_def_info(item):
    # Returns a string with the definition of 'item' in Kconfig syntax,
    # together with the definition location(s) and their include and menu paths

    nodes = [item] if isinstance(item, MenuNode) else item.nodes

    s = "Kconfig definition{}, with parent deps. propagated to 'depends on'\n" \
        .format("s" if len(nodes) > 1 else "")
    s += (len(s) - 1)*"="

    for node in nodes:
        s += "\n\n" \
             "At {}:{}\n" \
             "{}" \
             "Menu path: {}\n\n" \
             "{}" \
             .format(node.filename, node.linenr,
                     _include_path_info(node),
                     _menu_path_info(node),
                     node.custom_str(_name_and_val_str))

    return s


def _include_path_info(node):
    if not node.include_path:
        # In the top-level Kconfig file
        return ""

    return "Included via {}\n".format(
        " -> ".join("{}:{}".format(filename, linenr)
                    for filename, linenr in node.include_path))


def _menu_path_info(node):
    # Returns a string describing the menu path leading up to 'node'

    path = ""

    while node.parent is not _kconf.top_node:
        node = node.parent

        # Promptless choices might appear among the parents. Use
        # standard_sc_expr_str() for them, so that they show up as
        # '<choice (name if any)>'.
        path = " -> " + (node.prompt[0] if node.prompt else
                         standard_sc_expr_str(node.item)) + path

    return "(Top)" + path


def _name_and_val_str(sc):
    # Custom symbol/choice printer that shows symbol values after symbols

    # Show the values of non-constant (non-quoted) symbols that don't look like
    # numbers. Things like 123 are actually symbol references, and only work as
    # expected due to undefined symbols getting their name as their value.
    # Showing the symbol value for those isn't helpful though.
    if isinstance(sc, Symbol) and not sc.is_constant and not _is_num(sc.name):
        if not sc.nodes:
            # Undefined symbol reference
            return "{}(undefined/n)".format(sc.name)

        return '{}(={})'.format(sc.name, sc.str_value)

    # For other items, use the standard format
    return standard_sc_expr_str(sc)


def _expr_str(expr):
    # Custom expression printer that shows symbol values
    return expr_str(expr, _name_and_val_str)


def _is_num(name):
    # Heuristic to see if a symbol name looks like a number, for nicer output
    # when printing expressions. Things like 16 are actually symbol names, only
    # they get their name as their value when the symbol is undefined.

    try:
        int(name)
    except ValueError:
        if not name.startswith(("0x", "0X")):
            return False

        try:
            int(name, 16)
        except ValueError:
            return False

    return True


if __name__ == "__main__":
    _main()

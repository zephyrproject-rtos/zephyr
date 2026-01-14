# -*- coding: utf-8 -*-

from pygments.style import Style
from pygments.token import Name, Comment, String, Number, Operator, Whitespace

class CMakeTemplateStyle(Style):
    """
        for more token names, see pygments/styles.default
    """

    background_color = "#f8f8f8"
    default_style = ""

    styles = {
        Whitespace:                "#bbbbbb",
        Comment:                   "italic #408080",
        Operator:                  "#555555",
        String:                    "#217A21",
        Number:                    "#105030",
        Name.Builtin:              "#333333",          # anything lowercase
        Name.Function:             "#007020",          # function
        Name.Variable:             "#1080B0",          # <..>
        Name.Tag:                  "#bb60d5",          # ${..}
        Name.Constant:             "#4070a0",          # uppercase only
        Name.Entity:               "italic #70A020",   # @..@
        Name.Attribute:            "#906060",          # paths, URLs
        Name.Label:                "#A0A000",          # anything left over
        Name.Exception:            "bold #FF0000",     # for debugging only
    }

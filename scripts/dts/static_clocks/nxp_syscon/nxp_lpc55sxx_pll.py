# Copyright (c) 2026 Analog Devices
# SPDX-License-Identifier: Apache-2.0

import math

from common import ClockDriver


class NxpLPC55SxxPLL0(ClockDriver):
    """
    Clock driver for NXP LPC55Sxx PLL0 clock.
    """

    @classmethod
    def get_dt_compat(cls):
        return "nxp,lpc55sxx-pll0"

    def __init__(self, dt_node):
        super().__init__(dt_node)
        self.parent = dt_node.props['input'].val

    def get_parents(self):
        return [self.parent]

    def default_specifiers(self):
        return {
            'ndec': 0,
            'mdec': 0,
            'selr': 0,
            'seli': 0,
            'selp': 0,
            'sscg_en': 0,
            'sscg0': 0,
            'sscg1': 0,
        }

    def configure_recalc(self, specifiers: dict[str, int], parent_rate: int):
        prediv = specifiers['ndec']
        if specifiers['mdec'] != 0:
            fbdiv = specifiers['mdec']
        elif specifiers['sscg_en']:
            fbdiv = (specifiers['sscg0'] + (specifiers['sscg1'] << 32)) / (1 << 25)
        else:
            return -1

        # Validate seli, selr, selp per reference manual formulas
        if fbdiv >= 8000:
            seli = 1
        elif fbdiv >= 122:
            seli = math.floor(8000 / fbdiv)
        else:
            seli = 2 * math.floor(fbdiv / 4) + 3

        selp = min(math.floor(fbdiv / 4) + 1, 31)
        selr = 0

        if specifiers['seli'] != seli or specifiers['selp'] != selp or specifiers['selr'] != selr:
            return -1

        return math.floor(parent_rate * fbdiv / prediv)


class NxpLPC55SxxPLL1(ClockDriver):
    """
    Clock driver for NXP LPC55Sxx PLL1 clock.
    PLL1 is a fractional PLL with output: (parent_rate * mdec) / ndec.
    Bandwidth select values (selr, seli, selp) are validated against
    the same formulas as PLL0.
    """

    @classmethod
    def get_dt_compat(cls):
        return "nxp,lpc55sxx-pll1"

    def __init__(self, dt_node):
        super().__init__(dt_node)
        self.parent = dt_node.props['input'].val

    def get_parents(self):
        return [self.parent]

    def default_specifiers(self):
        return {'ndec': 0, 'mdec': 0, 'selr': 0, 'seli': 0, 'selp': 0}

    def configure_recalc(self, specifiers: dict[str, int], parent_rate: int):
        ndec = specifiers['ndec']
        mdec = specifiers['mdec']

        if ndec == 0:
            return -1

        # Validate bandwidth select values per reference manual
        if mdec >= 8000:
            seli = 1
        elif mdec >= 122:
            seli = math.floor(8000 / mdec)
        else:
            seli = 2 * math.floor(mdec / 4) + 3

        selp = min(math.floor(mdec / 4) + 1, 31)
        selr = 0

        if specifiers['seli'] != seli or specifiers['selp'] != selp or specifiers['selr'] != selr:
            return -1

        return (parent_rate * mdec) // ndec


class NxpLPC55SxxPllPdec(ClockDriver):
    """
    Clock driver for nxp,lpc55sxx-pll-pdec (PLL post divider).
    The specifier 'pdec' sets the division factor applied to the parent
    PLL output. Only even division factors are accepted (2, 4, 6, ...),
    since the hardware PDIV field stores pdec/2.
    Output: parent_rate / pdec.
    """

    @classmethod
    def get_dt_compat(cls):
        return "nxp,lpc55sxx-pll-pdec"

    def __init__(self, dt_node):
        super().__init__(dt_node)
        mask_width = dt_node.props['reg'].val[1]
        # Max PDIV register value
        self.max_pdiv = (1 << mask_width) - 1
        # Max division factor is max_pdiv * 2
        self.max_div = self.max_pdiv * 2
        self.parent = dt_node.props['input'].val

    def get_parents(self):
        return [self.parent]

    def default_specifiers(self):
        # Assumption: PDIV register defaults to 0 at reset, meaning
        # the post divider is disconnected (no output).
        return {'pdec': 0}

    def configure_recalc(self, specifiers: dict[str, int], parent_rate: int):
        pdec = specifiers['pdec']
        if pdec == 0:
            return -1
        if pdec % 2 != 0:
            return -1
        if pdec > self.max_div:
            return -1
        return parent_rate // pdec

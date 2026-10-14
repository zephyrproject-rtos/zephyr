# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for flashing an application booted by IronSide TE.'''

import subprocess

from runners.nrf_common import ErrNotAvailableBecauseProtection, ErrVerify
from runners.nrfutil import NrfUtilBinaryRunner

# Devicetree labels of the partitions the application owns. Everything else in
# an IronSide TE layout - the roots of trust, their secure storage, the DFU
# agent, the write-once UICR configuration, the KMU - belongs to IronSide TE.
APP_PARTITIONS = ('slot0_partition', 'dfubank_partition', 'storage_partition')

REINSTALL = 'ironsdk/scripts/ironside_install.py <bundle> --force'

RECOVER_REFUSED = f'''--recover is not available on a target running IronSide TE.

It is an unauthenticated mass erase and unlock, with no version of it scoped to
the application: an unprotected device would lose IronSide TE along with
everything else, and a protected device refuses it outright.

Reinstalling IronSide TE is what brings a device back:

  {REINSTALL}'''

ERASE_KEPT = f'''
IronSide TE is left intact, unlike normal --erase: the roots of trust, their
secure storage, the DFU agent, the write-once UICR configuration and the KMU,
the persistent keys, the OTA key-encryption keys and the rollback counters
all survive. An image below the resident security counter is therefore still
refused after this erase. To clear that too, reinstall IronSide TE:

  {REINSTALL}'''

PROTECTED = f'''Flashing failed because the target is protected.

A protected device refuses erases that are not authenticated. A reinstall
issues an authenticated one before it programs anything, and needs the keyset
the device was provisioned with, named by --keyset:

  {REINSTALL} --keyset <keyset>'''

UICR_IN_IMAGE = '''The image has data in the UICR region and the verify failed,
which usually means the UICR was not erased first. Here it cannot be: the UICR
configuration belongs to IronSide TE.'''


class IronSideTEBinaryRunner(NrfUtilBinaryRunner):
    '''Runner front-end for nrfutil on a target booted by IronSide TE.

    Flashing is unchanged from the nrfutil runner. What differs is the reach of
    the two options that erase beyond the image: a chip erase and a recover
    would both take IronSide TE and the write-once UICR configuration with
    them, leaving a device that only a reinstall brings back.
    '''

    @classmethod
    def name(cls):
        return 'ironside_te'

    @classmethod
    def do_create(cls, cfg, args):
        return cls(cfg, args.nrf_family, args.softreset, args.pinreset,
                   args.dev_id, erase=args.erase, erase_mode=args.erase_mode,
                   ext_erase_mode=args.ext_erase_mode, reset=args.reset,
                   tool_opt=args.tool_opt, force=args.force,
                   recover=args.recover,
                   ext_mem_config_file=args.ext_mem_config_file,
                   dry_run=args.dry_run)

    def do_run(self, command, **kwargs):
        # Refused here rather than in recover_target(), so that it fails before
        # the base class has gone looking for a board.
        if self.recover:
            raise RuntimeError(RECOVER_REFUSED)

        super().do_run(command, **kwargs)

    def flush(self, force=False):
        # The base class explains both of these failures by pointing at
        # --recover, the one operation this runner does not have.
        try:
            self.flush_ops(force=force)
        except subprocess.CalledProcessError as cpe:
            if cpe.returncode == ErrNotAvailableBecauseProtection:
                self.logger.error(PROTECTED)
            elif cpe.returncode == ErrVerify and self.hex_get_uicrs():
                self.logger.warning(UICR_IN_IMAGE)
            raise

    def op_program(self, hex_file, erase, ext_mem_erase, defer=False, core=None):
        # Both --erase and --erase-mode=all arrive here as a chip erase, which
        # would take the roots of trust and the UICR page with it.
        if erase == 'ERASE_ALL':
            self._erase_app_partitions()
            erase = 'ERASE_NONE'

        super().op_program(hex_file, erase, ext_mem_erase, defer=defer, core=core)

    def _append_batch(self, op, json_file):
        operation = op['operation']

        # The base class spells an erase as a bare --<kind> flag, which leaves
        # nowhere to put a page range.
        if operation['type'] == 'erase' and operation.get('kind') == 'pages':
            self._exec(['erase', '--pages', operation['pages'],
                        '--x-family', f'{self.family}',
                        '--x-append-batch', f'{json_file}'], force=True)
            return

        super()._append_batch(op, json_file)

    def _erase_app_partitions(self):
        partitions = self._app_partitions()
        ranges = self._erase_ranges(partitions)

        report = ["Erasing the application's partitions only:"]
        report += [f'  {label:<19}0x{addr:08x}  {size // 1024:>4} kB'
                   for label, addr, size in partitions]
        report.append(ERASE_KEPT)
        self.logger.info('\n'.join(report))

        for start, end in ranges:
            # nrfutil is given the last address to erase, which it documents
            # both as excluded and as naming the page to stop at. A byte inside
            # the range satisfies both.
            self.exec_op('erase', defer=True, kind='pages',
                         pages=f'0x{start:08x}-0x{end - 1:08x}')

    def _app_partitions(self):
        '''Return the application's partitions as (label, address, size),
        lowest address first.'''
        edt = self.build_conf.edt

        partitions = []
        for label in APP_PARTITIONS:
            node = edt.label2node.get(label)
            if node is not None:
                partitions.append((label, node.regs[0].addr, node.regs[0].size))

        if not partitions:
            raise RuntimeError(
                f'This build has none of {", ".join(APP_PARTITIONS)} in its '
                'devicetree, so there is no way to tell which partitions the '
                'application owns. Is this an IronSide TE board target?')

        partitions.sort(key=lambda partition: partition[1])

        return partitions

    def _erase_ranges(self, partitions):
        '''Return the (start, end) spans to erase, end excluded. Partitions
        that touch are coalesced, so a contiguous layout takes a single erase
        and a gap between two partitions is never erased.'''
        ranges = []
        for _, addr, size in partitions:
            if ranges and ranges[-1][1] == addr:
                ranges[-1][1] = addr + size
            else:
                ranges.append([addr, addr + size])

        ranges = [tuple(span) for span in ranges]
        self._check_page_aligned(ranges)

        return ranges

    def _check_page_aligned(self, ranges):
        # An erase reaches whole pages, so both edges of every span have to sit
        # on a page boundary to stay within the partitions it covers. Boundaries
        # between coalesced partitions do not matter; the span's own edges do,
        # IronSide TE being on the other side.
        page_size = self._page_size()

        for start, end in ranges:
            if start % page_size or end % page_size:
                raise RuntimeError(
                    "An erase of the application's partitions would run "
                    f'0x{start:08x}-0x{end:08x}, which the {page_size} byte '
                    'erase page does not divide, so it would reach into '
                    'IronSide TE. Compare the devicetree layout against '
                    'ironsdk/scripts/read_partitions.py.')

    def _page_size(self):
        flash = self.build_conf.edt.chosen_node('zephyr,flash')
        page = flash.props.get('erase-block-size') if flash else None
        page_size = page.val if page else 0
        if not page_size:
            raise RuntimeError(
                'The devicetree does not say how large an erase page is, so '
                "the erase cannot be held inside the application's partitions.")

        return page_size

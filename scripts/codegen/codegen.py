# Copyright 2004-2016, Ned Batchelder.
#           http://nedbatchelder.com/code/cog
# Copyright (c) 2018 Bobby Noelte.
#
# SPDX-License-Identifier: MIT

import sys
import os
import imp
import inspect
import re
from traceback import TracebackException

from .whiteutils import *
from .filereader import NumberedFileReader
from .options import Options, OptionsMixin
from .generic import GenericMixin
from .guard import GuardMixin
from .config import ConfigMixin
from .cmake import CMakeMixin
from .zephyr import ZephyrMixin
from .edts import EDTSMixin
from .include import IncludeMixin
from .log import LogMixin
from .error import ErrorMixin, Error
from .output import OutputMixin
from .importmodule import ImportMixin
from .redirectable import Redirectable, RedirectableMixin

class CodeGenerator(OptionsMixin, GenericMixin, ConfigMixin,
                    CMakeMixin, ZephyrMixin, EDTSMixin, GuardMixin,
                    IncludeMixin, LogMixin, ErrorMixin, OutputMixin,
                    ImportMixin, RedirectableMixin):

    code_start_marker = b'@code{.codegen}'
    code_end_marker = b'@endcode{.codegen}'
    code_insert_marker = b'@code{.codeins}@endcode'

    def __init__(self, processor, globals={},
                 output_file=None, snippet_file = None):
        self._stdout = sys.stdout
        self._stderr = sys.stderr
        self._outstring = ''
        # code snippet markers and lines
        self._start_marker = self.code_start_marker.decode('utf-8')
        self._end_marker = self.code_end_marker.decode('utf-8')
        self._insert_marker = self.code_insert_marker.decode('utf-8')
        self.markers = []
        self.lines = []
        # The processor that is using this generator
        self.processor = processor
        self.options = processor.options
        # All generators of a file usually work on the same global namespace
        self.generator_globals = globals
        self._output_file = output_file
        # The file that contains the snippet
        self._snippet_file = snippet_file
        # The snippet this generator works on
        self._snippet = None
        # the snippet start offset in original file
        self._snippet_offset = None
        # the tab size in the snippet
        self._snippet_tabsize = 8
        # the current evaluation start offset in the original file
        # may be different to snippet offset during evaluation
        self._eval_offset = None
        # the previous output
        self._previous = ''

    def line_is_start_marker(self, s):
        return self._start_marker in s

    def line_is_end_marker(self, s):
        return self._end_marker in s and not self.line_is_insert_marker(s)

    def line_is_insert_marker(self, s):
        return self._insert_marker in s

    def parse_start_marker(self, l, snippet_offset):
        self._snippet_offset = snippet_offset
        self._eval_offset = snippet_offset
        self.markers.append(l.expandtabs(self._snippet_tabsize))

    def parse_end_marker(self, l):
        self.markers.append(l.expandtabs(self._snippet_tabsize))

    def parse_line(self, l, single_line_snippet=False):
        l = l.expandtabs(self._snippet_tabsize).strip('\n')
        if single_line_snippet:
            # single line snippets contain the markers - remove them
            beg = l.find(self._start_marker)
            end = l.find(self._end_marker)
            if beg > end:
                self.lines.append(l)
                self.error("Codegen code markers inverted",
                           frame_index = -2)
            else:
                l = l[beg+len(self._start_marker):end].strip()
        self.lines.append(l)

    def _out(self, output='', dedent=False, trimblanklines=False):
        if trimblanklines and ('\n' in output):
            lines = output.split('\n')
            if lines[0].strip() == '':
                del lines[0]
            if lines and lines[-1].strip() == '':
                del lines[-1]
            output = '\n'.join(lines)+'\n'
        if dedent:
            output = reindentBlock(output)
        self._outstring += output

    ##
    # @brief Re-indent a code block.
    #
    # Take a block of text as a string or list of lines.
    # Remove any common prefix and whitespace indentation.
    # Re-indent using new_indent
    #
    # @param lines
    # @param common_prefix
    # @param new_indent
    # @return indented code block as a single string.
    def _reindent_code(self, lines, common_prefix = '', new_indent=''):
        if not isinstance(lines, list):
            lines = lines.split('\n')
        # remove common prefix
        code_lines = []          # all code lines
        code_no_white_lines = []  # code lines excluding white space lines
        for line in lines:
            line = line.expandtabs()
            if common_prefix:
                line = line.replace(common_prefix, '', 1)
            code_lines.append(line)
            if line and not line.isspace():
                code_no_white_lines.append(line)
        # remove common white space and re-indent
        out_lines = []
        common_white = os.path.commonprefix(code_no_white_lines)
        common_white = re.search(r'\s*', common_white).group(0)
        for line in code_lines:
            if common_white:
                line = line.replace(common_white, '', 1)
            if line and new_indent:
                line = new_indent + line
            out_lines.append(line)
        return '\n'.join(out_lines)

    ##
    # @brief Extract the executable Python code from the generator.
    #
    def _get_code(self, fname, snippet_offset):
        # If the markers and lines all have the same prefix
        # (end-of-line comment chars, for example),
        # then remove it from all the lines.
        common_prefix = os.path.commonprefix(self.markers + self.lines)
        if not common_prefix:
            # there may be a prefix error
            # just do some heuristics
            if fname.endswith(
                ('.h', '.hxx', '.c', '.cpp', '.cxx')):
                # assume C/C++ comments
                for line in (self.markers + self.lines):
                    if line.strip().startswith('*'):
                        common_prefix = '*'
                        break
                    elif line.strip().startswith('//'):
                        common_prefix = '//'
                        break
            if common_prefix:
                # there should be a common prefix -> error
                lines = list() # lines with correct prefix
                for lineno, line in enumerate(self.lines):
                    if not line.strip().startswith(common_prefix):
                        print("Codegen: Comment prefix may miss in codegen snippet (+{}) in '{}'.".format(
                            snippet_offset, fname))
                        line_start = lineno - 5
                        if line_start < 0:
                            line_start = 0
                        line_end = lineno + 5
                        if line_end > len(self.lines):
                            line_end = len(self.lines)
                        for i in range(line_start, line_end):
                            snippet_lineno = i + 2
                            input_lineno = snippet_offset + snippet_lineno
                            print("#{} (+{}, line {}): {}".format(
                                input_lineno, snippet_offset, snippet_lineno, self.lines[i]))
                    else:
                        lines.append(line)
                if len(lines) >= int(len(self.lines) / 2):
                    common_prefix = os.path.commonprefix(lines)
                    print("Codegen: Assuming comment prefix '{}' for codegen snippet (+{}) in '{}'.".format(
                          common_prefix, snippet_offset, fname))
                else:
                    common_prefix = ''
        if common_prefix:
            self.markers = [ line.replace(common_prefix, '', 1) for line in self.markers ]
        code = self._reindent_code(self.lines, common_prefix)
        return code

    # Make sure the "codegen" (alias cog) module has our state.
    def _set_module_state(self):
        restore_state = {}
        module_states = self.processor.module_states
        module = self.processor.cogmodule
        # General module values
        restore_state['inFile'] = getattr(module, 'inFile', None)
        module.inFile = self._snippet_file
        restore_state['outFile'] = getattr(module, 'outFile', None)
        module.outFile = self._output_file
        restore_state['firstLineNum'] = getattr(module, 'firstLineNum', None)
        module.firstLineNum = self._snippet_offset
        restore_state['previous'] = getattr(module, 'previous', None)
        module.previous = self._previous
        # CodeGenerator access
        restore_state['options'] = getattr(module, 'options', None)
        module.options = self.options
        restore_state['Error'] = getattr(module, 'Error', None)
        module.Error = self._get_error_exception
        # Look for the Mixin classes
        for base_cls in inspect.getmro(CodeGenerator):
            if "Mixin" in base_cls.__name__:
                for member_name, member_value in inspect.getmembers(base_cls):
                    if member_name.startswith('_'):
                        continue
                    if inspect.isroutine(member_value):
                        restore_state[member_name] = \
                            getattr(module, member_name, None)
                        setattr(module, member_name,
                            getattr(self, member_name))
        module_states.append(restore_state)

    def _restore_module_state(self):
        module_states = self.processor.module_states
        module = self.processor.cogmodule
        restore_state = module_states.pop()
        # General module values
        module.inFile = restore_state['inFile']
        module.outFile = restore_state['outFile']
        module.firstLineNum = restore_state['firstLineNum']
        module.previous = restore_state['previous']
        # CodeGenerator access
        module.options = restore_state['options']
        module.Error = restore_state['Error']
        # Look for the Mixin classes
        for base_cls in inspect.getmro(CodeGenerator):
            if "Mixin" in base_cls.__name__:
                for member_name, member_value in inspect.getmembers(base_cls):
                    if member_name.startswith('_'):
                        continue
                    if inspect.isroutine(member_value):
                        setattr(module, member_name, restore_state[member_name])

    ##
    # @brief snippet id to be used in logging and error reporting
    #
    # Accounts for extra lines added during evaluation
    #
    def _get_snippet_id(self):
        return "+{}".format(self._eval_offset)

    ##
    # @brief get snippet line number from evaluation line number
    #
    # Accounts for extra lines added during evaluation
    #
    # @param eval_lineno line number as reported from python code eval
    def _get_snippet_lineno(self, eval_lineno):
        return int(eval_lineno) + self._eval_offset - self._snippet_offset

    def _list_snippet(self):
        if not self._snippet:
            return None
        listing = ""
        for i, line in enumerate(self._snippet.splitlines()):
            eval_lineno = i + 2
            input_lineno = self._eval_offset + eval_lineno
            if i > 0:
                listing += "\n"
            listing += "#{} ({}, line {}): {}".format(
                input_lineno, self._get_snippet_id(), eval_lineno, line)
        return listing

    def _list_lines(self):
        if len(self.lines) == 0:
            return None
        listing = ""
        for i, line in enumerate(self.lines):
            eval_lineno = i + 2
            input_lineno = self._eval_offset + eval_lineno
            if i > 0:
                listing += "\n"
            listing += "#{} ({}, line {}): {}".format(
                input_lineno, self._get_snippet_id(), eval_lineno, line)
        return listing

    ##
    # @brief evaluate
    #
    def evaluate(self):
        self._snippet = self._get_code(self._snippet_file, self._snippet_offset)
        if not self._snippet:
            return ''

        # we add an extra line 'import codegen'
        # to snippet so account for that
        self._eval_offset = self._snippet_offset - 1

        # In Python 2.2, the last line has to end in a newline.
        eval_code = "import codegen\n" + self._snippet + "\n"
        eval_fname = "{} {}".format(self._snippet_file, self._get_snippet_id())

        try:
            code = compile(eval_code, eval_fname, 'exec')
        except:
            exc_type, exc_value, exc_tb = sys.exc_info()
            exc_traceback = TracebackException(exc_type, exc_value, exc_tb)
            self.error(
                "compile exception '{}' within snippet in {}".format(
                    exc_value, self._snippet_file),
                frame_index = -2,
                snippet_lineno = exc_traceback.lineno)

        # Make sure the "codegen" (alias cog) module has our state.
        self._set_module_state()

        self._outstring = ''
        try:
            eval(code, self.generator_globals)
        except:
            exc_type, exc_value, exc_tb = sys.exc_info()
            if exc_type is Error:
                # Exception raise by CodeGen means
                raise
            # Not raised by Codegen means - add some info
            print("Codegen: eval exception within codegen snippet ({}) in {}".format(
                  self._get_snippet_id(), self._snippet_file))
            for i, line in enumerate(self._snippet.splitlines()):
                eval_lineno = i + 2
                input_lineno = self._eval_offset + eval_lineno
                print("#{} ({}, line {}): {}".format(input_lineno, self._get_snippet_id(), eval_lineno, line))
            raise
        finally:
            self._restore_module_state()

        # We need to make sure that the last line in the output
        # ends with a newline, or it will be joined to the
        # end-output line, ruining cog's idempotency.
        if self._outstring and self._outstring[-1] != '\n':
            self._outstring += '\n'

        # end of evaluation - no extra offset anymore
        self._eval_offset = self._snippet_offset

        # figure out the right whitespace prefix for the output
        prefOut = whitePrefix(self.markers)
        self._previous = reindentBlock(self._outstring, prefOut)
        return self._previous

##
# @brief The code generation processor
#
class CodeGen(Redirectable):

    def __init__(self):
        Redirectable.__init__(self)
        # Stack of module states
        self.module_states = []
        self.options = Options()
        # assure codegen module is installed
        self._install_codegen_module()

    ##
    # @brief Is this a trailing line after an end spec marker.
    #
    # @todo Make trailing end spec line detection dependent on
    #       type of text or file type.
    #
    # @param s line
    #
    def _is_end_spec_trailer(self, s):
        return '*/' in s

    def _install_codegen_module(self):
        """ Magic mumbo-jumbo so that imported Python modules
            can say "import codegen" and get our state.

            Make us the module, and not our parent cog.
        """
        self.cogmodule = imp.new_module('codegen')
        self.cogmodule.path = []
        sys.modules['codegen'] = self.cogmodule

    def openOutputFile(self, fname):
        """ Open an output file, taking all the details into account.
        """
        opts = {}
        mode = "w"
        opts['encoding'] = self.options.sEncoding
        if self.options.bNewlines:
            opts['newline'] = "\n"
        fdir = os.path.dirname(fname)
        if os.path.dirname(fdir) and not os.path.exists(fdir):
            os.makedirs(fdir)
        return open(fname, mode, **opts)

    def openInputFile(self, fname):
        """ Open an input file. """
        if fname == "-":
            return sys.stdin
        else:
            opts = {}
            opts['encoding'] = self.options.sEncoding
            return open(fname, "r", **opts)

    ##
    # @brief Process an input file object to an output file object.
    #
    # May be called recursively
    #
    # @param fIn input file object, or file name
    # @param fOut output file object, or file name
    # @param fname [optional]
    # @param globals [optional]
    #
    def process_file(self, fIn, fOut, fname=None, globals=None):

        fInToClose = fOutToClose = None
        # Convert filenames to files.
        if isinstance(fIn, (str, bytes)):
            # Open the input file.
            sFileIn = fIn
            fIn = fInToClose = self.openInputFile(fIn)
        elif hasattr(fIn, 'name'):
            sFileIn = fIn.name
        else:
            sFileIn = fname or ''
        if isinstance(fOut, (str, bytes)):
            # Open the output file.
            sFileOut = fOut
            fOut = fOutToClose = self.openOutputFile(fOut)
        elif hasattr(fOut, 'name'):
            sFileOut = fOut.name
        else:
            sFileOut = fname or ''

        try:
            fIn = NumberedFileReader(fIn)

            bSawCog = False

            # The globals dict we will use for this file.
            if globals is None:
                globals = {}
                # list of include files that are guarded against inclusion
                globals['_guard_include'] = []

            # If there are any global defines, put them in the globals.
            globals.update(self.options.defines)

            # global flag for code generation
            globals['_generate_code'] = True

            # loop over generator chunks
            l = fIn.readline()
            gen = None
            while l and globals['_generate_code']:

                if gen is None:
                    # have a generator ready
                    # for marker check and error reporting
                    gen = CodeGenerator(self, globals, sFileOut, str(sFileIn))
                    gen.setOutput(stdout=self._stdout)

                # Find the next spec begin
                while l and not gen.line_is_start_marker(l):
                    if gen.line_is_end_marker(l):
                        gen._snippet_offset = fIn.linenumber()
                        gen.error("Unexpected '%s'" % gen._end_marker,
                                  frame_index=-1, snippet_lineno=0)
                    if gen.line_is_insert_marker(l):
                        gen._snippet_offset = fIn.linenumber()
                        gen.error("Unexpected '%s'" % gen._insert_marker,
                                  frame_index=-1, snippet_lineno=0)
                    fOut.write(l)
                    l = fIn.readline()
                if not l:
                    break
                if not self.options.bDeleteCode:
                    fOut.write(l)

                # l is the begin spec
                firstLineNum = fIn.linenumber()
                # Start parsing the inline code spec
                # Assure a new generator is in use
                gen = CodeGenerator(self, globals, sFileOut, str(sFileIn))
                gen.setOutput(stdout=self._stdout)
                gen.parse_start_marker(l, firstLineNum)

                gen.log('s{}: process {} #{}'.format(len(self.module_states), sFileIn, firstLineNum))
                # If the spec begin is also a spec end, then process the single
                # line of code inside.
                if gen.line_is_end_marker(l):
                    gen.parse_line(l, True)
                    # next line
                    l = fIn.readline()
                else:
                    # Deal with an ordinary code block.
                    l = fIn.readline()

                    # Get all the lines in the spec
                    while l and not gen.line_is_end_marker(l):
                        gen.parse_line(l)
                        if gen.line_is_start_marker(l):
                            gen.error("Code followed by unexpected '%s'" % gen._start_marker,
                                      frame_index = -2,
                                      snippet_lineno = fIn.linenumber() - firstLineNum)
                        if gen.line_is_insert_marker(l):
                            gen.error("Code followed by unexpected '%s'" % gen._insert_marker,
                                      frame_index = -2,
                                      snippet_lineno = fIn.linenumber() - firstLineNum)
                        if not self.options.bDeleteCode:
                            fOut.write(l)
                        l = fIn.readline()
                    if not l:
                        gen.error("Codegen block begun but never ended.",
                                  frame_index = -2, snippet_lineno = 0)
                    # write out end spec line
                    if not self.options.bDeleteCode:
                        fOut.write(l)
                    gen.parse_end_marker(l)
                    # next line - may be trailing end spec line
                    l = fIn.readline()
                    if self._is_end_spec_trailer(l) and not gen.line_is_insert_marker(l):
                        fOut.write(l)
                        l = fIn.readline()

                # Eat all the lines in the output section.
                while l and not gen.line_is_insert_marker(l):
                    if gen.line_is_start_marker(l):
                        gen.error("Unexpected '%s'" % gen._start_marker,
                                  frame_index = -2,
                                  snippet_lineno = fIn.linenumber() - firstLineNum)
                    if gen.line_is_end_marker(l):
                        gen.error("Unexpected '%s'" % gen._end_marker,
                                  frame_index = -2,
                                  snippet_lineno = fIn.linenumber() - firstLineNum)
                    l = fIn.readline()

                if not l:
                    # We reached end of file before we found the end output line.
                    gen.error("Missing '%s' before end of file." % gen._insert_marker,
                              frame_index = -2,
                              snippet_lineno = fIn.linenumber() - firstLineNum)

                # Write the output of the spec to be the new output if we're
                # supposed to generate code.
                if not self.options.bNoGenerate:
                    sGen = gen.evaluate()
                    fOut.write(sGen)
                    if not globals['_generate_code']:
                        # generator code snippet stopped code generation
                        break

                bSawCog = True

                if not self.options.bDeleteCode:
                    fOut.write(l)
                l = fIn.readline()

            if not bSawCog and self.options.bWarnEmpty:
                self.warning("no codegen code found in %s" % sFileIn)
        finally:
            if fInToClose:
                fInToClose.close()
            if fOutToClose:
                fOutToClose.close()

    def saveIncludePath(self):
        self.savedInclude = self.options.includePath[:]
        self.savedSysPath = sys.path[:]

    def restoreIncludePath(self):
        self.options.includePath = self.savedInclude
        self.cogmodule.path = self.options.includePath
        sys.path = self.savedSysPath

    def addToIncludePath(self, includePath):
        self.cogmodule.path.extend(includePath)
        sys.path.extend(includePath)

    ##
    # @brief process one file through CodeGen
    #
    # @param sFile file name
    #
    def _process_one_file(self, sFile):
        """ Process one filename through cog.
        """

        self.saveIncludePath()
        bNeedNewline = False

        try:
            self.addToIncludePath(self.options.includePath)
            # Since we know where the input file came from,
            # push its directory onto the include path.
            self.addToIncludePath([os.path.dirname(sFile)])

            # How we process the file depends on where the output is going.
            if self.options.sOutputName:
                self.process_file(sFile, self.options.sOutputName, sFile)
            else:
                self.process_file(sFile, self.stdout, sFile)
        finally:
            self.restoreIncludePath()

    def callableMain(self, argv):
        """ All of command-line codegen, but in a callable form.
            This is used by main.
            argv is the equivalent of sys.argv.
        """
        argv = argv[1:]

        self.options.parse_args(argv)

        if self.options.input_file is None:
            raise FileNotFoundError("No files to process")

        self._process_one_file(self.options.input_file)

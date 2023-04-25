.. _coding_guidelines:

Coding Guidelines
#################

The project TSC and the Safety Committee of the project agreed to implement
a staged and incremental approach for complying with a set of coding rules (AKA
Coding Guidelines) to improve quality and consistency of the code base. Below
are the agreed upon stages and the approximate timelines:

Stage I
  Coding guideline rules are available to be followed and referenced,
  but not enforced. Rules are not yet enforced in CI and pull-requests cannot be
  blocked by reviewers/approvers due to violations.

Stage II
  Begin enforcement on a limited scope of the code base. Initially, this would be
  the safety certification scope. For rules easily applied across codebase, we
  should not limit compliance to initial scope. This step requires tooling,
  CI setup and an enforcement strategy.

Stage III
  Revisit the coding guideline rules and based on experience from previous
  stages, refine/iterate on selected rules.

Stage IV
   Expand enforcement to the wider codebase. Exceptions may be granted on some
   areas of the codebase with a proper justification. Exception would require
   TSC approval.

.. note::

    Coding guideline rules may be removed/changed at any time by filing a
    GH issue/RFC.

Main rules
**********

The coding guideline rules are based on MISRA-C 2012 and are a subset of MISRA-C.
The subset is listed in the table below with a summary of the rules, its
severity and the equivalent rules from other standards for reference.

.. note::

    For existing Zephyr maintainers and collaborators, if you are unable to
    obtain a copy through your employer, a limited number of copies will be made
    available through the project. If you need a copy of MISRA-C 2012, please
    send email to safety@lists.zephyrproject.org and provide details on reason
    why you can't obtain one through other options and expected contributions
    once you have one.  The safety committee will review all requests.


.. list-table:: Main rules
    :header-rows: 1
    :widths: 17 14 43 12 14

    * -  MISRA C 2012
      -  Severity
      -  Description
      -  CERT C
      -  Example

         .. _MisraC_Dir_1_1:
    * -  Dir 1.1
      -  Required
      -  Any implementation-defined behaviour on which the output of the program depends shall be documented and understood
      -  `MSC09-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC09-C.+Character+encoding%3A+Use+subset+of+ASCII+for+safety>`_
      -  `Dir 1.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_01_01.c>`_

         .. _MisraC_Dir_2_1:
    * -  Dir 2.1
      -  Required
      -  All source files shall compile without any compilation errors
      -  N/A
      -  `Dir 2.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_02_01.c>`_

         .. _MisraC_Dir_3_1:
    * -  Dir 3.1
      -  Required
      -  All code shall be traceable to documented requirements
      -  N/A
      -  `Dir 3.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_03_01.c>`_

         .. _MisraC_Dir_4_1:
    * -  Dir 4.1
      -  Required
      -  Run-time failures shall be minimized
      -  N/A
      -  `Dir 4.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_01.c>`_

         .. _MisraC_Dir_4_2:
    * -  Dir 4.2
      -  Advisory
      -  All usage of assembly language should be documented
      -  N/A
      -  `Dir 4.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_02.c>`_

         .. _MisraC_Dir_4_4:
    * -  Dir 4.4
      -  Advisory
      -  Sections of code should not be “commented out”
      -  `MSC04-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC04-C.+Use+comments+consistently+and+in+a+readable+fashion>`_
      -  `Dir 4.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_04.c>`_

         .. _MisraC_Dir_4_5:
    * -  Dir 4.5
      -  Advisory
      -  Identifiers in the same name space with overlapping visibility should be typographically unambiguous
      -  `DCL02-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL02-C.+Use+visually+distinct+identifiers>`_
      -  `Dir 4.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_05.c>`_

         .. _MisraC_Dir_4_6:
    * -  Dir 4.6
      -  Advisory
      -  typedefs that indicate size and signedness should be used in place of the basic numerical types
      -  N/A
      -  `Dir 4.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_06.c>`_

         .. _MisraC_Dir_4_7:
    * -  Dir 4.7
      -  Required
      -  If a function returns error information, then that error information shall be tested
      -  N/A
      -  `Dir 4.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_07.c>`_

         .. _MisraC_Dir_4_8:
    * -  Dir 4.8
      -  Advisory
      -  If a pointer to a structure or union is never dereferenced within a translation unit, then the implementation of the object should be hidden
      -  `DCL12-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL12-C.+Implement+abstract+data+types+using+opaque+types>`_
      -  | `Dir 4.8 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_08_1.c>`_
         | `Dir 4.8 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_08_2.c>`_

         .. _MisraC_Dir_4_9:
    * -  Dir 4.9
      -  Advisory
      -  A function should be used in preference to a function-like macro where they are interchangeable
      -  `PRE00-C <https://wiki.sei.cmu.edu/confluence/display/c/PRE00-C.+Prefer+inline+or+static+functions+to+function-like+macros>`_
      -  `Dir 4.9 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_09.c>`_

         .. _MisraC_Dir_4_10:
    * -  Dir 4.10
      -  Required
      -  Precautions shall be taken in order to prevent the contents of a header file being included more than once
      -  `PRE06-C <https://wiki.sei.cmu.edu/confluence/display/c/PRE06-C.+Enclose+header+files+in+an+include+guard>`_
      -  `Dir 4.10 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_10.c>`_

         .. _MisraC_Dir_4_11:
    * -  Dir 4.11
      -  Required
      -  The validity of values passed to library functions shall be checked
      -  N/A
      -  `Dir 4.11 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_11.c>`_

         .. _MisraC_Dir_4_12:
    * -  Dir 4.12
      -  Required
      -  Dynamic memory allocation shall not be used
      -  `STR01-C <https://wiki.sei.cmu.edu/confluence/display/c/STR01-C.+Adopt+and+implement+a+consistent+plan+for+managing+strings>`_
      -  `Dir 4.12 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_12.c>`_

         .. _MisraC_Dir_4_13:
    * -  Dir 4.13
      -  Advisory
      -  Functions which are designed to provide operations on a resource should be called in an appropriate sequence
      -  N/A
      -  `Dir 4.13 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_13.c>`_

         .. _MisraC_Dir_4_14:
    * -  Dir 4.14
      -  Required
      -  The validity of values received from external sources shall be checked

      -  N/A
      -  `Dir 4.14 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_14.c>`_

         .. _MisraC_Rule_1_2:
    * -  Rule 1.2
      -  Advisory
      -  Language extensions should not be used
      -  `MSC04-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC04-C.+Use+comments+consistently+and+in+a+readable+fashion>`_
      -  `Rule 1.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_01_02.c>`_

         .. _MisraC_Rule_1_3:
    * -  Rule 1.3
      -  Required
      -  There shall be no occurrence of undefined or critical unspecified behaviour
      -  N/A
      -  `Rule 1.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_01_03.c>`_

         .. _MisraC_Rule_2_1:
    * -  Rule 2.1
      -  Required
      -  A project shall not contain unreachable code
      -  `MSC07-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC07-C.+Detect+and+remove+dead+code>`_
      -  | `Rule 2.1 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_02_01_1.c>`_
         | `Rule 2.1 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_02_01_2.c>`_

         .. _MisraC_Rule_2_2:
    * -  Rule 2.2
      -  Required
      -  There shall be no dead code
      -  `MSC12-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC12-C.+Detect+and+remove+code+that+has+no+effect+or+is+never+executed>`_
      -  `Rule 2.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_02_02.c>`_

         .. _MisraC_Rule_2_3:
    * -  Rule 2.3
      -  Advisory
      -  A project should not contain unused type declarations
      -  N/A
      -  `Rule 2.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_02_03.c>`_

         .. _MisraC_Rule_2_6:
    * -  Rule 2.6
      -  Advisory
      -  A function should not contain unused label declarations
      -  N/A
      -  `Rule 2.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_02_06.c>`_

         .. _MisraC_Rule_2_7:
    * -  Rule 2.7
      -  Advisory
      -  There should be no unused parameters in functions
      -  N/A
      -  `Rule 2.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_02_07.c>`_

         .. _MisraC_Rule_3_1:
    * -  Rule 3.1
      -  Required
      -  The character sequences /* and // shall not be used within a comment
      -  `MSC04-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC04-C.+Use+comments+consistently+and+in+a+readable+fashion>`_
      -  `Rule 3.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_03_01.c>`_

         .. _MisraC_Rule_3_2:
    * -  Rule 3.2
      -  Required
      -  Line-splicing shall not be used in // comments
      -  N/A
      -  `Rule 3.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_03_02.c>`_

         .. _MisraC_Rule_4_1:
    * -  Rule 4.1
      -  Required
      -  Octal and hexadecimal escape sequences shall be terminated
      -  `MSC09-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC09-C.+Character+encoding%3A+Use+subset+of+ASCII+for+safety>`_
      -  `Rule 4.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_04_01.c>`_

         .. _MisraC_Rule_4_2:
    * -  Rule 4.2
      -  Advisory
      -  Trigraphs should not be used
      -  `PRE07-C <https://wiki.sei.cmu.edu/confluence/display/c/PRE07-C.+Avoid+using+repeated+question+marks>`_
      -  `Rule 4.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_04_02.c>`_

         .. _MisraC_Rule_5_1:
    * -  Rule 5.1
      -  Required
      -  External identifiers shall be distinct
      -  `DCL23-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL23-C.+Guarantee+that+mutually+visible+identifiers+are+unique>`_
      -  | `Rule 5.1 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_01_1.c>`_
         | `Rule 5.1 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_01_2.c>`_

         .. _MisraC_Rule_5_2:
    * -  Rule 5.2
      -  Required
      -  Identifiers declared in the same scope and name space shall be distinct
      -  `DCL23-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL23-C.+Guarantee+that+mutually+visible+identifiers+are+unique>`_
      -  `Rule 5.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_02.c>`_

         .. _MisraC_Rule_5_3:
    * -  Rule 5.3
      -  Required
      -  An identifier declared in an inner scope shall not hide an identifier declared in an outer scope
      -  `DCL23-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL23-C.+Guarantee+that+mutually+visible+identifiers+are+unique>`_
      -  `Rule 5.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_03.c>`_

         .. _MisraC_Rule_5_4:
    * -  Rule 5.4
      -  Required
      -  Macro identifiers shall be distinct
      -  `DCL23-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL23-C.+Guarantee+that+mutually+visible+identifiers+are+unique>`_
      -  `Rule 5.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_04.c>`_

         .. _MisraC_Rule_5_5:
    * -  Rule 5.5
      -  Required
      -  Identifiers shall be distinct from macro names
      -  `DCL23-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL23-C.+Guarantee+that+mutually+visible+identifiers+are+unique>`_
      -  `Rule 5.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_05.c>`_

         .. _MisraC_Rule_5_6:
    * -  Rule 5.6
      -  Required
      -  A typedef name shall be a unique identifier
      -  N/A
      -  `Rule 5.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_06.c>`_

         .. _MisraC_Rule_5_7:
    * -  Rule 5.7
      -  Required
      -  A tag name shall be a unique identifier
      -  N/A
      -  `Rule 5.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_07.c>`_

         .. _MisraC_Rule_5_8:
    * -  Rule 5.8
      -  Required
      -  Identifiers that define objects or functions with external linkage shall be unique
      -  N/A
      -  | `Rule 5.8 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_08_1.c>`_
         | `Rule 5.8 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_08_2.c>`_

         .. _MisraC_Rule_5_9:
    * -  Rule 5.9
      -  Advisory
      -  Identifiers that define objects or functions with internal linkage should be unique
      -  N/A
      -  | `Rule 5.9 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_09_1.c>`_
         | `Rule 5.9 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_09_2.c>`_

         .. _MisraC_Rule_6_1:
    * -  Rule 6.1
      -  Required
      -  Bit-fields shall only be declared with an appropriate type
      -  `INT14-C <https://wiki.sei.cmu.edu/confluence/display/c/INT14-C.+Avoid+performing+bitwise+and+arithmetic+operations+on+the+same+data>`_
      -  `Rule 6.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_06_01.c>`_

         .. _MisraC_Rule_6_2:
    * -  Rule 6.2
      -  Required
      -  Single-bit named bit fields shall not be of a signed type
      -  `INT14-C <https://wiki.sei.cmu.edu/confluence/display/c/INT14-C.+Avoid+performing+bitwise+and+arithmetic+operations+on+the+same+data>`_
      -  `Rule 6.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_06_02.c>`_

         .. _MisraC_Rule_7_1:
    * -  Rule 7.1
      -  Required
      -  Octal constants shall not be used
      -  `DCL18-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL18-C.+Do+not+begin+integer+constants+with+0+when+specifying+a+decimal+value>`_
      -  `Rule 7.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_07_01.c>`_

         .. _MisraC_Rule_7_2:
    * -  Rule 7.2
      -  Required
      -  A u or U suffix shall be applied to all integer constants that are represented in an unsigned type
      -  N/A
      -  `Rule 7.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_07_02.c>`_

         .. _MisraC_Rule_7_3:
    * -  Rule 7.3
      -  Required
      -  The lowercase character l shall not be used in a literal suffix
      -  `DCL16-C <https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=87152241>`_
      -  `Rule 7.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_07_03.c>`_

         .. _MisraC_Rule_7_4:
    * -  Rule 7.4
      -  Required
      -  A string literal shall not be assigned to an object unless the objects type is pointer to const-qualified char
      -  N/A
      -  `Rule 7.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_07_04.c>`_

         .. _MisraC_Rule_8_1:
    * -  Rule 8.1
      -  Required
      -  Types shall be explicitly specified
      -  N/A
      -  `Rule 8.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_01.c>`_

         .. _MisraC_Rule_8_2:
    * -  Rule 8.2
      -  Required
      -  Function types shall be in prototype form with named parameters
      -  `DCL20-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL20-C.+Explicitly+specify+void+when+a+function+accepts+no+arguments>`_
      -  `Rule 8.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_02.c>`_

         .. _MisraC_Rule_8_3:
    * -  Rule 8.3
      -  Required
      -  All declarations of an object or function shall use the same names and type qualifiers
      -  N/A
      -  `Rule 8.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_03.c>`_

         .. _MisraC_Rule_8_4:
    * -  Rule 8.4
      -  Required
      -  A compatible declaration shall be visible when an object or function with external linkage is defined
      -  N/A
      -  `Rule 8.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_04.c>`_

         .. _MisraC_Rule_8_5:
    * -  Rule 8.5
      -  Required
      -  An external object or function shall be declared once in one and only one file
      -  N/A
      -  | `Rule 8.5 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_05_1.c>`_
         | `Rule 8.5 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_05_2.c>`_

         .. _MisraC_Rule_8_6:
    * -  Rule 8.6
      -  Required
      -  An identifier with external linkage shall have exactly one external definition
      -  N/A
      -  | `Rule 8.6 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_06_1.c>`_
         | `Rule 8.6 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_06_2.c>`_

         .. _MisraC_Rule_8_8:
    * -  Rule 8.8
      -  Required
      -  The static storage class specifier shall be used in all declarations of objects and functions that have internal linkage
      -  `DCL15-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL15-C.+Declare+file-scope+objects+or+functions+that+do+not+need+external+linkage+as+static>`_
      -  `Rule 8.8 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_08.c>`_

         .. _MisraC_Rule_8_9:
    * -  Rule 8.9
      -  Advisory
      -  An object should be defined at block scope if its identifier only appears in a single function
      -  `DCL19-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL19-C.+Minimize+the+scope+of+variables+and+functions>`_
      -  `Rule 8.9 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_09.c>`_

         .. _MisraC_Rule_8_10:
    * -  Rule 8.10
      -  Required
      -  An inline function shall be declared with the static storage class
      -  N/A
      -  `Rule 8.10 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_10.c>`_

         .. _MisraC_Rule_8_12:
    * -  Rule 8.12
      -  Required
      -  Within an enumerator list, the value of an implicitly-specified enumeration constant shall be unique
      -  `INT09-C <https://wiki.sei.cmu.edu/confluence/display/c/INT09-C.+Ensure+enumeration+constants+map+to+unique+values>`_
      -  `Rule 8.12 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_12.c>`_

         .. _MisraC_Rule_8_14:
    * -  Rule 8.14
      -  Required
      -  The restrict type qualifier shall not be used
      -  N/A
      -  `Rule 8.14 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_14.c>`_

         .. _MisraC_Rule_9_1:
    * -  Rule 9.1
      -  Mandatory
      -  The value of an object with automatic storage duration shall not be read before it has been set
      -  N/A
      -  `Rule 9.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_09_01.c>`_

         .. _MisraC_Rule_9_2:
    * -  Rule 9.2
      -  Required
      -  The initializer for an aggregate or union shall be enclosed in braces
      -  N/A
      -  `Rule 9.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_09_02.c>`_

         .. _MisraC_Rule_9_3:
    * -  Rule 9.3
      -  Required
      -  Arrays shall not be partially initialized
      -  N/A
      -  `Rule 9.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_09_03.c>`_

         .. _MisraC_Rule_9_4:
    * -  Rule 9.4
      -  Required
      -  An element of an object shall not be initialized more than once
      -  N/A
      -  `Rule 9.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_09_04.c>`_

         .. _MisraC_Rule_9_5:
    * -  Rule 9.5
      -  Required
      -  Where designated initializers are used to initialize an array object the size of the array shall be specified explicitly
      -  N/A
      -  `Rule 9.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_09_05.c>`_

         .. _MisraC_Rule_10_1:
    * -  Rule 10.1
      -  Required
      -  Operands shall not be of an inappropriate essential type
      -  `STR04-C <https://wiki.sei.cmu.edu/confluence/display/c/STR04-C.+Use+plain+char+for+characters+in+the+basic+character+set>`_
      -  `Rule 10.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_01.c>`_

         .. _MisraC_Rule_10_2:
    * -  Rule 10.2
      -  Required
      -  Expressions of essentially character type shall not be used inappropriately in addition and subtraction operations
      -  `STR04-C <https://wiki.sei.cmu.edu/confluence/display/c/STR04-C.+Use+plain+char+for+characters+in+the+basic+character+set>`_
      -  `Rule 10.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_02.c>`_

         .. _MisraC_Rule_10_3:
    * -  Rule 10.3
      -  Required
      -  The value of an expression shall not be assigned to an object with a narrower essential type or of a different essential type category
      -  `STR04-C <https://wiki.sei.cmu.edu/confluence/display/c/STR04-C.+Use+plain+char+for+characters+in+the+basic+character+set>`_
      -  `Rule 10.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_03.c>`_

         .. _MisraC_Rule_10_4:
    * -  Rule 10.4
      -  Required
      -  Both operands of an operator in which the usual arithmetic conversions are performed shall have the same essential type category
      -  `STR04-C <https://wiki.sei.cmu.edu/confluence/display/c/STR04-C.+Use+plain+char+for+characters+in+the+basic+character+set>`_
      -  `Rule 10.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_04.c>`_

         .. _MisraC_Rule_10_5:
    * -  Rule 10.5
      -  Advisory
      -  The value of an expression should not be cast to an inappropriate essential type
      -  N/A
      -  `Rule 10.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_05.c>`_

         .. _MisraC_Rule_10_6:
    * -  Rule 10.6
      -  Required
      -  The value of a composite expression shall not be assigned to an object with wider essential type
      -  `INT02-C <https://wiki.sei.cmu.edu/confluence/display/c/INT02-C.+Understand+integer+conversion+rules>`_
      -  `Rule 10.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_06.c>`_

         .. _MisraC_Rule_10_7:
    * -  Rule 10.7
      -  Required
      -  If a composite expression is used as one operand of an operator in which the usual arithmetic conversions are performed then the other operand shall not have wider essential type
      -  `INT02-C <https://wiki.sei.cmu.edu/confluence/display/c/INT02-C.+Understand+integer+conversion+rules>`_
      -  `Rule 10.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_07.c>`_

         .. _MisraC_Rule_10_8:
    * -  Rule 10.8
      -  Required
      -  The value of a composite expression shall not be cast to a different essential type category or a wider essential type
      -  `INT02-C <https://wiki.sei.cmu.edu/confluence/display/c/INT02-C.+Understand+integer+conversion+rules>`_
      -  `Rule 10.8 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_08.c>`_

         .. _MisraC_Rule_11_2:
    * -  Rule 11.2
      -  Required
      -  Conversions shall not be performed between a pointer to an incomplete type and any other type
      -  N/A
      -  `Rule 11.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_11_02.c>`_

         .. _MisraC_Rule_11_6:
    * -  Rule 11.6
      -  Required
      -  A cast shall not be performed between pointer to void and an arithmetic type
      -  N/A
      -  `Rule 11.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_11_06.c>`_

         .. _MisraC_Rule_11_7:
    * -  Rule 11.7
      -  Required
      -  A cast shall not be performed between pointer to object and a noninteger arithmetic type
      -  N/A
      -  `Rule 11.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_11_07.c>`_

         .. _MisraC_Rule_11_8:
    * -  Rule 11.8
      -  Required
      -  A cast shall not remove any const or volatile qualification from the type pointed to by a pointer
      -  `EXP05-C <https://wiki.sei.cmu.edu/confluence/display/c/EXP05-C.+Do+not+cast+away+a+const+qualification>`_
      -  `Rule 11.8 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_11_08.c>`_

         .. _MisraC_Rule_11_9:
    * -  Rule 11.9
      -  Required
      -  The macro NULL shall be the only permitted form of integer null pointer constant
      -  N/A
      -  `Rule 11.9 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_11_09.c>`_

         .. _MisraC_Rule_12_1:
    * -  Rule 12.1
      -  Advisory
      -  The precedence of operators within expressions should be made explicit
      -  `EXP00-C <https://wiki.sei.cmu.edu/confluence/display/c/EXP00-C.+Use+parentheses+for+precedence+of+operation>`_
      -  `Rule 12.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_12_01.c>`_

         .. _MisraC_Rule_12_2:
    * -  Rule 12.2
      -  Required
      -  The right hand operand of a shift operator shall lie in the range zero to one less than the width in bits of the essential type of the left hand operand
      -  N/A
      -  `Rule 12.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_12_02.c>`_

         .. _MisraC_Rule_12_4:
    * -  Rule 12.4
      -  Advisory
      -  Evaluation of constant expressions should not lead to unsigned integer wrap-around
      -  N/A
      -  `Rule 12.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_12_04.c>`_

         .. _MisraC_Rule_12_5:
    * -  Rule 12.5
      -  Mandatory
      -  The sizeof operator shall not have an operand which is a function parameter declared as “array of type”
      -  N/A
      -  `Rule 12.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_12_05.c>`_

         .. _MisraC_Rule_13_1:
    * -  Rule 13.1
      -  Required
      -  Initializer lists shall not contain persistent side effects
      -  N/A
      -  | `Rule 13.1 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_13_01_1.c>`_
         | `Rule 13.1 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_13_01_2.c>`_

         .. _MisraC_Rule_13_2:
    * -  Rule 13.2
      -  Required
      -  The value of an expression and its persistent side effects shall be the same under all permitted evaluation orders
      -  N/A
      -  `Rule 13.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_13_02.c>`_

         .. _MisraC_Rule_13_3:
    * -  Rule 13.3
      -  Advisory
      -  A full expression containing an increment (++) or decrement (--) operator should have no other potential side effects other than that caused by the increment or decrement operator
      -  N/A
      -  `Rule 13.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_13_03.c>`_

         .. _MisraC_Rule_13_4:
    * -  Rule 13.4
      -  Advisory
      -  The result of an assignment operator should not be used
      -  N/A
      -  `Rule 13.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_13_04.c>`_

         .. _MisraC_Rule_13_5:
    * -  Rule 13.5
      -  Required
      -  The right hand operand of a logical && or || operator shall not contain persistent side effects
      -  `EXP10-C <https://wiki.sei.cmu.edu/confluence/display/c/EXP10-C.+Do+not+depend+on+the+order+of+evaluation+of+subexpressions+or+the+order+in+which+side+effects+take+place>`_
      -  | `Rule 13.5 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_13_05_1.c>`_
         | `Rule 13.5 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_13_05_2.c>`_

         .. _MisraC_Rule_13_6:
    * -  Rule 13.6
      -  Mandatory
      -  The operand of the sizeof operator shall not contain any expression which has potential side effects
      -  N/A
      -  `Rule 13.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_13_06.c>`_

         .. _MisraC_Rule_14_1:
    * -  Rule 14.1
      -  Required
      -  A loop counter shall not have essentially floating type
      -  N/A
      -  `Rule 14.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_14_01.c>`_

         .. _MisraC_Rule_14_2:
    * -  Rule 14.2
      -  Required
      -  A for loop shall be well-formed
      -  N/A
      -  `Rule 14.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_14_02.c>`_

         .. _MisraC_Rule_14_3:
    * -  Rule 14.3
      -  Required
      -  Controlling expressions shall not be invariant
      -  N/A
      -  `Rule 14.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_14_03.c>`_

         .. _MisraC_Rule_14_4:
    * -  Rule 14.4
      -  Required
      -  The controlling expression of an if statement and the controlling expression of an iteration-statement shall have essentially Boolean type
      -  N/A
      -  `Rule 14.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_14_04.c>`_

         .. _MisraC_Rule_15_2:
    * -  Rule 15.2
      -  Required
      -  The goto statement shall jump to a label declared later in the same function
      -  N/A
      -  `Rule 15.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_15_02.c>`_

         .. _MisraC_Rule_15_3:
    * -  Rule 15.3
      -  Required
      -  Any label referenced by a goto statement shall be declared in the same block, or in any block enclosing the goto statement
      -  N/A
      -  `Rule 15.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_15_03.c>`_

         .. _MisraC_Rule_15_6:
    * -  Rule 15.6
      -  Required
      -  The body of an iteration-statement or a selection-statement shall be a compound-statement
      -  `EXP19-C <https://wiki.sei.cmu.edu/confluence/display/c/EXP19-C.+Use+braces+for+the+body+of+an+if%2C+for%2C+or+while+statement>`_
      -  `Rule 15.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_15_06.c>`_

         .. _MisraC_Rule_15_7:
    * -  Rule 15.7
      -  Required
      -  All if else if constructs shall be terminated with an else statement
      -  N/A
      -  `Rule 15.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_15_07.c>`_

         .. _MisraC_Rule_16_1:
    * -  Rule 16.1
      -  Required
      -  All switch statements shall be well-formed
      -  N/A
      -  `Rule 16.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_16_01.c>`_

         .. _MisraC_Rule_16_2:
    * -  Rule 16.2
      -  Required
      -  A switch label shall only be used when the most closely-enclosing compound statement is the body of a switch statement
      -  `MSC20-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC20-C.+Do+not+use+a+switch+statement+to+transfer+control+into+a+complex+block>`_
      -  `Rule 16.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_16_02.c>`_

         .. _MisraC_Rule_16_3:
    * -  Rule 16.3
      -  Required
      -  An unconditional break statement shall terminate every switch-clause
      -  N/A
      -  `Rule 16.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_16_03.c>`_

         .. _MisraC_Rule_16_4:
    * -  Rule 16.4
      -  Required
      -  Every switch statement shall have a default label
      -  N/A
      -  `Rule 16.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_16_04.c>`_

         .. _MisraC_Rule_16_5:
    * -  Rule 16.5
      -  Required
      -  A default label shall appear as either the first or the last switch label of a switch statement
      -  N/A
      -  `Rule 16.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_16_05.c>`_

         .. _MisraC_Rule_16_6:
    * -  Rule 16.6
      -  Required
      -  Every switch statement shall have at least two switch-clauses
      -  N/A
      -  `Rule 16.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_16_06.c>`_

         .. _MisraC_Rule_16_7:
    * -  Rule 16.7
      -  Required
      -  A switch-expression shall not have essentially Boolean type
      -  N/A
      -  `Rule 16.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_16_07.c>`_

         .. _MisraC_Rule_17_1:
    * -  Rule 17.1
      -  Required
      -  The features of <stdarg.h> shall not be used
      -  `ERR00-C <https://wiki.sei.cmu.edu/confluence/display/c/ERR00-C.+Adopt+and+implement+a+consistent+and+comprehensive+error-handling+policy>`_
      -  `Rule 17.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_17_01.c>`_

         .. _MisraC_Rule_17_2:
    * -  Rule 17.2
      -  Required
      -  Functions shall not call themselves, either directly or indirectly
      -  `MEM05-C <https://wiki.sei.cmu.edu/confluence/display/c/MEM05-C.+Avoid+large+stack+allocations>`_
      -  `Rule 17.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_17_02.c>`_

         .. _MisraC_Rule_17_3:
    * -  Rule 17.3
      -  Mandatory
      -  A function shall not be declared implicitly
      -  N/A
      -  `Rule 17.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_17_03.c>`_

         .. _MisraC_Rule_17_4:
    * -  Rule 17.4
      -  Mandatory
      -  All exit paths from a function with non-void return type shall have an explicit return statement with an expression
      -  N/A
      -  `Rule 17.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_17_04.c>`_

         .. _MisraC_Rule_17_5:
    * -  Rule 17.5
      -  Advisory
      -  The function argument corresponding to a parameter declared to have an array type shall have an appropriate number of elements
      -  N/A
      -  `Rule 17.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_17_05.c>`_

         .. _MisraC_Rule_17_6:
    * -  Rule 17.6
      -  Mandatory
      -  The declaration of an array parameter shall not contain the static keyword between the [ ]
      -  N/A
      -  `Rule 17.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_17_06.c>`_

         .. _MisraC_Rule_17_7:
    * -  Rule 17.7
      -  Required
      -  The value returned by a function having non-void return type shall be used
      -  N/A
      -  `Rule 17.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_17_07.c>`_

         .. _MisraC_Rule_18_1:
    * -  Rule 18.1
      -  Required
      -  A pointer resulting from arithmetic on a pointer operand shall address an element of the same array as that pointer operand
      -  `EXP08-C <https://wiki.sei.cmu.edu/confluence/display/c/EXP08-C.+Ensure+pointer+arithmetic+is+used+correctly>`_
      -  `Rule 18.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_18_01.c>`_

         .. _MisraC_Rule_18_2:
    * -  Rule 18.2
      -  Required
      -  Subtraction between pointers shall only be applied to pointers that address elements of the same array
      -  `EXP08-C <https://wiki.sei.cmu.edu/confluence/display/c/EXP08-C.+Ensure+pointer+arithmetic+is+used+correctly>`_
      -  `Rule 18.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_18_02.c>`_

         .. _MisraC_Rule_18_3:
    * -  Rule 18.3
      -  Required
      -  The relational operators >, >=, < and <= shall not be applied to objects of pointer type except where they point into the same object
      -  `EXP08-C <https://wiki.sei.cmu.edu/confluence/display/c/EXP08-C.+Ensure+pointer+arithmetic+is+used+correctly>`_
      -  `Rule 18.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_18_03.c>`_

         .. _MisraC_Rule_18_5:
    * -  Rule 18.5
      -  Advisory
      -  Declarations should contain no more than two levels of pointer nesting
      -  N/A
      -  `Rule 18.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_18_05.c>`_

         .. _MisraC_Rule_18_6:
    * -  Rule 18.6
      -  Required
      -  The address of an object with automatic storage shall not be copied to another object that persists after the first object has ceased to exist
      -  N/A
      -  | `Rule 18.6 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_18_06_1.c>`_
         | `Rule 18.6 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_18_06_2.c>`_

         .. _MisraC_Rule_18_8:
    * -  Rule 18.8
      -  Required
      -  Variable-length array types shall not be used
      -  N/A
      -  `Rule 18.8 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_18_08.c>`_

         .. _MisraC_Rule_19_1:
    * -  Rule 19.1
      -  Mandatory
      -  An object shall not be assigned or copied to an overlapping object
      -  N/A
      -  `Rule 19.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_19_01.c>`_

         .. _MisraC_Rule_20_2:
    * -  Rule 20.2
      -  Required
      -  The ', or \ characters and the /* or // character sequences shall not occur in a header file name"
      -  N/A
      -  `Rule 20.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_02.c>`_

         .. _MisraC_Rule_20_3:
    * -  Rule 20.3
      -  Required
      -  The #include directive shall be followed by either a <filename> or "filename" sequence
      -  N/A
      -  `Rule 20.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_03.c>`_

         .. _MisraC_Rule_20_4:
    * -  Rule 20.4
      -  Required
      -  A macro shall not be defined with the same name as a keyword
      -  N/A
      -  `Rule 20.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_04.c>`_

         .. _MisraC_Rule_20_7:
    * -  Rule 20.7
      -  Required
      -  Expressions resulting from the expansion of macro parameters shall be enclosed in parentheses
      -  `PRE01-C <https://wiki.sei.cmu.edu/confluence/display/c/PRE01-C.+Use+parentheses+within+macros+around+parameter+names>`_
      -  `Rule 20.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_07.c>`_

         .. _MisraC_Rule_20_8:
    * -  Rule 20.8
      -  Required
      -  The controlling expression of a #if or #elif preprocessing directive shall evaluate to 0 or 1
      -  N/A
      -  `Rule 20.8 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_08.c>`_

         .. _MisraC_Rule_20_9:
    * -  Rule 20.9
      -  Required
      -  All identifiers used in the controlling expression of #if or #elif preprocessing directives shall be #defined before evaluation
      -  N/A
      -  `Rule 20.9 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_09.c>`_

         .. _MisraC_Rule_20_11:
    * -  Rule 20.11
      -  Required
      -  A macro parameter immediately following a # operator shall not immediately be followed by a ## operator
      -  N/A
      -  `Rule 20.11 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_11.c>`_

         .. _MisraC_Rule_20_12:
    * -  Rule 20.12
      -  Required
      -  A macro parameter used as an operand to the # or ## operators, which is itself subject to further macro replacement, shall only be used as an operand to these operators
      -  N/A
      -  `Rule 20.12 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_12.c>`_

         .. _MisraC_Rule_20_13:
    * -  Rule 20.13
      -  Required
      -  A line whose first token is # shall be a valid preprocessing directive
      -  N/A
      -  `Rule 20.13 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_13.c>`_

         .. _MisraC_Rule_20_14:
    * -  Rule 20.14
      -  Required
      -  All #else, #elif and #endif preprocessor directives shall reside in the same file as the #if, #ifdef or #ifndef directive to which they are related
      -  N/A
      -  `Rule 20.14 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_14.c>`_

         .. _MisraC_Rule_21_1:
    * -  Rule 21.1
      -  Required
      -  #define and #undef shall not be used on a reserved identifier or reserved macro name
      -  N/A
      -  `Rule 21.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_01.c>`_

         .. _MisraC_Rule_21_2:
    * -  Rule 21.2
      -  Required
      -  A reserved identifier or macro name shall not be declared
      -  N/A
      -  `Rule 21.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_02.c>`_

         .. _MisraC_Rule_21_3:
    * -  Rule 21.3
      -  Required
      -  The memory allocation and deallocation functions of <stdlib.h> shall not be used
      -  `MSC24-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC24-C.+Do+not+use+deprecated+or+obsolescent+functions>`_
      -  `Rule 21.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_03.c>`_

         .. _MisraC_Rule_21_4:
    * -  Rule 21.4
      -  Required
      -  The standard header file <setjmp.h> shall not be used
      -  N/A
      -  `Rule 21.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_04.c>`_

         .. _MisraC_Rule_21_6:
    * -  Rule 21.6
      -  Required
      -  The Standard Library input/output functions shall not be used
      -  N/A
      -  `Rule 21.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_06.c>`_

         .. _MisraC_Rule_21_7:
    * -  Rule 21.7
      -  Required
      -  The atof, atoi, atol and atoll functions of <stdlib.h> shall not be used
      -  N/A
      -  `Rule 21.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_07.c>`_

         .. _MisraC_Rule_21_9:
    * -  Rule 21.9
      -  Required
      -  The library functions bsearch and qsort of <stdlib.h> shall not be used
      -  N/A
      -  `Rule 21.9 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_09.c>`_

         .. _MisraC_Rule_21_11:
    * -  Rule 21.11
      -  Required
      -  The standard header file <tgmath.h> shall not be used
      -  N/A
      -  `Rule 21.11 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_11.c>`_

         .. _MisraC_Rule_21_12:
    * -  Rule 21.12
      -  Advisory
      -  The exception handling features of <fenv.h> should not be used
      -  N/A
      -  `Rule 21.12 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_12.c>`_

         .. _MisraC_Rule_21_13:
    * -  Rule 21.13
      -  Mandatory
      -  Any value passed to a function in <ctype.h> shall be representable as an unsigned char or be the value EO
      -  N/A
      -  `Rule 21.13 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_13.c>`_

         .. _MisraC_Rule_21_14:
    * -  Rule 21.14
      -  Required
      -  The Standard Library function memcmp shall not be used to compare null terminated strings
      -  N/A
      -  `Rule 21.14 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_14.c>`_

         .. _MisraC_Rule_21_15:
    * -  Rule 21.15
      -  Required
      -  The pointer arguments to the Standard Library functions memcpy, memmove and memcmp shall be pointers to qualified or unqualified versions of compatible types
      -  N/A
      -  `Rule 21.15 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_15.c>`_

         .. _MisraC_Rule_21_16:
    * -  Rule 21.16
      -  Required
      -  The pointer arguments to the Standard Library function memcmp shall point to either a pointer type, an essentially signed type, an essentially unsigned type, an essentially Boolean type or an essentially enum type
      -  N/A
      -  `Rule 21.16 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_16.c>`_

         .. _MisraC_Rule_21_17:
    * -  Rule 21.17
      -  Mandatory
      -  Use of the string handling functions from <string.h> shall not result in accesses beyond the bounds of the objects referenced by their pointer parameters
      -  N/A
      -  `Rule 21.17 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_17.c>`_

         .. _MisraC_Rule_21_18:
    * -  Rule 21.18
      -  Mandatory
      -  The size_t argument passed to any function in <string.h> shall have an appropriate value
      -  N/A
      -  `Rule 21.18 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_18.c>`_

         .. _MisraC_Rule_21_19:
    * -  Rule 21.19
      -  Mandatory
      -  The pointers returned by the Standard Library functions localeconv, getenv, setlocale or, strerror shall only be used as if they have pointer to const-qualified type
      -  N/A
      -  `Rule 21.19 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_19.c>`_

         .. _MisraC_Rule_21_20:
    * -  Rule 21.20
      -  Mandatory
      -  The pointer returned by the Standard Library functions asctime, ctime, gmtime, localtime, localeconv, getenv, setlocale or strerror shall not be used following a subsequent call to the same function
      -  N/A
      -  `Rule 21.20 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_20.c>`_

         .. _MisraC_Rule_22_1:
    * -  Rule 22.1
      -  Required
      -  All resources obtained dynamically by means of Standard Library functions shall be explicitly released
      -  N/A
      -  `Rule 22.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_01.c>`_

         .. _MisraC_Rule_22_2:
    * -  Rule 22.2
      -  Mandatory
      -  A block of memory shall only be freed if it was allocated by means of a Standard Library function
      -  N/A
      -  `Rule 22.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_02.c>`_

         .. _MisraC_Rule_22_3:
    * -  Rule 22.3
      -  Required
      -  The same file shall not be open for read and write access at the same time on different streams
      -  N/A
      -  `Rule 22.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_03.c>`_

         .. _MisraC_Rule_22_4:
    * -  Rule 22.4
      -  Mandatory
      -  There shall be no attempt to write to a stream which has been opened as read-only
      -  N/A
      -  `Rule 22.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_04.c>`_

         .. _MisraC_Rule_22_5:
    * -  Rule 22.5
      -  Mandatory
      -  A pointer to a FILE object shall not be dereferenced
      -  N/A
      -  `Rule 22.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_05.c>`_

         .. _MisraC_Rule_22_6:
    * -  Rule 22.6
      -  Mandatory
      -  The value of a pointer to a FILE shall not be used after the associated stream has been closed
      -  N/A
      -  `Rule 22.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_06.c>`_

         .. _MisraC_Rule_22_7:
    * -  Rule 22.7
      -  Required
      -  The macro EOF shall only be compared with the unmodified return value from any Standard Library function capable of returning EOF
      -  N/A
      -  `Rule 22.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_07.c>`_

         .. _MisraC_Rule_22_8:
    * -  Rule 22.8
      -  Required
      -  The value of errno shall be set to zero prior to a call to an errno-setting-function
      -  N/A
      -  `Rule 22.8 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_08.c>`_

         .. _MisraC_Rule_22_9:
    * -  Rule 22.9
      -  Required
      -  The value of errno shall be tested against zero after calling an errno-setting-function
      -  N/A
      -  `Rule 22.9 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_09.c>`_

         .. _MisraC_Rule_22_10:
    * -  Rule 22.10
      -  Required
      -  The value of errno shall only be tested when the last function to be called was an errno-setting-function
      -  N/A
      -  `Rule 22.10 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_10.c>`_

Additional rules
****************

Rule A.1: Conditional Compilation
=================================

Severity
--------

Required

Description
-----------

Do not conditionally compile function declarations in header files.  Do not
conditionally compile structure declarations in header files.  You may
conditionally exclude fields within structure definitions to avoid wasting
memory when the feature they support is not enabled.

Rationale
---------

Excluding declarations from the header based on compile-time options may prevent
their documentation from being generated. Their absence also prevents use of
``if (IS_ENABLED(CONFIG_FOO)) {}`` as an alternative to preprocessor
conditionals when the code path should change based on the selected options.

.. _coding_guideline_inclusive_language:

Rule A.2: Inclusive Language
============================

Severity
--------

Required

Description
-----------

Do not introduce new usage of offensive terms listed below. This rule applies
but is not limited to source code, comments, documentation, and branch names.
Replacement terms may vary by area or subsystem, but should aim to follow
updated industry standards when possible.

Exceptions are allowed for maintaining existing implementations or adding new
implementations of industry standard specifications governed externally to the
Zephyr Project.

Existing usage is recommended to change as soon as updated industry standard
specifications become available or new terms are publicly announced by the
governing body, or immediately if no specifications apply.

.. list-table::
   :header-rows: 1

   * - Offensive Terms
     - Recommended Replacements

   * - ``{master,leader} / slave``
     - - ``{primary,main} / {secondary,replica}``
       - ``{initiator,requester} / {target,responder}``
       - ``{controller,host} / {device,worker,proxy,target}``
       - ``director / performer``
       - ``central / peripheral``

   * - ``blacklist / whitelist``
     - * ``denylist / allowlist``
       * ``blocklist / allowlist``
       * ``rejectlist / acceptlist``

   * - ``grandfather policy``
     - * ``legacy``

   * - ``sanity``
     - * ``coherence``
       * ``confidence``


Rationale
---------

Offensive terms do not create an inclusive community environment and therefore
violate the Zephyr Project `Code of Conduct`_. This coding rule was inspired by
a similar rule in `Linux`_.

.. _Code of Conduct: https://github.com/zephyrproject-rtos/zephyr/blob/main/CODE_OF_CONDUCT.md
.. _Linux: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=49decddd39e5f6132ccd7d9fdc3d7c470b0061bb

Status
------

Related GitHub Issues and Pull Requests are tagged with the `Inclusive Language Label`_.

.. list-table::
   :header-rows: 1

   * - Area
     - Selected Replacements
     - Status

   * - :ref:`bluetooth_api`
     - See `Bluetooth Appropriate Language Mapping Tables`_
     -

   * - CAN
     - This `CAN in Automation Inclusive Language news post`_ has a list of general
       recommendations. See `CAN in Automation Inclusive Language`_ for terms to
       be used in specification document updates.
     -

   * - eSPI
     - * ``master / slave`` => TBD
     -

   * - gPTP
     - * ``master / slave`` => TBD
     -

   * - :ref:`i2c_api`
     - * ``master / slave`` => TBD
     - NXP publishes the `I2C Specification`_ and has selected ``controller /
       target`` as replacement terms, but the timing to publish an announcement
       or new specification is TBD. Zephyr will update I2C when replacement
       terminology is confirmed by a public announcement or updated
       specification.

       See :github:`Zephyr issue 27033 <27033>`.

   * - :ref:`i2s_api`
     - * ``master / slave`` => TBD
     -

   * - SMP/AMP
     - * ``master / slave`` => TBD
     -

   * - :ref:`spi_api`
     - * ``master / slave`` => ``controller / peripheral``
       * ``MOSI / MISO / SS`` => ``SDO / SDI / CS``
     - The Open Source Hardware Association has selected these replacement
       terms. See `OSHWA Resolution to Redefine SPI Signal Names`_

   * - :ref:`twister_script`
     - * ``platform_whitelist`` => ``platform_allow``
       * ``sanitycheck`` => ``twister``
     -

.. _Inclusive Language Label: https://github.com/zephyrproject-rtos/zephyr/issues?q=label%3A%22Inclusive+Language%22
.. _I2C Specification: https://www.nxp.com/docs/en/user-guide/UM10204.pdf
.. _Bluetooth Appropriate Language Mapping Tables: https://btprodspecificationrefs.blob.core.windows.net/language-mapping/Appropriate_Language_Mapping_Table.pdf
.. _OSHWA Resolution to Redefine SPI Signal Names: https://www.oshwa.org/a-resolution-to-redefine-spi-signal-names/
.. _CAN in Automation Inclusive Language news post: https://www.can-cia.org/news/archive/view/?tx_news_pi1%5Bnews%5D=699&tx_news_pi1%5Bday%5D=6&tx_news_pi1%5Bmonth%5D=12&tx_news_pi1%5Byear%5D=2020&cHash=784e79eb438141179386cf7c29ed9438
.. _CAN in Automation Inclusive Language: https://can-newsletter.org/canopen/categories/


Rule A.3: Macro name collisions
===============================

Severity
--------

Required

Description
-----------

Macros with commonly used names such as  ``MIN``, ``MAX``, ``ARRAY_SIZE``, must
not be modified or protected to avoid name collisions with other
implementations. In particular, they must not be prefixed to place them in a
Zephyr-specific namespace, re-defined using ``#undef``, or conditionally
excluded from compilation using ``#ifndef``.  Instead, if a conflict arises with
an existing definition originating from a :ref:`module <modules>`, the module's
code itself needs to be modified (ideally upstream, alternatively via a change
in Zephyr's own fork).
This rule applies to Zephyr as a project in general, regardless of the time of
introduction of the macro or its current name in the tree. If a macro name is
commonly used in several other well-known open source projects then the
implementation in Zephyr should use that name. While there is a subjective and
non-measurable component to what "commonly used" means, the ultimate goal is
to offer users familiar macros.
Finally, this rule applies to inter-module name collisions as well: in that case
both modules, prior to their inclusion, should be modified to use
module-specific versions of the macro name that collides.

Rationale
---------

Zephyr is an RTOS that comes with additional functionality and dependencies in
the form of modules. Those modules are typically independent projects that may
use macro names that can conflict with other modules or with Zephyr itself.
Since, in the context of this documentation, Zephyr is considered the central or
main project, it should implement the non-namespaced versions of the
macros. Given that Zephyr uses a fork of the corresponding upstream for each
module, it is always possible to patch the macro implementation in each module
to avoid collisions.

Parasoft Codescan Tool
**********************

Parasoft Codescan is an official static code analysis tool used by the Zephyr
project. It is used to automate compliance with a range of coding and security
standards.
The tool is currently set to the MISRA-C:2012 Coding Standard because the Zephyr
:ref:`coding_guidelines` are based on that standard.
It is used together with the Coverity Scan tool to achieve the best code health
and precision in bug findings.

Violations fixing process
=========================

Step 1
  Any Zephyr Project member, company or a developer can request access
  to the Parasoft reporting centre if they wish to get involved in fixing
  violations by submitting issues.

Step 2
  A developer starts to review violations.

Step 3
  A developer submits a Github PR with the fix. Commit messages should follow
  the same guidelines as other PRs in the Zephyr project. Please add a comment
  that your fix was found by a static coding scanning tool.
  Developers should follow and refer to the Zephyr :ref:`coding_guidelines`
  as basic rules for coding. These rules are based on the MISRA-C standard.

  Below you can find an example of a recommended commit message::

     lib: os: add braces to 'if' statements

     An 'if' (expression) construct shall be followed by a compound statement.
     Add braces to improve readability and maintainability.

     Found as a coding guideline violation (Rule 15.6) by static
     coding scanning tool.

     Signed-off-by: Johnny Developer <johnny.developer@company.com>

Step 4
  If a violation is a false positive, the developer should mark it for the Codescan
  tool just like they would do for the Coverity tool.
  The developer should also add a comment to the code explaining that
  the violation raised by the static code analysis tool should be considered a
  false positive.

Step 5
  If the developer has found a real violation that the community decided to ignore,
  the developer must submit a PR with a suppression tag
  and a comment explaining why the violation has been deviated.
  The template structure of the comment and tag in the code should be::

     /* Explain why that part of the code doesn't follow the standard,
      * explain why it is a deliberate deviation from the standard.
      * Don't refer to the Parasoft tool here, just mention that static code
      * analysis tool raised a violation in the line below.
      */
     code_line_with_a_violation /* parasoft-suppress Rule ID */

  Below you can find an example of a recommended commit message::

     testsuite: suppress usage of setjmp in a testcode (rule 21.4)

     According to the Rule 21.4 the standard header file <setjmp.h> shall not
     be used. We will suppress this violation because it is in
     test code. Tag suppresses reporting of the violation for the
     line where the violation is located.
     This is a deliberate deviation.

     Found as a coding guideline violation (Rule 21.4) by static coding
     scanning tool.

     Signed-off-by: Johnny Developer <johnny.developer@company.com>

  The example below demonstrates how deviations can be suppressed in the code::

     /* Static code analysis tool can raise a violation that the standard
      * header <setjmp.h> shall not be used.
      * Since this violation is in test code, we will suppress it.
      * Deliberate deviation.
      */
     #include <setjmp.h> /* parasoft-suppress MISRAC2012-RULE_21_4-a MISRAC2012-RULE_21_4-b */

  This variant above suppresses item ``MISRAC2012-RULE_21_4-a`` and ``MISRAC2012-RULE_21_4-b``
  on the line with "setjump" header include. You can add as many rules to suppress you want -
  just make sure to keep the Parasoft tag on one line and separate rules with a space.
  To read more about suppressing findings in the Parasoft tool, refer to the
  official Parasoft `documentation`_

  .. _documentation: https://docs.parasoft.com/display/CPPTEST1031/Suppressing+Findings

Step 6
  After a PR is submitted, the developer should add the ``Coding guidelines``
  and ``MISRA-C`` Github labels so their PR can be easily tracked by maintainers.
  If you have any concerns about what your PR should look like, you can search
  on Github using those tags and refer to similar PRs that have already been merged.

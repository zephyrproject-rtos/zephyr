.. _coding_guidelines:

Coding Guidelines
#################


Main rules
**********

The coding guideline rules are based on MISRA-C 2012 and are a **subset** of MISRA-C.
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
    :widths: 12 45 15 14 12

    * -  Zephyr rule
      -  Description
      -  MISRA-C 2012 rule
      -  MISRA-C severity
      -  CERT C reference

         .. _MisraC_Dir_1_1:
    * -  1
      -  Any implementation-defined behaviour on which the output of the program depends shall be documented and understood
      -  `Dir 1.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_01_01.c>`_
      -  Required
      -  `MSC09-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC09-C.+Character+encoding%3A+Use+subset+of+ASCII+for+safety>`_

         .. _MisraC_Dir_2_1:
    * -  2
      -  All source files shall compile without any compilation errors
      -  `Dir 2.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_02_01.c>`_
      -  Required
      -  N/A

         .. _MisraC_Dir_3_1:
    * -  3
      -  All code shall be traceable to documented requirements
      -  `Dir 3.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_03_01.c>`_
      -  Required
      -  N/A

         .. _MisraC_Dir_4_1:
    * -  4
      -  Run-time failures shall be minimized
      -  `Dir 4.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_01.c>`_
      -  Required
      -  N/A

         .. _MisraC_Dir_4_2:
    * -  5
      -  All usage of assembly language should be documented
      -  `Dir 4.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_02.c>`_
      -  Advisory
      -  N/A

         .. _MisraC_Dir_4_4:
    * -  6
      -  Sections of code should not be “commented out”
      -  `Dir 4.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_04.c>`_
      -  Advisory
      -  `MSC04-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC04-C.+Use+comments+consistently+and+in+a+readable+fashion>`_

         .. _MisraC_Dir_4_5:
    * -  7
      -  Identifiers in the same name space with overlapping visibility should be typographically unambiguous
      -  `Dir 4.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_05.c>`_
      -  Advisory
      -  `DCL02-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL02-C.+Use+visually+distinct+identifiers>`_

         .. _MisraC_Dir_4_6:
    * -  8
      -  typedefs that indicate size and signedness should be used in place of the basic numerical types
      -  `Dir 4.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_06.c>`_
      -  Advisory
      -  N/A

         .. _MisraC_Dir_4_7:
    * -  9
      -  If a function returns error information, then that error information shall be tested
      -  `Dir 4.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_07.c>`_
      -  Required
      -  N/A

         .. _MisraC_Dir_4_8:
    * -  10
      -  If a pointer to a structure or union is never dereferenced within a translation unit, then the implementation of the object should be hidden
      -  | `Dir 4.8 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_08_1.c>`_
         | `Dir 4.8 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_08_2.c>`_
      -  Advisory
      -  `DCL12-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL12-C.+Implement+abstract+data+types+using+opaque+types>`_

         .. _MisraC_Dir_4_9:
    * -  11
      -  A function should be used in preference to a function-like macro where they are interchangeable
      -  `Dir 4.9 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_09.c>`_
      -  Advisory
      -  `PRE00-C <https://wiki.sei.cmu.edu/confluence/display/c/PRE00-C.+Prefer+inline+or+static+functions+to+function-like+macros>`_

         .. _MisraC_Dir_4_10:
    * -  12
      -  Precautions shall be taken in order to prevent the contents of a header file being included more than once
      -  `Dir 4.10 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_10.c>`_
      -  Required
      -  `PRE06-C <https://wiki.sei.cmu.edu/confluence/display/c/PRE06-C.+Enclose+header+files+in+an+include+guard>`_

         .. _MisraC_Dir_4_11:
    * -  13
      -  The validity of values passed to library functions shall be checked
      -  `Dir 4.11 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_11.c>`_
      -  Required
      -  N/A

         .. _MisraC_Dir_4_12:
    * - 14
      -  Dynamic memory allocation shall not be used
      -  `Dir 4.12 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_12.c>`_
      -  Required
      -  `STR01-C <https://wiki.sei.cmu.edu/confluence/display/c/STR01-C.+Adopt+and+implement+a+consistent+plan+for+managing+strings>`_

         .. _MisraC_Dir_4_13:
    * -  15
      -  Functions which are designed to provide operations on a resource should be called in an appropriate sequence
      -  `Dir 4.13 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_13.c>`_
      -  Advisory
      -  N/A

         .. _MisraC_Dir_4_14:
    * -  16
      -  The validity of values received from external sources shall be checked
      -  `Dir 4.14 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/D_04_14.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_1_2:
    * -  17
      -  Language extensions should not be used
      -  `Rule 1.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_01_02.c>`_
      -  Advisory
      -  `MSC04-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC04-C.+Use+comments+consistently+and+in+a+readable+fashion>`_

         .. _MisraC_Rule_1_3:
    * -  18
      -  There shall be no occurrence of undefined or critical unspecified behaviour
      -  `Rule 1.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_01_03.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_2_1:
    * -  19
      -  A project shall not contain unreachable code
      -  | `Rule 2.1 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_02_01_1.c>`_
         | `Rule 2.1 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_02_01_2.c>`_
      -  Required
      -  `MSC07-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC07-C.+Detect+and+remove+dead+code>`_

         .. _MisraC_Rule_2_2:
    * -  20
      -  There shall be no dead code
      -  `Rule 2.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_02_02.c>`_
      -  Required
      -  `MSC12-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC12-C.+Detect+and+remove+code+that+has+no+effect+or+is+never+executed>`_

         .. _MisraC_Rule_2_3:
    * -  21
      -  A project should not contain unused type declarations
      -  `Rule 2.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_02_03.c>`_
      -  Advisory
      -  N/A

         .. _MisraC_Rule_2_6:
    * -  22
      -  A function should not contain unused label declarations
      -  `Rule 2.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_02_06.c>`_
      -  Advisory
      -  N/A

         .. _MisraC_Rule_2_7:
    * -  23
      -  There should be no unused parameters in functions
      -  `Rule 2.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_02_07.c>`_
      -  Advisory
      -  N/A

         .. _MisraC_Rule_3_1:
    * -  24
      -  The character sequences /* and // shall not be used within a comment
      -  `Rule 3.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_03_01.c>`_
      -  Required
      -  `MSC04-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC04-C.+Use+comments+consistently+and+in+a+readable+fashion>`_

         .. _MisraC_Rule_3_2:
    * -  25
      -  Line-splicing shall not be used in // comments
      -  `Rule 3.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_03_02.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_4_1:
    * -  26
      -  Octal and hexadecimal escape sequences shall be terminated
      -  `Rule 4.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_04_01.c>`_
      -  Required
      -  `MSC09-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC09-C.+Character+encoding%3A+Use+subset+of+ASCII+for+safety>`_

         .. _MisraC_Rule_4_2:
    * -  27
      -  Trigraphs should not be used
      -  `Rule 4.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_04_02.c>`_
      -  Advisory
      -  `PRE07-C <https://wiki.sei.cmu.edu/confluence/display/c/PRE07-C.+Avoid+using+repeated+question+marks>`_

         .. _MisraC_Rule_5_1:
    * -  28
      -  External identifiers shall be distinct
      -  | `Rule 5.1 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_01_1.c>`_
         | `Rule 5.1 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_01_2.c>`_
      -  Required
      -  `DCL23-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL23-C.+Guarantee+that+mutually+visible+identifiers+are+unique>`_

         .. _MisraC_Rule_5_2:
    * -  29
      -  Identifiers declared in the same scope and name space shall be distinct
      -  `Rule 5.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_02.c>`_
      -  Required
      -  `DCL23-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL23-C.+Guarantee+that+mutually+visible+identifiers+are+unique>`_

         .. _MisraC_Rule_5_3:
    * -  30
      -  An identifier declared in an inner scope shall not hide an identifier declared in an outer scope
      -  `Rule 5.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_03.c>`_
      -  Required
      -  `DCL23-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL23-C.+Guarantee+that+mutually+visible+identifiers+are+unique>`_

         .. _MisraC_Rule_5_4:
    * -  31
      -  Macro identifiers shall be distinct
      -  `Rule 5.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_04.c>`_
      -  Required
      -  `DCL23-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL23-C.+Guarantee+that+mutually+visible+identifiers+are+unique>`_

         .. _MisraC_Rule_5_5:
    * -  32
      -  Identifiers shall be distinct from macro names
      -  `Rule 5.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_05.c>`_
      -  Required
      -  `DCL23-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL23-C.+Guarantee+that+mutually+visible+identifiers+are+unique>`_

         .. _MisraC_Rule_5_6:
    * -  33
      -  A typedef name shall be a unique identifier
      -  `Rule 5.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_06.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_5_7:
    * -  34
      -  A tag name shall be a unique identifier
      -  `Rule 5.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_07.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_5_8:
    * -  35
      -  Identifiers that define objects or functions with external linkage shall be unique
      -  | `Rule 5.8 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_08_1.c>`_
         | `Rule 5.8 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_08_2.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_5_9:
    * -  36
      -  Identifiers that define objects or functions with internal linkage should be unique
      -  | `Rule 5.9 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_09_1.c>`_
         | `Rule 5.9 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_05_09_2.c>`_
      -  Advisory
      -  N/A

         .. _MisraC_Rule_6_1:
    * -  37
      -  Bit-fields shall only be declared with an appropriate type
      -  `Rule 6.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_06_01.c>`_
      -  Required
      -  `INT14-C <https://wiki.sei.cmu.edu/confluence/display/c/INT14-C.+Avoid+performing+bitwise+and+arithmetic+operations+on+the+same+data>`_

         .. _MisraC_Rule_6_2:
    * -  38
      -  Single-bit named bit fields shall not be of a signed type
      -  `Rule 6.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_06_02.c>`_
      -  Required
      -  `INT14-C <https://wiki.sei.cmu.edu/confluence/display/c/INT14-C.+Avoid+performing+bitwise+and+arithmetic+operations+on+the+same+data>`_

         .. _MisraC_Rule_7_1:
    * -  39
      -  Octal constants shall not be used
      -  `Rule 7.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_07_01.c>`_
      -  Required
      -  `DCL18-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL18-C.+Do+not+begin+integer+constants+with+0+when+specifying+a+decimal+value>`_

         .. _MisraC_Rule_7_2:
    * -  40
      -  A u or U suffix shall be applied to all integer constants that are represented in an unsigned type
      -  `Rule 7.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_07_02.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_7_3:
    * -  41
      -  The lowercase character l shall not be used in a literal suffix
      -  `Rule 7.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_07_03.c>`_
      -  Required
      -  `DCL16-C <https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=87152241>`_

         .. _MisraC_Rule_7_4:
    * -  42
      -  A string literal shall not be assigned to an object unless the objects type is pointer to const-qualified char
      -  `Rule 7.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_07_04.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_8_1:
    * -  43
      -  Types shall be explicitly specified
      -  `Rule 8.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_01.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_8_2:
    * -  44
      -  Function types shall be in prototype form with named parameters
      -  `Rule 8.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_02.c>`_
      -  Required
      -  `DCL20-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL20-C.+Explicitly+specify+void+when+a+function+accepts+no+arguments>`_

         .. _MisraC_Rule_8_3:
    * -  45
      -  All declarations of an object or function shall use the same names and type qualifiers
      -  `Rule 8.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_03.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_8_4:
    * -  46
      -  A compatible declaration shall be visible when an object or function with external linkage is defined
      -  `Rule 8.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_04.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_8_5:
    * -  47
      -  An external object or function shall be declared once in one and only one file
      -  | `Rule 8.5 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_05_1.c>`_
         | `Rule 8.5 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_05_2.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_8_6:
    * -  48
      -  An identifier with external linkage shall have exactly one external definition
      -  | `Rule 8.6 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_06_1.c>`_
         | `Rule 8.6 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_06_2.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_8_8:
    * -  49
      -  The static storage class specifier shall be used in all declarations of objects and functions that have internal linkage
      -  `Rule 8.8 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_08.c>`_
      -  Required
      -  `DCL15-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL15-C.+Declare+file-scope+objects+or+functions+that+do+not+need+external+linkage+as+static>`_

         .. _MisraC_Rule_8_9:
    * -  50
      -  An object should be defined at block scope if its identifier only appears in a single function
      -  `Rule 8.9 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_09.c>`_
      -  Advisory
      -  `DCL19-C <https://wiki.sei.cmu.edu/confluence/display/c/DCL19-C.+Minimize+the+scope+of+variables+and+functions>`_

         .. _MisraC_Rule_8_10:
    * -  51
      -  An inline function shall be declared with the static storage class
      -  `Rule 8.10 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_10.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_8_12:
    * -  52
      -  Within an enumerator list, the value of an implicitly-specified enumeration constant shall be unique
      -  `Rule 8.12 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_12.c>`_
      -  Required
      -  `INT09-C <https://wiki.sei.cmu.edu/confluence/display/c/INT09-C.+Ensure+enumeration+constants+map+to+unique+values>`_

         .. _MisraC_Rule_8_14:
    * -  53
      -  The restrict type qualifier shall not be used
      -  `Rule 8.14 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_08_14.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_9_1:
    * -  54
      -  The value of an object with automatic storage duration shall not be read before it has been set
      -  `Rule 9.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_09_01.c>`_
      -  Mandatory
      -  N/A

         .. _MisraC_Rule_9_2:
    * -  55
      -  The initializer for an aggregate or union shall be enclosed in braces
      -  `Rule 9.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_09_02.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_9_3:
    * -  56
      -  Arrays shall not be partially initialized
      -  `Rule 9.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_09_03.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_9_4:
    * -  57
      -  An element of an object shall not be initialized more than once
      -  `Rule 9.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_09_04.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_9_5:
    * -  58
      -  Where designated initializers are used to initialize an array object the size of the array shall be specified explicitly
      -  `Rule 9.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_09_05.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_10_1:
    * -  59
      -  Operands shall not be of an inappropriate essential type
      -  `Rule 10.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_01.c>`_
      -  Required
      -  `STR04-C <https://wiki.sei.cmu.edu/confluence/display/c/STR04-C.+Use+plain+char+for+characters+in+the+basic+character+set>`_

         .. _MisraC_Rule_10_2:
    * -  60
      -  Expressions of essentially character type shall not be used inappropriately in addition and subtraction operations
      -  `Rule 10.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_02.c>`_
      -  Required
      -  `STR04-C <https://wiki.sei.cmu.edu/confluence/display/c/STR04-C.+Use+plain+char+for+characters+in+the+basic+character+set>`_

         .. _MisraC_Rule_10_3:
    * -  61
      -  The value of an expression shall not be assigned to an object with a narrower essential type or of a different essential type category
      -  `Rule 10.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_03.c>`_
      -  Required
      -  `STR04-C <https://wiki.sei.cmu.edu/confluence/display/c/STR04-C.+Use+plain+char+for+characters+in+the+basic+character+set>`_

         .. _MisraC_Rule_10_4:
    * -  62
      -  Both operands of an operator in which the usual arithmetic conversions are performed shall have the same essential type category
      -  `Rule 10.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_04.c>`_
      -  Required
      -  `STR04-C <https://wiki.sei.cmu.edu/confluence/display/c/STR04-C.+Use+plain+char+for+characters+in+the+basic+character+set>`_

         .. _MisraC_Rule_10_5:
    * -  63
      -  The value of an expression should not be cast to an inappropriate essential type
      -  `Rule 10.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_05.c>`_
      -  Advisory
      -  N/A

         .. _MisraC_Rule_10_6:
    * -  64
      -  The value of a composite expression shall not be assigned to an object with wider essential type
      -  `Rule 10.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_06.c>`_
      -  Required
      -  `INT02-C <https://wiki.sei.cmu.edu/confluence/display/c/INT02-C.+Understand+integer+conversion+rules>`_

         .. _MisraC_Rule_10_7:
    * -  65
      -  If a composite expression is used as one operand of an operator in which the usual arithmetic conversions are performed then the other operand shall not have wider essential type
      -  `Rule 10.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_07.c>`_
      -  Required
      -  `INT02-C <https://wiki.sei.cmu.edu/confluence/display/c/INT02-C.+Understand+integer+conversion+rules>`_

         .. _MisraC_Rule_10_8:
    * -  66
      -  The value of a composite expression shall not be cast to a different essential type category or a wider essential type
      -  `Rule 10.8 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_10_08.c>`_
      -  Required
      -  `INT02-C <https://wiki.sei.cmu.edu/confluence/display/c/INT02-C.+Understand+integer+conversion+rules>`_

         .. _MisraC_Rule_11_2:
    * -  67
      -  Conversions shall not be performed between a pointer to an incomplete type and any other type
      -  `Rule 11.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_11_02.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_11_6:
    * -  68
      -  A cast shall not be performed between pointer to void and an arithmetic type
      -  `Rule 11.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_11_06.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_11_7:
    * -  69
      -  A cast shall not be performed between pointer to object and a noninteger arithmetic type
      -  `Rule 11.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_11_07.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_11_8:
    * -  70
      -  A cast shall not remove any const or volatile qualification from the type pointed to by a pointer
      -  `Rule 11.8 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_11_08.c>`_
      -  Required
      -  `EXP05-C <https://wiki.sei.cmu.edu/confluence/display/c/EXP05-C.+Do+not+cast+away+a+const+qualification>`_

         .. _MisraC_Rule_11_9:
    * -  71
      -  The macro NULL shall be the only permitted form of integer null pointer constant
      -  `Rule 11.9 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_11_09.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_12_1:
    * -  72
      -  The precedence of operators within expressions should be made explicit
      -  `Rule 12.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_12_01.c>`_
      -  Advisory
      -  `EXP00-C <https://wiki.sei.cmu.edu/confluence/display/c/EXP00-C.+Use+parentheses+for+precedence+of+operation>`_

         .. _MisraC_Rule_12_2:
    * -  73
      -  The right hand operand of a shift operator shall lie in the range zero to one less than the width in bits of the essential type of the left hand operand
      -  `Rule 12.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_12_02.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_12_4:
    * -  74
      -  Evaluation of constant expressions should not lead to unsigned integer wrap-around
      -  `Rule 12.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_12_04.c>`_
      -  Advisory
      -  N/A

         .. _MisraC_Rule_12_5:
    * -  75
      -  The sizeof operator shall not have an operand which is a function parameter declared as “array of type”
      -  `Rule 12.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_12_05.c>`_
      -  Mandatory
      -  N/A

         .. _MisraC_Rule_13_1:
    * -  76
      -  Initializer lists shall not contain persistent side effects
      -  | `Rule 13.1 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_13_01_1.c>`_
         | `Rule 13.1 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_13_01_2.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_13_2:
    * -  77
      -  The value of an expression and its persistent side effects shall be the same under all permitted evaluation orders
      -  `Rule 13.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_13_02.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_13_3:
    * -  78
      -  A full expression containing an increment (++) or decrement (--) operator should have no other potential side effects other than that caused by the increment or decrement operator
      -  `Rule 13.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_13_03.c>`_
      -  Advisory
      -  N/A

         .. _MisraC_Rule_13_4:
    * -  79
      -  The result of an assignment operator should not be used
      -  `Rule 13.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_13_04.c>`_
      -  Advisory
      -  N/A

         .. _MisraC_Rule_13_5:
    * -  80
      -  The right hand operand of a logical && or || operator shall not contain persistent side effects
      -  | `Rule 13.5 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_13_05_1.c>`_
         | `Rule 13.5 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_13_05_2.c>`_
      -  Required
      -  `EXP10-C <https://wiki.sei.cmu.edu/confluence/display/c/EXP10-C.+Do+not+depend+on+the+order+of+evaluation+of+subexpressions+or+the+order+in+which+side+effects+take+place>`_

         .. _MisraC_Rule_13_6:
    * -  81
      -  The operand of the sizeof operator shall not contain any expression which has potential side effects
      -  `Rule 13.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_13_06.c>`_
      -  Mandatory
      -  N/A

         .. _MisraC_Rule_14_1:
    * -  82
      -  A loop counter shall not have essentially floating type
      -  `Rule 14.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_14_01.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_14_2:
    * -  83
      -  A for loop shall be well-formed
      -  `Rule 14.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_14_02.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_14_3:
    * -  84
      -  Controlling expressions shall not be invariant
      -  `Rule 14.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_14_03.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_14_4:
    * -  85
      -  The controlling expression of an if statement and the controlling expression of an iteration-statement shall have essentially Boolean type
      -  `Rule 14.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_14_04.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_15_2:
    * -  86
      -  The goto statement shall jump to a label declared later in the same function
      -  `Rule 15.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_15_02.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_15_3:
    * -  87
      -  Any label referenced by a goto statement shall be declared in the same block, or in any block enclosing the goto statement
      -  `Rule 15.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_15_03.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_15_6:
    * -  88
      -  The body of an iteration-statement or a selection-statement shall be a compound-statement
      -  `Rule 15.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_15_06.c>`_
      -  Required
      -  `EXP19-C <https://wiki.sei.cmu.edu/confluence/display/c/EXP19-C.+Use+braces+for+the+body+of+an+if%2C+for%2C+or+while+statement>`_

         .. _MisraC_Rule_15_7:
    * -  89
      -  All if else if constructs shall be terminated with an else statement
      -  `Rule 15.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_15_07.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_16_1:
    * -  90
      -  All switch statements shall be well-formed
      -  `Rule 16.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_16_01.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_16_2:
    * -  91
      -  A switch label shall only be used when the most closely-enclosing compound statement is the body of a switch statement
      -  `Rule 16.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_16_02.c>`_
      -  Required
      -  `MSC20-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC20-C.+Do+not+use+a+switch+statement+to+transfer+control+into+a+complex+block>`_

         .. _MisraC_Rule_16_3:
    * -  92
      -  An unconditional break statement shall terminate every switch-clause
      -  `Rule 16.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_16_03.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_16_4:
    * -  93
      -  Every switch statement shall have a default label
      -  `Rule 16.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_16_04.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_16_5:
    * -  94
      -  A default label shall appear as either the first or the last switch label of a switch statement
      -  `Rule 16.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_16_05.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_16_6:
    * -  95
      -  Every switch statement shall have at least two switch-clauses
      -  `Rule 16.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_16_06.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_16_7:
    * -  96
      -  A switch-expression shall not have essentially Boolean type
      -  `Rule 16.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_16_07.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_17_1:
    * -  97
      -  The features of <stdarg.h> shall not be used
      -  `Rule 17.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_17_01.c>`_
      -  Required
      -  `ERR00-C <https://wiki.sei.cmu.edu/confluence/display/c/ERR00-C.+Adopt+and+implement+a+consistent+and+comprehensive+error-handling+policy>`_

         .. _MisraC_Rule_17_2:
    * -  98
      -  Functions shall not call themselves, either directly or indirectly
      -  `Rule 17.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_17_02.c>`_
      -  Required
      -  `MEM05-C <https://wiki.sei.cmu.edu/confluence/display/c/MEM05-C.+Avoid+large+stack+allocations>`_

         .. _MisraC_Rule_17_3:
    * -  99
      -  A function shall not be declared implicitly
      -  `Rule 17.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_17_03.c>`_
      -  Mandatory
      -  N/A

         .. _MisraC_Rule_17_4:
    * -  100
      -  All exit paths from a function with non-void return type shall have an explicit return statement with an expression
      -  `Rule 17.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_17_04.c>`_
      -  Mandatory
      -  N/A

         .. _MisraC_Rule_17_5:
    * -  101
      -  The function argument corresponding to a parameter declared to have an array type shall have an appropriate number of elements
      -  `Rule 17.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_17_05.c>`_
      -  Advisory
      -  N/A

         .. _MisraC_Rule_17_6:
    * -  102
      -  The declaration of an array parameter shall not contain the static keyword between the [ ]
      -  `Rule 17.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_17_06.c>`_
      -  Mandatory
      -  N/A

         .. _MisraC_Rule_17_7:
    * -  103
      -  The value returned by a function having non-void return type shall be used
      -  `Rule 17.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_17_07.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_18_1:
    * -  104
      -  A pointer resulting from arithmetic on a pointer operand shall address an element of the same array as that pointer operand
      -  `Rule 18.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_18_01.c>`_
      -  Required
      -  `EXP08-C <https://wiki.sei.cmu.edu/confluence/display/c/EXP08-C.+Ensure+pointer+arithmetic+is+used+correctly>`_

         .. _MisraC_Rule_18_2:
    * -  105
      -  Subtraction between pointers shall only be applied to pointers that address elements of the same array
      -  `Rule 18.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_18_02.c>`_
      -  Required
      -  `EXP08-C <https://wiki.sei.cmu.edu/confluence/display/c/EXP08-C.+Ensure+pointer+arithmetic+is+used+correctly>`_

         .. _MisraC_Rule_18_3:
    * -  106
      -  The relational operators >, >=, < and <= shall not be applied to objects of pointer type except where they point into the same object
      -  `Rule 18.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_18_03.c>`_
      -  Required
      -  `EXP08-C <https://wiki.sei.cmu.edu/confluence/display/c/EXP08-C.+Ensure+pointer+arithmetic+is+used+correctly>`_

         .. _MisraC_Rule_18_5:
    * -  107
      -  Declarations should contain no more than two levels of pointer nesting
      -  `Rule 18.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_18_05.c>`_
      -  Advisory
      -  N/A

         .. _MisraC_Rule_18_6:
    * -  108
      -  The address of an object with automatic storage shall not be copied to another object that persists after the first object has ceased to exist
      -  | `Rule 18.6 example 1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_18_06_1.c>`_
         | `Rule 18.6 example 2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_18_06_2.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_18_8:
    * -  109
      -  Variable-length array types shall not be used
      -  `Rule 18.8 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_18_08.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_19_1:
    * -  110
      -  An object shall not be assigned or copied to an overlapping object
      -  `Rule 19.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_19_01.c>`_
      -  Mandatory
      -  N/A

         .. _MisraC_Rule_20_2:
    * -  111
      -  The ', or \ characters and the /* or // character sequences shall not occur in a header file name"
      -  `Rule 20.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_02.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_20_3:
    * -  112
      -  The #include directive shall be followed by either a <filename> or "filename" sequence
      -  `Rule 20.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_03.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_20_4:
    * -  113
      -  A macro shall not be defined with the same name as a keyword
      -  `Rule 20.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_04.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_20_7:
    * -  114
      -  Expressions resulting from the expansion of macro parameters shall be enclosed in parentheses
      -  `Rule 20.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_07.c>`_
      -  Required
      -  `PRE01-C <https://wiki.sei.cmu.edu/confluence/display/c/PRE01-C.+Use+parentheses+within+macros+around+parameter+names>`_

         .. _MisraC_Rule_20_8:
    * -  115
      -  The controlling expression of a #if or #elif preprocessing directive shall evaluate to 0 or 1
      -  `Rule 20.8 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_08.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_20_9:
    * -  116
      -  All identifiers used in the controlling expression of #if or #elif preprocessing directives shall be #defined before evaluation
      -  `Rule 20.9 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_09.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_20_11:
    * -  117
      -  A macro parameter immediately following a # operator shall not immediately be followed by a ## operator
      -  `Rule 20.11 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_11.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_20_12:
    * -  118
      -  A macro parameter used as an operand to the # or ## operators, which is itself subject to further macro replacement, shall only be used as an operand to these operators
      -  `Rule 20.12 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_12.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_20_13:
    * -  119
      -  A line whose first token is # shall be a valid preprocessing directive
      -  `Rule 20.13 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_13.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_20_14:
    * -  120
      -  All #else, #elif and #endif preprocessor directives shall reside in the same file as the #if, #ifdef or #ifndef directive to which they are related
      -  `Rule 20.14 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_20_14.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_21_1:
    * -  121
      -  #define and #undef shall not be used on a reserved identifier or reserved macro name
      -  `Rule 21.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_01.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_21_2:
    * -  122
      -  A reserved identifier or macro name shall not be declared
      -  `Rule 21.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_02.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_21_3:
    * -  123
      -  The memory allocation and deallocation functions of <stdlib.h> shall not be used
      -  `Rule 21.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_03.c>`_
      -  Required
      -  `MSC24-C <https://wiki.sei.cmu.edu/confluence/display/c/MSC24-C.+Do+not+use+deprecated+or+obsolescent+functions>`_

         .. _MisraC_Rule_21_4:
    * -  124
      -  The standard header file <setjmp.h> shall not be used
      -  `Rule 21.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_04.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_21_6:
    * -  125
      -  The Standard Library input/output functions shall not be used
      -  `Rule 21.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_06.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_21_7:
    * -  126
      -  The atof, atoi, atol and atoll functions of <stdlib.h> shall not be used
      -  `Rule 21.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_07.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_21_9:
    * -  127
      -  The library functions bsearch and qsort of <stdlib.h> shall not be used
      -  `Rule 21.9 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_09.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_21_11:
    * -  128
      -  The standard header file <tgmath.h> shall not be used
      -  `Rule 21.11 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_11.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_21_12:
    * -  129
      -  The exception handling features of <fenv.h> should not be used
      -  `Rule 21.12 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_12.c>`_
      -  Advisory
      -  N/A

         .. _MisraC_Rule_21_13:
    * -  130
      -  Any value passed to a function in <ctype.h> shall be representable as an unsigned char or be the value EO
      -  `Rule 21.13 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_13.c>`_
      -  Mandatory
      -  N/A

         .. _MisraC_Rule_21_14:
    * -  131
      -  The Standard Library function memcmp shall not be used to compare null terminated strings
      -  `Rule 21.14 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_14.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_21_15:
    * -  132
      -  The pointer arguments to the Standard Library functions memcpy, memmove and memcmp shall be pointers to qualified or unqualified versions of compatible types
      -  `Rule 21.15 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_15.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_21_16:
    * -  133
      -  The pointer arguments to the Standard Library function memcmp shall point to either a pointer type, an essentially signed type, an essentially unsigned type, an essentially Boolean type or an essentially enum type
      -  `Rule 21.16 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_16.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_21_17:
    * -  134
      -  Use of the string handling functions from <string.h> shall not result in accesses beyond the bounds of the objects referenced by their pointer parameters
      -  `Rule 21.17 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_17.c>`_
      -  Mandatory
      -  N/A

         .. _MisraC_Rule_21_18:
    * -  135
      -  The size_t argument passed to any function in <string.h> shall have an appropriate value
      -  `Rule 21.18 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_18.c>`_
      -  Mandatory
      -  N/A

         .. _MisraC_Rule_21_19:
    * -  136
      -  The pointers returned by the Standard Library functions localeconv, getenv, setlocale or, strerror shall only be used as if they have pointer to const-qualified type
      -  `Rule 21.19 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_19.c>`_
      -  Mandatory
      -  N/A

         .. _MisraC_Rule_21_20:
    * -  137
      -  The pointer returned by the Standard Library functions asctime, ctime, gmtime, localtime, localeconv, getenv, setlocale or strerror shall not be used following a subsequent call to the same function
      -  `Rule 21.20 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_21_20.c>`_
      -  Mandatory
      -  N/A

         .. _MisraC_Rule_22_1:
    * -  138
      -  All resources obtained dynamically by means of Standard Library functions shall be explicitly released
      -  `Rule 22.1 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_01.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_22_2:
    * -  139
      -  A block of memory shall only be freed if it was allocated by means of a Standard Library function
      -  `Rule 22.2 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_02.c>`_
      -  Mandatory
      -  N/A

         .. _MisraC_Rule_22_3:
    * -  140
      -  The same file shall not be open for read and write access at the same time on different streams
      -  `Rule 22.3 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_03.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_22_4:
    * -  141
      -  There shall be no attempt to write to a stream which has been opened as read-only
      -  `Rule 22.4 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_04.c>`_
      -  Mandatory
      -  N/A

         .. _MisraC_Rule_22_5:
    * -  142
      -  A pointer to a FILE object shall not be dereferenced
      -  `Rule 22.5 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_05.c>`_
      -  Mandatory
      -  N/A

         .. _MisraC_Rule_22_6:
    * -  143
      -  The value of a pointer to a FILE shall not be used after the associated stream has been closed
      -  `Rule 22.6 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_06.c>`_
      -  Mandatory
      -  N/A

         .. _MisraC_Rule_22_7:
    * -  144
      -  The macro EOF shall only be compared with the unmodified return value from any Standard Library function capable of returning EOF
      -  `Rule 22.7 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_07.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_22_8:
    * -  145
      -  The value of errno shall be set to zero prior to a call to an errno-setting-function
      -  `Rule 22.8 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_08.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_22_9:
    * -  146
      -  The value of errno shall be tested against zero after calling an errno-setting-function
      -  `Rule 22.9 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_09.c>`_
      -  Required
      -  N/A

         .. _MisraC_Rule_22_10:
    * -  147
      -  The value of errno shall only be tested when the last function to be called was an errno-setting-function
      -  `Rule 22.10 <https://gitlab.com/MISRA/MISRA-C/MISRA-C-2012/Example-Suite/-/blob/master/R_22_10.c>`_
      -  Required
      -  N/A

Additional rules
****************

Rule A.1: Conditional Compilation
=================================
Severity
  Required

Description
  Do not conditionally compile function declarations in header files. Do not
  conditionally compile structure declarations in header files. You may
  conditionally exclude fields within structure definitions to avoid wasting
  memory when the feature they support is not enabled.

Rationale
  Excluding declarations from the header based on compile-time options may prevent
  their documentation from being generated. Their absence also prevents use of
  ``if (IS_ENABLED(CONFIG_FOO)) {}`` as an alternative to preprocessor
  conditionals when the code path should change based on the selected options.

.. _coding_guideline_inclusive_language:

Rule A.2: Inclusive Language
============================
Severity
  Required

Description
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
  Offensive terms do not create an inclusive community environment and therefore
  violate the Zephyr Project `Code of Conduct`_. This coding rule was inspired by
  a similar rule in `Linux`_.

  .. _Code of Conduct: https://github.com/zephyrproject-rtos/zephyr/blob/main/CODE_OF_CONDUCT.md
  .. _Linux: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=49decddd39e5f6132ccd7d9fdc3d7c470b0061bb

Status
  Related GitHub Issues and Pull Requests are tagged with the `Inclusive Language Label`_.

  .. list-table::
     :header-rows: 1

     * - Area
       - Selected Replacements
       - Status

     * - :ref:`Bluetooth <bluetooth_api>`
       - See `Bluetooth Appropriate Language Mapping Tables`_
       -

     * - CAN
       - This `CAN in Automation Inclusive Language news post`_ has a list of general
         recommendations. See `CAN in Automation Inclusive Language`_ for terms to
         be used in specification document updates.
       -

     * - eSPI
       - * ``master / slave`` => ``controller / target``
       - Refer to `eSPI Specification`_ for new terminology

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
  .. _Bluetooth Appropriate Language Mapping Tables: https://specificationrefs.bluetooth.com/language-mapping/Appropriate_Language_Mapping_Table.pdf
  .. _OSHWA Resolution to Redefine SPI Signal Names: https://www.oshwa.org/a-resolution-to-redefine-spi-signal-names/
  .. _CAN in Automation Inclusive Language news post: https://www.can-cia.org/news/archive/view/?tx_news_pi1%5Bnews%5D=699&tx_news_pi1%5Bday%5D=6&tx_news_pi1%5Bmonth%5D=12&tx_news_pi1%5Byear%5D=2020&cHash=784e79eb438141179386cf7c29ed9438
  .. _CAN in Automation Inclusive Language: https://can-newsletter.org/canopen/categories/
  .. _eSPI Specification: https://downloadmirror.intel.com/27055/327432%20espi_base_specification%20R1-5.pdf


.. _coding_guideline_libc_usage_restrictions_in_zephyr_kernel:

Rule A.3: Macro name collisions
===============================
Severity
  Required

Description
  Macros with commonly used names such as ``MIN``, ``MAX``, ``ARRAY_SIZE``, must not be modified or
  protected to avoid name collisions with other implementations. In particular, they must not be
  prefixed to place them in a Zephyr-specific namespace, re-defined using ``#undef``, or
  conditionally excluded from compilation using ``#ifndef``. Instead, if a conflict arises with an
  existing definition originating from a :ref:`module <modules>`, the module's code itself needs to
  be modified (ideally upstream, alternatively via a change in Zephyr's own fork).

  This rule applies to Zephyr as a project in general, regardless of the time of introduction of the
  macro or its current name in the tree. If a macro name is commonly used in several other well-known
  open source projects then the implementation in Zephyr should use that name. While there is a
  subjective and non-measurable component to what "commonly used" means, the ultimate goal is to offer
  users familiar macros.

  Finally, this rule applies to inter-module name collisions as well: in that case both modules, prior
  to their inclusion, should be modified to use module-specific versions of the macro name that
  collides.

Rationale
  Zephyr is an RTOS that comes with additional functionality and dependencies in the form of modules.
  Those modules are typically independent projects that may use macro names that can conflict with
  other modules or with Zephyr itself. Since, in the context of this documentation, Zephyr is
  considered the central or main project, it should implement the non-namespaced versions of the
  macros. Given that Zephyr uses a fork of the corresponding upstream for each module, it is always
  possible to patch the macro implementation in each module to avoid collisions.

Rule A.4: C Standard Library Usage Restrictions in Zephyr Kernel
================================================================
Severity
  Required

Description
  The use of the C standard library functions and macros in the Zephyr kernel
  shall be limited to the following functions and macros from the ISO/IEC
  9899:2011 standard, also known as C11, and their extensions:

  .. csv-table:: List of allowed libc functions and macros in the Zephyr kernel
     :header: Function,Source
     :widths: auto

     abort(),ISO/IEC 9899:2011
     abs(),ISO/IEC 9899:2011
     aligned_alloc(),ISO/IEC 9899:2011
     assert(),ISO/IEC 9899:2011
     atoi(),ISO/IEC 9899:2011
     bsearch(),ISO/IEC 9899:2011
     calloc(),ISO/IEC 9899:2011
     exit(),ISO/IEC 9899:2011
     fprintf(),ISO/IEC 9899:2011
     fputc(),ISO/IEC 9899:2011
     fputs(),ISO/IEC 9899:2011
     free(),ISO/IEC 9899:2011
     fwrite(),ISO/IEC 9899:2011
     gmtime(),ISO/IEC 9899:2011
     isalnum(),ISO/IEC 9899:2011
     isalpha(),ISO/IEC 9899:2011
     iscntrl(),ISO/IEC 9899:2011
     isdigit(),ISO/IEC 9899:2011
     isgraph(),ISO/IEC 9899:2011
     isprint(),ISO/IEC 9899:2011
     isspace(),ISO/IEC 9899:2011
     isupper(),ISO/IEC 9899:2011
     isxdigit(),ISO/IEC 9899:2011
     labs(),ISO/IEC 9899:2011
     llabs(),ISO/IEC 9899:2011
     malloc(),ISO/IEC 9899:2011
     memchr(),ISO/IEC 9899:2011
     memcmp(),ISO/IEC 9899:2011
     memcpy(),ISO/IEC 9899:2011
     memmove(),ISO/IEC 9899:2011
     memset(),ISO/IEC 9899:2011
     perror(),ISO/IEC 9899:2011
     printf(),ISO/IEC 9899:2011
     putc(),ISO/IEC 9899:2011
     putchar(),ISO/IEC 9899:2011
     puts(),ISO/IEC 9899:2011
     qsort(),ISO/IEC 9899:2011
     rand(),ISO/IEC 9899:2011
     realloc(),ISO/IEC 9899:2011
     snprintf(),ISO/IEC 9899:2011
     sprintf(),ISO/IEC 9899:2011
     sqrt(),ISO/IEC 9899:2011
     sqrtf(),ISO/IEC 9899:2011
     srand(),ISO/IEC 9899:2011
     strcat(),ISO/IEC 9899:2011
     strchr(),ISO/IEC 9899:2011
     strcmp(),ISO/IEC 9899:2011
     strcpy(),ISO/IEC 9899:2011
     strcspn(),ISO/IEC 9899:2011
     strerror(),ISO/IEC 9899:2011
     strlen(),ISO/IEC 9899:2011
     strncat(),ISO/IEC 9899:2011
     strncmp(),ISO/IEC 9899:2011
     strncpy(),ISO/IEC 9899:2011
     `strnlen()`_,POSIX.1-2008
     strrchr(),ISO/IEC 9899:2011
     strspn(),ISO/IEC 9899:2011
     strstr(),ISO/IEC 9899:2011
     strtol(),ISO/IEC 9899:2011
     strtoll(),ISO/IEC 9899:2011
     strtoul(),ISO/IEC 9899:2011
     strtoull(),ISO/IEC 9899:2011
     time(),ISO/IEC 9899:2011
     tolower(),ISO/IEC 9899:2011
     toupper(),ISO/IEC 9899:2011
     vfprintf(),ISO/IEC 9899:2011
     vprintf(),ISO/IEC 9899:2011
     vsnprintf(),ISO/IEC 9899:2011
     vsprintf(),ISO/IEC 9899:2011

  All of the functions listed above must be implemented by the
  :ref:`minimal libc <c_library_minimal>` to ensure that the Zephyr kernel can
  build with the minimal libc.

  In addition, any functions from the above list that are not part of the
  ISO/IEC 9899:2011 standard must be implemented by the
  :ref:`common libc <c_library_common>` to ensure their availability across
  multiple C standard libraries.

  Introducing new C standard library functions to the Zephyr kernel is allowed
  with justification given that the above requirements are satisfied.

  Note that the use of the functions listed above are subject to secure and safe
  coding practices and it should not be assumed that their use in the Zephyr
  kernel is unconditionally permitted by being listed in this rule.

  The "Zephyr kernel" in this context consists of the following components:

  * Kernel (:file:`kernel`)
  * OS Library (:file:`lib/os`)
  * Architecture Port (:file:`arch`)
  * Logging Subsystem (:file:`subsys/logging`)

Rationale
  Zephyr kernel must be able to build with the
  :ref:`minimal libc <c_library_minimal>`, a limited C standard library
  implementation that is part of the Zephyr RTOS and maintained by the Zephyr
  Project, to allow self-contained testing and verification of the kernel and
  core OS services.

  In order to ensure that the Zephyr kernel can build with the minimal libc, it
  is necessary to restrict the use of the C standard library functions and macros
  in the Zephyr kernel to the functions and macros that are available as part of
  the minimal libc.

Rule A.5: C Standard Library Usage Restrictions in Zephyr Codebase
==================================================================
Severity
  Required

Description
  The use of the C standard library functions and macros in the Zephyr codebase
  shall be limited to the functions, excluding the Annex K "Bounds-checking
  interfaces", from the ISO/IEC 9899:2011 standard, also known as C11, unless
  exempted by this rule.

  The "Zephyr codebase" in this context refers to all embedded source code files committed
  to the `main Zephyr repository`_, except the Zephyr kernel as defined by the
  :ref:`coding_guideline_libc_usage_restrictions_in_zephyr_kernel`.
  With embedded source code we refer to code which is meant to be executed in embedded
  targets, and therefore excludes host tooling, and code specific for the
  :ref:`native <boards_posix>` test targets.

  The following non-ISO 9899:2011, hereinafter referred to as non-standard,
  functions and macros are exempt from this rule and allowed to be used in the
  Zephyr codebase:

  .. csv-table:: List of allowed non-standard libc functions
     :header: Function,Source
     :widths: auto

     `gmtime_r()`_,POSIX.1-2001
     `strnlen()`_,POSIX.1-2008
     `strtok_r()`_,POSIX.1-2001

  All non-standard functions and macros listed above must be implemented by the
  :ref:`common libc <c_library_common>` in order to make sure that these
  functions can be made available when using a C standard library that does not
  implement these functions.

  Adding a new non-standard function from common C standard libraries to the
  above list is allowed with justification, given that the above requirement is
  satisfied. However, when there exists a standard function that is functionally
  equivalent, the standard function shall be used.

Rationale
  Some C standard libraries, such as Newlib and Picolibc, include additional
  functions and macros that are defined by the standards and de-facto standards
  that extend the ISO C standard (e.g. POSIX, Linux).

  The ISO/IEC 9899:2011 standard does not require C compiler toolchains to
  include the support for these non-standard functions, and therefore using
  these functions can lead to compatibility issues with the third-party
  toolchains that come with their own C standard libraries.

  .. _main Zephyr repository: https://github.com/zephyrproject-rtos/zephyr
  .. _gmtime_r(): https://pubs.opengroup.org/onlinepubs/9699919799/functions/gmtime_r.html
  .. _strnlen(): https://pubs.opengroup.org/onlinepubs/9699919799/functions/strlen.html
  .. _strtok_r(): https://pubs.opengroup.org/onlinepubs/9699919799/functions/strtok.html

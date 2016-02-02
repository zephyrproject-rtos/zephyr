.. _licenses:

License Guidelines
##################

The Zephyr Kernel is licensed under a BSD 3-clause license. This is a
permissive license that allows our customers to write kernel-level code
without having to open source their code. All code linked in a Zephyr Kernel
binary image must be permissively licensed. The source tree includes some GPL
-licensed build tools, but those are not linked to the final binary image,
and thus the image itself is not GPL-tainted.

Customers are able to add GPL code to their kernel. Therefore, all
permissively licensed code in the project must be GPL-compatible.

.. caution::

   Some permissive licenses are not GPL-compatible).

Acceptable Licenses
*******************

The following list shows all licenses that can be used without further
considerations:

* 2-clause BSD
* 3-clause BSD
* MIT
* ISC
* NCSA, the license for Clang and LLVM
* True public domain
* Creative Commons Zero (public domain) but not other CC licenses


Licenses that Require Caution
*****************************

The following list shows the licenses that can be used if the special
considerations included are taken into account:

* GPL
* Apache: Incompatible with GPLv2 but not GPLv3, according to the FSF.
* Mozilla: weak copy-left. It is weaker than LGPL.
* Microsoft public license
* LGPL
* OpenSSL license: permissive, but not GPL compatible.
* BSD 4-clause: not GPL compatible. It is difficult to attribute.

IANAL Disclaimer
****************

Be cautious about working with GPL code while working on permissively
licensed code.

Do not copy code or have a different project's code open while you re-
implement the same code in your project.

It is allowed to look at other project's documentation, code concepts,
comments, and git commits while implementing new code. You may contribute
code to unrelated GPL projects while working on permissively licensed code.
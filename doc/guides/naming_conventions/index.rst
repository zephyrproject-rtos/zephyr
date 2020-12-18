.. _naming_conventions:

Naming Conventions
##################

Functions
**********

#. No function shall have a name that is a keyword of any standard version of
   the C or C++ programming language. Restricted names include ``interrupt``, ``inline``,
   ``class``, ``true``, ``false``, ``public``, ``private``, ``friend``,
   ``protected``, and many others.

#. Function names are always in lowercase, with an underscore between each part of
   the name : ``k_timer_init()`` , ``k_timer_start()``.

#. If a function operates primarily on a certain type, the prefix of this function
   corresponds to the type name. For example, the k_timer_* functions work with the
   ``k_timer`` type, and k_sem_* functions go with ``k_sem``.

#. Each functions' name shall be descriptive of its purpose. Note that functions
   encapsulate the “actions” of a program and thus benefit from the use of verbs in
   their names (e.g., ``k_mutex_lock()``); this “noun-verb” word ordering is recommended.
   Alternatively, functions may be named according to the question they answer
   (e.g., ``device_is_ready()``).

#. The names of all public functions shall be prefixed with their module name and
   an underscore (e.g., ``sensor_read()``).

#. Avoid names that differ only in case, like foo and Foo. Similarly, avoid foobar
   and foo_bar. The potential for confusion is considerable.

#. Similarly, avoid names that look like each other. On many terminals and
   printers, 'l', '1' and 'I' look quite similar. A variable named 'l' is
   particularly bad because it looks so much like the constant '1'.

#. #define constants shall be in all CAPS, except when required to
   conform with the spelling used in existing standards.

#. No function shall have a name that overlaps a function in the C Standard
   Library. Examples of such names include strlen, atoi, and memset.

#. No function shall have a name that begins with an underscore.

#. Functions should be short and sweet, and do just one thing. They should fit
   on one or two screenfuls of text (the ISO/ANSI screen size is 80x24,
   as we all know), and do one thing and do that well. (Linux)

#. The maximum length of a function is inversely proportional to the complexity
   and indentation level of that function. So, if you have a conceptually simple
   function that is just one long (but simple) case-statement, where you have to
   do lots of small things for a lot of different cases, it’s OK to have a
   longer function. (Linux)

#. A prototype shall be declared for each public function in the module header
   file.

#. All private functions shall be declared ``static``.

#. Each parameter of a function shall be explicitly declared and meaningfully
   named.

#. Parameterized macros shall not be used if a function can be written to
   accomplish the same behavior.


Variables
*********

#. No variable shall have a name that is a keyword of C, C++, or any other
   well-known extension of the C programming language, including specifically K&R C
   and C99. Restricted names include ``interrupt``, ``inline``, ``restrict``,
   ``class``, ``true``, ``false``, ``public``, ``private``,``friend``, and ``protected``.

#. No variable shall have a name that overlaps with a variable name from the
   C Standard Library (e.g., ``errno``).

#. Local variable names should be short, and to the point. If you have some
   random integer loop counter, it should probably be called i. Calling it
   loop_counter is non-productive, if there is no chance of it being
   mis-understood.  Similarly, tmp can be just about any type of variable that
   is used to hold a temporary value. (Linux)

#. Variable names, structure tag names, and function names should be in lower
   case.

#. No variable name shall contain any uppercase letters, except when required to
   conform with the spelling used in existing standards.

#. Underscores shall be used to separate words in variable names.

#. Each variable’s name shall be descriptive of its purpose.

#. GLOBAL variables (to be used only if you really need them) need to have
   descriptive names, as do global functions. If you have a function that counts
   the number of active users, you should call that count_active_users()
   or similar, you should not call it cntusr(). (Linux)

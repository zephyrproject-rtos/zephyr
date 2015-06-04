Sanity Check Data Files
=======================

The sanity check data files contain project metadata that is parsed by the
relevant sanity check script to both build and run the listed projects.

Project metadata consists of:
 - project directory (relative to PRJ_PATH - defined in the script)
 - build arguments (optional)
 - project flags, enclosed in angle brackets "<>"
   - 'u' => microkernel; 'n' => nanokernel (required; must be first flag)
   - 'q' => run as part of quick sanity check (optional)
 - list of BSP names that can be used with the project

It is important that each set of project metadata be specified on a single line.

The sanity check scripts will select first listed BSP name if the user does not
specify a BSP name (i.e. is the "default" BSP).

A given project can appear more than once, allowing the project to be
sanitized multiple times. This can be useful if the project has multiple
configurations, or if the project should be characterized on more than one BSP
during a default BSP sanity check.

Comment lines are permitted (denoted by '#').

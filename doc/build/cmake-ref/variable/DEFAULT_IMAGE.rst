DEFAULT_IMAGE
#############

Name of the main application image in a sysbuild build.

This is the image added with ``APP_TYPE MAIN``, and the one reported as the default domain in
:file:`domains.yaml`. Use it from a :file:`sysbuild.cmake` file to refer to the main image without
hardcoding its name.

See :cmake:command:`ExternalZephyrProject_Add`.

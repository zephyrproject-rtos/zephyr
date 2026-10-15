APP_BUILD_VERSION
#################

This variable is used in all contexts as the build version of the application.

By default, if the application is in a Git repository, its contents are generated from the output of
the command ``git describe --abbrev=12 --always`` run in the application folder. This results in a
string which contains the most recent tag, the number of commits since that tag, and the abbreviated
commit hash.

This default behavior can also be overridden by setting the CMake variable ``APP_BUILD_VERSION`` to
the desired string value. This can either be done in the ``CMakeLists.txt`` file, or on the command
line when invoking CMake, e.g. via ``cmake -DAPP_BUILD_VERSION="v3.3.0-18-g2c85d92" ..``.

Input to :cmake:module:`version`
********************************

Custom application build version to use for this build, if set externally.

Output from :cmake:module:`version`
***********************************

Final value of the build version of the application: either the value of ``APP_BUILD_VERSION``, if
set, or the value generated from the Git repository.

See :ref:`app-version-details` for details; see also :cmake:variable:`BUILD_VERSION`.

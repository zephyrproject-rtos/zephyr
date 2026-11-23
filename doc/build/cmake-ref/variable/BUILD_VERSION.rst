BUILD_VERSION
#############

This variable is used in all contexts as the build version of the Zephyr kernel.

By default, if the Zephyr project is in a Git repository and the ``git`` tool is available, its
contents are generated from the output of the command ``git describe --abbrev=12 --always`` run in
the Zephyr folder. This results in a string which contains the most recent tag, the number of
commits since that tag, and the abbreviated commit hash.

This default behavior can also be overridden by setting the CMake variable ``BUILD_VERSION`` to
the desired string value. This can either be done in the ``CMakeLists.txt`` file, or on the command
line when invoking CMake, e.g. via ``cmake -DBUILD_VERSION="v3.3.0-18-g2c85d92" ..``.

Input to :cmake:module:`version`
********************************

Custom Zephyr build version to use for this build, if set externally.

Output from :cmake:module:`version`
***********************************

Final value of the build version of the Zephyr project: either the value of ``BUILD_VERSION``, if
already set, or the value generated from the Git repository.

See also :cmake:variable:`APP_BUILD_VERSION`.

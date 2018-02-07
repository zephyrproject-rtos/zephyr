SOURCES += \
    $$PWD/cborencoder.c \
    $$PWD/cborencoder_close_container_checked.c \
    $$PWD/cborerrorstrings.c \
    $$PWD/cborparser.c \
    $$PWD/cborparser_dup_string.c \
    $$PWD/cborpretty.c \
    $$PWD/cbortojson.c \
    $$PWD/cborvalidation.c \

HEADERS += $$PWD/cbor.h $$PWD/tinycbor-version.h

QMAKE_CFLAGS *= $$QMAKE_CFLAGS_SPLIT_SECTIONS
QMAKE_LFLAGS *= $$QMAKE_LFLAGS_GCSECTIONS
INCLUDEPATH += $$PWD
CONFIG(release, debug|release): DEFINES += NDEBUG

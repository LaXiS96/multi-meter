add_library(ch32v003_sdk INTERFACE)

target_include_directories(ch32v003_sdk INTERFACE
    Core
    Debug
    Peripheral/inc
    User
)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    *.c
    *.S
)
target_sources(ch32v003_sdk INTERFACE
    ${SOURCES}
)

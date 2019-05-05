
set (MS_MAJOR_VERSION 1)
set (MS_MINOR_VERSION 0)
set (MS_PATCH_VERSION 0)
set (MS_PATCH_DATE 20190505)
set (MS_VERSION ${MS_MAJOR_VERSION}.${MS_MINOR_VERSION}.${MS_PATCH_VERSION})

set(PROG_NAME auto_clean)
set(PROG_VER ${MS_VERSION}.${MS_PATCH_DATE})
#set(REVISION RELEASE)
set(REVISION BATE)

add_definitions(
        -D_GNU_SOURCE
        -DPROG_NAME="${PROG_NAME}"
        -DPROG_VER="${PROG_VER}"
        -DREVISION=${REVISION})

add_executable(auto_clean main.cpp)

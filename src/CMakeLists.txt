
set (MAJOR_VERSION 1)
set (MINOR_VERSION 0)
set (PATCH_VERSION 0)
set (PATCH_DATE 20190505)
set (VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})

set(PROG_NAME auto_clean)
set(PROG_VER ${VERSION}.${PATCH_DATE})
#set(REVISION RELEASE)
set(REVISION BATE)
set(DEFAULT_PIDFILE /var/run/auto_clean.pid)

add_definitions(
        -DPROG_NAME="${PROG_NAME}"
        -DPROG_VER="${PROG_VER}"
        -DREVISION=${REVISION}
        -DDEFAULT_PIDFILE="${DEFAULT_PIDFILE}")

set(AUTO_CLEAN
        main.cpp
        util-pidfile.cpp util-pidfile.h)

add_executable(auto_clean ${AUTO_CLEAN})

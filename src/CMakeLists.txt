
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
set(DEFAULT_CONFIG /etc/auto_clean.yaml)

add_definitions(
        -DPROG_NAME="${PROG_NAME}"
        -DPROG_VER="${PROG_VER}"
        -DREVISION=${REVISION}
        -DDEFAULT_PIDFILE="${DEFAULT_PIDFILE}"
        -DDEFAULT_CONFIG="${DEFAULT_CONFIG}")

set(AUTO_CLEAN
        main.cpp
        conf.cpp conf.h
        conf-yaml-loader.cpp conf-yaml-loader.h
        manager.cpp manager.h
        worker.cpp worker.cpp
        tm-threads.cpp tm-threads.h
        util-disk.cpp util-disk.h
        util-file.cpp util-file.h
        util-path.cpp util-path.h
        util-pidfile.cpp util-pidfile.h
        util-strlcatu.cpp util-strlcatu.h
        util-strlcpyu.cpp util-strlcpyu.h
        util-threads.cpp util-threads.h)

link_libraries(
        yaml
        pthread)

add_executable(auto_clean ${AUTO_CLEAN})


set(MAJOR_VERSION 1)
set(MINOR_VERSION 0)
set(PATCH_VERSION 0)
set(PATCH_DATE 20190505)
set(VERSION ${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION})

set(PROG_NAME auto_clean)
set(PROG_VER ${VERSION}.${PATCH_DATE})
#set(REVISION RELEASE)
set(REVISION BATE)
set(DEFAULT_PIDFILE /var/run/auto_clean.pid)
set(DEFAULT_CONFIG /etc/auto_clean.json)

include(FindPkgConfig)
pkg_check_modules(DEPENDS_LIBS REQUIRED jsoncpp-static>=1.4.0 spdlog>=1.3.1)
include_directories(${DEPENDS_LIBS_INCLUDE_DIRS})
link_libraries(${DEPENDS_LIBS_LIBRARIES})

set(Boost_USE_STATIC_LIBS ON) # only find static libs, default None
set(Boost_USE_MULTITHREADED ON) # default TRUE
set(Boost_USE_STATIC_RUNTIME OFF) # default None
find_package(Boost 1.69.0 REQUIRED COMPONENTS filesystem timer date_time)
link_libraries(${Boost_LIBRARIES})

add_definitions(
        -DPROG_NAME="${PROG_NAME}"
        -DPROG_VER="${PROG_VER}"
        -DREVISION=${REVISION}
        -DDEFAULT_PIDFILE="${DEFAULT_PIDFILE}"
        -DDEFAULT_CONFIG="${DEFAULT_CONFIG}")

set(AUTO_CLEAN
        main.cpp
        manager.cpp manager.h
        worker.cpp worker.cpp
        tm-threads.cpp tm-threads.h
        util-disk.cpp util-disk.h
        util-file.cpp util-file.h
        util/config.cpp util/config.h
        util/pidfile.cpp util/pidfile.h
        util/log.cpp util/log.h
        util-threads.cpp util-threads.h)

link_libraries(pthread)
include_directories(.)

add_executable(auto_clean ${AUTO_CLEAN})

cmake_minimum_required (VERSION 3.0)
project (cdcc C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_PREFIX $ENV{HOME}/.local)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB2 glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
link_directories(${GLIB2_LIBRARY_DIRS})

pkg_check_modules(SQLITE3 sqlite3)
include_directories(${SQLITE3_INCLUDE_DIRS})
link_directories(${SQLITE3_LIBRARY_DIRS})

add_executable(cdcc cdcc.c)
target_link_libraries(cdcc ${GLIB2_LIBRARIES} ${SQLITE3_LIBRARIES})

install(TARGETS cdcc RUNTIME DESTINATION bin)


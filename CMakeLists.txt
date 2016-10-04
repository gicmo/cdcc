cmake_minimum_required (VERSION 3.0)
project (cdcc C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_PREFIX $ENV{HOME}/.local)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB2 glib-2.0 gio-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
link_directories(${GLIB2_LIBRARY_DIRS})

pkg_check_modules(SQLITE3 sqlite3)
include_directories(${SQLITE3_INCLUDE_DIRS})
link_directories(${SQLITE3_LIBRARY_DIRS})

pkg_check_modules(GJSON json-glib-1.0)
include_directories(${GJSON_INCLUDE_DIRS})
link_directories(${GJSON_LIBRARY_DIRS})

add_executable(cdcc-cc cc.c db.c)
target_link_libraries(cdcc-cc ${GLIB2_LIBRARIES} ${SQLITE3_LIBRARIES})

add_executable(cdcc-query query.c db.c)
target_link_libraries(cdcc-query
  ${GLIB2_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${GJSON_LIBRARIES})

install(TARGETS cdcc-cc    RUNTIME DESTINATION bin)
install(TARGETS cdcc-query RUNTIME DESTINATION bin)


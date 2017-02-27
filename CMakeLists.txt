cmake_minimum_required (VERSION 3.0)
project (cdcc C)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)

set(CDCC_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wunreachable-code")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -O0 -ggdb3")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GLOBAL_INSTALL "install globally, not in $HOME/.local/bin" [OFF])

if(NOT GLOBAL_INSTALL)
  set(CMAKE_INSTALL_PREFIX $ENV{HOME}/.local)
endif()

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

add_executable(cdcc-gen gen.c db.c)
target_link_libraries(cdcc-gen
  ${GLIB2_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${GJSON_LIBRARIES})

add_executable(cdcc-query query.c db.c)
target_link_libraries(cdcc-query
  ${GLIB2_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${GJSON_LIBRARIES})


install(TARGETS cdcc-cc    RUNTIME DESTINATION bin)
install(TARGETS cdcc-gen   RUNTIME DESTINATION bin)
install(TARGETS cdcc-query RUNTIME DESTINATION bin)

set(TOOLS gcc g++ clang clang++)
foreach(tool IN LISTS TOOLS)
  SET(link_name   cdcc-${tool})
  SET(link_target cdcc-cc)
  ADD_CUSTOM_COMMAND(TARGET cdcc-cc POST_BUILD
    COMMAND ln -sf ${link_target} ${link_name}
    COMMENT "cdcc-${tool} -> cdcc-cc")
  install(FILES ${CMAKE_BINARY_DIR}/cdcc-${tool}  DESTINATION bin)
endforeach()

# tarballs
set(CPACK_GENERATOR TGZ)
set(CPACK_SOURCE_PACKAGE_FILE_NAME "cdcc-${CDCC_VERSION}"
    CACHE INTERNAL "tarball basename")
set(CPACK_SOURCE_GENERATOR TGZ)

set(CPACK_SOURCE_IGNORE_FILES
    "~$"
    "\\\\.gitignore$"
    "^${PROJECT_SOURCE_DIR}/.git/"
    "^${PROJECT_SOURCE_DIR}/build/")

include(CPack)

message("=========================================")
message("Version:      ... ${CDCC_VERSION}")
message("Install path: ... ${CMAKE_INSTALL_PREFIX}")

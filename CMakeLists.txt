project(tilda C)
cmake_minimum_required(VERSION 2.8)

if(NOT(CMAKE_C_FLAGS))
  set(CMAKE_C_FLAGS  "-Wall -Wformat -Wformat-security -Wextra -Wshadow -Wmissing-prototypes -Wmissing-declarations -Wno-unused-parameter -Wno-deprecated-declarations -std=c99")
endif(NOT(CMAKE_C_FLAGS))

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cMake")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(VTE REQUIRED vte-2.90)
pkg_check_modules(CONFUSE REQUIRED libconfuse)
find_package(X11 REQUIRED)

INCLUDE_DIRECTORIES(${GLIB2_INCLUDE_DIRS} ${GTK3_INCLUDE_DIRS} ${VTE_INCLUDE_DIRS} ${CONFUSE_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/src)
LINK_DIRECTORIES(${GTK3_LIBRARY_DIRS} ${CONFUSE_LIBRARY_DIRS})

set(BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
set(DATADIR ${CMAKE_INSTALL_PREFIX}/share/)
set(PKGDATADIR ${DATADIR}/tilda)
set(PIXMAPSDIR ${CMAKE_INSTALL_PREFIX}/share/tilda/pixmaps)

CONFIGURE_FILE(tilda.desktop.in ${CMAKE_BINARY_DIR}/tilda.desktop)
install(FILES "${CMAKE_BINARY_DIR}/tilda.desktop" DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
install(FILES "${CMAKE_SOURCE_DIR}/tilda.png" DESTINATION ${PIXMAPSDIR}/)
install(FILES "${CMAKE_SOURCE_DIR}/tilda.glade" DESTINATION ${PKGDATADIR}/)

include(po)
COMPILE_PO_FILES(${CMAKE_SOURCE_DIR}/po MO_FILES)

add_definitions(${GTK3_DEFINITIONS})
add_definitions(-DPACKAGE_NAME="Tilda")
add_definitions(-DPACKAGE_VERSION="1.2-alpha")
add_definitions(-DDATADIR="${DATADIR}")
add_definitions(-DPKGDATADIR="${PKGDATADIR}")
add_definitions(-DDEBUG)
add_definitions(-DDEBUG_FUNCTIONS)


ADD_SUBDIRECTORY(${CMAKE_SOURCE_DIR}/src)


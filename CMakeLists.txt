project(tilda C)
cmake_minimum_required(VERSION 2.8)

#===========================================================
IF(NOT(CMAKE_C_FLAGS))
  SET(CMAKE_C_FLAGS  "-Wall -Wformat -Wformat-security -Wextra -Wshadow -Wmissing-prototypes -Wmissing-declarations -pedantic")
ENDIF(NOT(CMAKE_C_FLAGS))
#===========================================================
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cMake")

SET(_VTE_VERSION_NUM 2.90)
FIND_PACKAGE(glib2 REQUIRED)
FIND_PACKAGE(GTK3 REQUIRED)
FIND_PACKAGE(VTE REQUIRED)
FIND_PACKAGE(confuse REQUIRED)

INCLUDE_DIRECTORIES(${GLIB2_INCLUDE_DIRS} ${GTK3_INCLUDE_DIRS} ${VTE_INCLUDE_DIRS} ${CONFUSE_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/src)
LINK_DIRECTORIES(${GTK3_LIBRARY_DIRS})
ADD_DEFINITIONS(${GTK3_DEFINITIONS})

ADD_DEFINITIONS(-DPACKAGE_NAME="Tilda")
ADD_DEFINITIONS(-DPACKAGE_VERSION="1.5.1")
ADD_DEFINITIONS(-DDATADIR="share/")
#===========================================================
ADD_SUBDIRECTORY(${CMAKE_SOURCE_DIR}/src)
#===========================================================

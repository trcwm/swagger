# Makefile for swagger
# Niels A. Moseley
#
#
# Note: PreferredToolArchtecture=x64 can be used to force 64 bit
# MSVC compiling -- untested.
#

cmake_minimum_required (VERSION 3.2)
project (swagger)

set(CMAKE_AUTOMOC ON)

find_package(Qt5Core REQUIRED)

# custom compilation of squirrel library
# because the standard CMAKE script
# don't work properly..

# #################################################################
# SQUIRREL CORE
# #################################################################
set(SQUIRREL_SRC_DIR ${CMAKE_SOURCE_DIR}/contrib/squirrel/squirrel)
set(SQUIRREL_SRC ${SQUIRREL_SRC_DIR}/sqapi.cpp
                 ${SQUIRREL_SRC_DIR}/sqbaselib.cpp
                 ${SQUIRREL_SRC_DIR}/sqclass.cpp
                 ${SQUIRREL_SRC_DIR}/sqcompiler.cpp
                 ${SQUIRREL_SRC_DIR}/sqdebug.cpp
                 ${SQUIRREL_SRC_DIR}/sqfuncstate.cpp
                 ${SQUIRREL_SRC_DIR}/sqlexer.cpp
                 ${SQUIRREL_SRC_DIR}/sqmem.cpp
                 ${SQUIRREL_SRC_DIR}/sqobject.cpp
                 ${SQUIRREL_SRC_DIR}/sqstate.cpp
                 ${SQUIRREL_SRC_DIR}/sqtable.cpp
                 ${SQUIRREL_SRC_DIR}/sqvm.cpp)
include_directories(${CMAKE_SOURCE_DIR}/contrib/squirrel/include)

# #################################################################
# SQUIRREL STANDARD LIBRARY
# #################################################################

set(SQSTDLIB_SRC_DIR ${CMAKE_SOURCE_DIR}/contrib/squirrel/sqstdlib)
set(SQSTDLIB_SRC ${SQSTDLIB_SRC_DIR}/sqstdaux.cpp
                 ${SQSTDLIB_SRC_DIR}/sqstdblob.cpp
                 ${SQSTDLIB_SRC_DIR}/sqstdio.cpp
                 ${SQSTDLIB_SRC_DIR}/sqstdmath.cpp
                 ${SQSTDLIB_SRC_DIR}/sqstdrex.cpp
                 ${SQSTDLIB_SRC_DIR}/sqstdstream.cpp
                 ${SQSTDLIB_SRC_DIR}/sqstdstring.cpp
                 ${SQSTDLIB_SRC_DIR}/sqstdsystem.cpp)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# #################################################################
# SWAGGER STUFF
# #################################################################

set(SWAGGER_SRC src/main.cpp
                src/cobs.cpp
                src/hardwareinterface.cpp)

include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable (swagger ${SQUIRREL_SRC} ${SQSTDLIB_SRC} ${SWAGGER_SRC})

qt5_use_modules(swagger SerialPort)

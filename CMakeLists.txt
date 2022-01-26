###############################################################################
#
# This file is CMake configuration for HASHUDR library
#
###############################################################################
# General settings
############

cmake_minimum_required(VERSION 3.4 FATAL_ERROR)

project (HASHUDR)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

###############################################################################
# CMake standard options
############

option(BUILD_SHARED_LIBS "Build shared library" ON)

###############################################################################
# Specific options
############

option(HASHUDR_DISABLE_LIBCXX "Do not use libc++, if available" ON)

###############################################################################
# Version
############

find_path(UDR_VERSION VERSION.txt ${CMAKE_SOURCE_DIR})
if(UDR_VERSION)
	file(STRINGS VERSION.txt HASHUDR_VERSION REGEX "[0-9]+\\.[0-9]+\\.[0-9]+")
	string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" HASHUDR_VERSION_MAJOR "${HASHUDR_VERSION}")
	string(REGEX REPLACE "^[0-9]+\\.([0-9])+\\.[0-9]+" "\\1" HASHUDR_VERSION_MINOR "${HASHUDR_VERSION}")
	string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" HASHUDR_VERSION_PATCH "${HASHUDR_VERSION}")
	message(STATUS "hashudr version: ${HASHUDR_VERSION}")
endif()

###############################################################################
# Require and enable C++ 0x/11/14/17
############

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
message(STATUS "hashudr compile: C++${CMAKE_CXX_STANDARD}")

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_COMPILER_IS_GNUCXX)
  	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wnarrowing -Werror")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
	include(CheckCXXCompilerFlag)
	if(NOT HASHUDR_DISABLE_LIBCXX)
		check_cxx_compiler_flag("-stdlib=libc++" CXX_SUPPORTS_STDLIB)
		if(CXX_SUPPORTS_STDLIB)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
			set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
			set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
			set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++")
		endif()
		set(HASHUDR_DISABLE_LIBCXX ${HASHUDR_DISABLE_LIBCXX} CACHE BOOL "Do not use libc++, if available." FORCE)
	endif()
	message(STATUS "hashudr build: Disable linking libc++ - ${HASHUDR_DISABLE_LIBCXX}")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Intel")
	string(REGEX REPLACE "[/-]W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	if (NOT (CMAKE_VERSION VERSION_LESS 3.6.0)) # Compiler features for Intel in CMake 3.6+
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Qstd=c++17")
	endif()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /QaxCORE-AVX2")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise")
	set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   /Od")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O3")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Qipo")
elseif(MSVC)
	string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	if(MSVC_VERSION LESS 1900)
		message(FATAL_ERROR "hashudr build: Build requires C++17-compliant compiler")
	endif()
endif()

if(CMAKE_BUILD_TYPE MATCHES Debug)
	message(STATUS "hashudr build: compiler flags - ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG}")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
	message(STATUS "hashudr build: compiler flags - ${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE}")
endif()

###############################################################################
# HASH sources to link
############

find_path(HASH_INCLUDE_DIR crc_32.h	${CMAKE_SOURCE_DIR}/hash/include)

if(NOT HASH_INCLUDE_DIR)
	message(FATAL_ERROR "hashudr build: Location of hash files could not found")
endif()

message(STATUS "hashudr link: hash files - ${HASH_INCLUDE_DIR}")

###############################################################################
# Firebird libraries to link
############

find_path(FIREBIRD_INCLUDE_DIR ibase.h
	/usr/include
	/opt/firebird/include
	${CMAKE_SOURCE_DIR}/firebird/include
	$ENV{ProgramFiles}/Firebird/*/include
)

set(FIREBIRD_LIB_NAMES fbclient fbclient_ms)
find_library(FIREBIRD_LIB
	NAMES
		${FIREBIRD_LIB_NAMES}
	PATHS
		/usr/lib
		/opt/firebird/lib
		${CMAKE_SOURCE_DIR}/firebird/lib
		$ENV{ProgramFiles}/Firebird/*/lib
)

if(NOT FIREBIRD_INCLUDE_DIR OR NOT FIREBIRD_LIB)
	message(FATAL_ERROR "hashudr build: Location of Firebird library could not found")
endif()

add_library(firebird SHARED IMPORTED)
if(UNIX)
	set_target_properties(firebird
		PROPERTIES
			IMPORTED_LOCATION ${FIREBIRD_LIB}
			INTERFACE_INCLUDE_DIRECTORIES "${FIREBIRD_INCLUDE_DIR};${FIREBIRD_INCLUDE_DIR}/firebird"
	)
else()
	set_target_properties(firebird
		PROPERTIES
			IMPORTED_IMPLIB ${FIREBIRD_LIB}
			INTERFACE_INCLUDE_DIRECTORIES "${FIREBIRD_INCLUDE_DIR};${FIREBIRD_INCLUDE_DIR}/firebird"
)
endif()

message(STATUS "hashudr link: firebird - ${FIREBIRD_LIB}")

###############################################################################
# Target library
############

file(GLOB files "${CMAKE_SOURCE_DIR}/src/*")

add_library(hash SHARED ${files})

set(DEF_FILE "udr_plugin") 

if(UNIX) 
	set(DEF_FILE ${DEF_FILE}.vers)
	set(WL_OPTION "--version-script")
	set_target_properties(hash
		PROPERTIES
			LINK_FLAGS -Wl,${WL_OPTION},${CMAKE_SOURCE_DIR}/build/defs/${DEF_FILE}
			LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/x86_64-linux
	)
elseif(MSVC OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")
	set(DEF_FILE ${DEF_FILE}.def) 
	set_target_properties(hash PROPERTIES LINK_FLAGS "/DEF:\"${CMAKE_SOURCE_DIR}/build/defs/${DEF_FILE}\"")
	if(MSVC AND CMAKE_BUILD_TYPE MATCHES Debug)
		find_path(FIREBIRD_PLUGINS udr_engine.dll
			$ENV{ProgramFiles}/Firebird/*/plugins
		)
		set_target_properties(hash
			PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY ${FIREBIRD_PLUGINS}/udr
		)
	elseif(CMAKE_BUILD_TYPE MATCHES Release)
		set_target_properties(hash
			PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/x86_64-win64
		)
	endif()
endif()

if(BUILD_SHARED_LIBS AND HASHUDR_VERSION)
	set_target_properties(hash PROPERTIES VERSION ${HASHUDR_VERSION})
endif()

target_include_directories(hash PUBLIC ${HASH_INCLUDE_DIR})

target_link_libraries(hash PUBLIC firebird)

######


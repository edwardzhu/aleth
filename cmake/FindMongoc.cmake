# Find mongoDb c

find_path(
	MONGOC_INCLUDE_DIR 
	NAMES libmongoc-1.0/mongoc/mongoc.h
	DOC "mongoc include dir"
)

option(MONGOC_USE_STATIC_LIBS "link static libs" OFF)

if(MONGOC_USE_STATIC_LIBS)
	set(names_1 ${CMAKE_STATIC_LIBRARY_PREFIX}bson-static-1.0${CMAKE_STATIC_LIBRARY_SUFFIX})
	set(names_2 ${CMAKE_STATIC_LIBRARY_PREFIX}mongoc-static-1.0${CMAKE_STATIC_LIBRARY_SUFFIX})
else()
	set(names_1 bson-1.0)
	set(names_2 mongoc-1.0)
endif()

find_library(
	MONGOC_LIBRARY_1
	NAMES ${names_1}
	DOC "mongoc library"
)
find_library(
	MONGOC_LIBRARY_2
	NAMES ${names_2}
	DOC "mongoc library"
)
set(MONGOC_LIBRARY ${MONGOC_LIBRARY_1} ${MONGOC_LIBRARY_2})

message(STATUS "MONGOC_LIBRARY => ${MONGOC_LIBRARY}")

# handle the QUIETLY and REQUIRED arguments and set MONGOC_FOUND to TRUE
# if all listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(mongoc DEFAULT_MSG
	MONGOC_LIBRARY MONGOC_INCLUDE_DIR)
mark_as_advanced (MONGOC_INCLUDE_DIR MONGOC_LIBRARY)

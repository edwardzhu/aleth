# Find mongoDb cxx

find_path(
	MONGOCXX_INCLUDE_DIR_1 
    NAMES mongocxx/instance.hpp
    DOC "mongocxx include dir"
    PATH_SUFFIXES mongocxx/v_noabi
)

find_path(
	MONGOCXX_INCLUDE_DIR_2
    NAMES bsoncxx/builder/stream/document.hpp
    DOC "mongocxx include dir"
    PATH_SUFFIXES bsoncxx/v_noabi
)

set(MONGOCXX_INCLUDE_DIR ${MONGOCXX_INCLUDE_DIR_1} ${MONGOCXX_INCLUDE_DIR_2})

option(MONGOCXX_USE_STATIC_LIBS "link static libs" OFF)

if(MONGOCXX_USE_STATIC_LIBS)
	set(names_1 ${CMAKE_STATIC_LIBRARY_PREFIX}bsoncxx-static${CMAKE_STATIC_LIBRARY_SUFFIX})
	set(names_2 ${CMAKE_STATIC_LIBRARY_PREFIX}mongocxx-static${CMAKE_STATIC_LIBRARY_SUFFIX})
else()
	set(names_1 libbsoncxx.so)
	set(names_2 libmongocxx.so)
endif()

find_library(
	MONGOCXX_LIBRARY_1
	NAMES ${names_1}
	DOC "mongocxx library"
)

find_library(
	MONGOCXX_LIBRARY_2
	NAMES ${names_2}
	DOC "mongocxx library"
)

set(MONGOCXX_LIBRARY ${MONGOCXX_LIBRARY_1} ${MONGOCXX_LIBRARY_2})

message(STATUS "MONGOCXX_LIBRARY => ${MONGOCXX_LIBRARY}")

# handle the QUIETLY and REQUIRED arguments and set MONGOC_FOUND to TRUE
# if all listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(mongocxx DEFAULT_MSG
	MONGOCXX_LIBRARY MONGOCXX_INCLUDE_DIR)
mark_as_advanced (MONGOCXX_INCLUDE_DIR MONGOCXX_LIBRARY)

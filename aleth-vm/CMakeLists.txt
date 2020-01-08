add_executable(aleth-vm main.cpp)

find_package(Mongocxx REQUIRED)
target_link_libraries(aleth-vm PRIVATE ethereum evm ethashseal devcore Boost::program_options ${MONGOCXX_LIBRARY})

target_include_directories(aleth-vm PRIVATE ../utils ${MONGOCXX_INCLUDE_DIR})

install(TARGETS aleth-vm EXPORT alethTargets DESTINATION bin)

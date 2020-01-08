set(
    sources
    AccountManager.cpp AccountManager.h
    main.cpp
    MinerAux.cpp MinerAux.h
)

find_package(Mongocxx REQUIRED)
add_executable(aleth ${sources})
target_link_libraries(
    aleth
    PRIVATE ethereum ethashseal evm web3jsonrpc webthree devcore Boost::program_options ${MONGOCXX_LIBRARY}
)

target_include_directories(aleth PRIVATE ../utils ${MONGOCXX_INCLUDE_DIR})

if(MINIUPNPC)
    target_compile_definitions(aleth PRIVATE ETH_MINIUPNPC)
endif()

install(TARGETS aleth DESTINATION bin)

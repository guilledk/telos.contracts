add_contract(telos.gpu telos.gpu ${CMAKE_CURRENT_SOURCE_DIR}/src/telos.gpu.cpp)

target_include_directories(telos.gpu
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(telos.gpu
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/telos.gpu.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/telos.gpu.contracts.md @ONLY )

# target_compile_options( telos.gpu PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )

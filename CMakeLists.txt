cmake_minimum_required(VERSION 2.8.7)

project(WordCount CXX)

enable_language(CXX)
enable_testing()

# include_directories(
#     ${CMAKE_CURRENT_BINARY_DIR}
# )



if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/catch)
    message(FATAL_ERROR "Catch unit test library not cloned. Please run "
            "git submodule update --init to clone this repository.")
endif()

include_directories(
    ./src
    ./lib/catch/single_include
)

function(add_wc_test name)
    add_executable(${name} tests/${name}.cpp)
    install(TARGETS ${name} RUNTIME DESTINATION bin)
    add_test(test_${name} ${name})
    add_custom_target(all_tests COMMAND ${CMAKE_CTEST_COMMAND}
                      DEPENDS test_${name})
endfunction()

add_wc_test(count)

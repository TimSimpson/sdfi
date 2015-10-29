cmake_minimum_required(VERSION 2.8.7)

project(WordCountTests CXX)

if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/catch)
  message(FATAL_ERROR "Catch unit test library not cloned. Please run "
    "git submodule update --init to clone this repository.")
endif()

include_directories(
    ../src
    ../lib/catch/single_include
)

function(add_wc_test name)
    add_executable(${name} ${name}.cpp)
    add_test(${name} ${name})
endfunction()

add_wc_test(count)

cmake_minimum_required(VERSION 2.8.7)

project(WordCount CXX)

enable_language(CXX)
enable_testing()


if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()


set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost COMPONENTS filesystem regex system REQUIRED)
if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/lib/catch)
    message(FATAL_ERROR "Catch unit test library not cloned. Please run "
            "git submodule update --init to clone this repository.")
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(
    ./src
    ./lib/catch/single_include
)



function(add_wc_exe name src)
    add_executable(${name} src/${src})
    target_link_libraries(${name}
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_REGEX_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_LIBRARIES}
    )
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_link_libraries(${name}
            -lm
            -lpthread
        )
    endif()

    install(TARGETS ${name} RUNTIME DESTINATION bin)
endfunction()


function(add_wc_test name)
    add_executable(${name} tests/${name}.cpp)
    target_link_libraries(${name}
        ${Boost_LIBRARIES}
    )
    install(TARGETS ${name} RUNTIME DESTINATION bin)
    add_test(test_${name} ${name})
    add_custom_target(all_tests COMMAND ${CMAKE_CTEST_COMMAND}
                      DEPENDS test_${name})
endfunction()


add_wc_exe(read_file read_file.cpp)
add_wc_exe(read_directory read_directory.cpp)
add_wc_exe(worker worker.cpp)
add_wc_exe(master master.cpp)

add_wc_exe(worker2 worker2.cpp)
add_wc_exe(master2 master2.cpp)

add_wc_exe(worker3 worker3.cpp)
add_wc_exe(master3 master3.cpp)

add_wc_test(count_tests)

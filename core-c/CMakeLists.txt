cmake_minimum_required(VERSION 3.16)
project(aegis_crypto C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_definitions(-D_POSIX_C_SOURCE=199309L)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -pedantic -O3")

include_directories(include)

file(GLOB SOURCES "src/*.c")

add_library(aegis STATIC ${SOURCES})
add_library(aegis_shared SHARED ${SOURCES})
set_target_properties(aegis_shared PROPERTIES OUTPUT_NAME "aegis")

add_executable(aegis-cli cli/main.c)
target_link_libraries(aegis-cli aegis)

enable_testing()

add_executable(test_hash tests/test_hash.c)
target_link_libraries(test_hash aegis)
add_test(NAME HashTests COMMAND test_hash)

add_executable(test_smt tests/test_smt.c)
target_link_libraries(test_smt aegis)
add_test(NAME SmtTests COMMAND test_smt)

add_executable(bench tests/bench.c)
target_link_libraries(bench aegis)

cmake_minimum_required(VERSION 3.10)

project(DAOS-benchmark 
        DESCRIPTION "DAOS benchmarking for Atlas data flow"
        LANGUAGES C CXX
)

set(TARGET ${CMAKE_PROJECT_NAME})
set(DAOS_DOCKER_IMG daos-client)
# set(CMAKE_CXX_COMPILER clang-13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags for debuging
add_compile_options(-ggdb -fdiagnostics-color=always -fsanitize=address -fno-omit-frame-pointer) #-Wextra -fsanitize=thread,memory
add_link_options(-fsanitize=address) # -fsanitize=address,thread,memory

add_compile_options(-Wall -Wno-comment -Wpedantic -Wno-c99-extensions -Wno-unused-private-field -Wno-c++98-compat-pedantic)

add_subdirectory(lib/daos-cxx)


add_executable(${TARGET} src/benchmark.cxx)

target_link_libraries(${TARGET} daos-cxx)
target_include_directories(${TARGET} PUBLIC src/daos-cxx/include)

add_custom_target(install_docker
    COMMAND docker cp ${TARGET} ${DAOS_DOCKER_IMG}:/
    DEPENDS ${TARGET}
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)

add_custom_target(run
    COMMAND docker exec ${DAOS_DOCKER_IMG} ./${TARGET}
    DEPENDS install_docker
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)


cmake_minimum_required(VERSION 3.10)

project(DAOS-benchmark 
        DESCRIPTION "DAOS benchmarking for Atlas data flow"
        LANGUAGES C CXX
)

set(TARGET ${CMAKE_PROJECT_NAME})
set(DAOS_INSTALL_DIR /home/mkojro/s/daos/install)
set(DAOS_DOCKER_IMG daos-client)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-ggdb -fdiagnostics-color=always) #-Wextra 
add_compile_options(-Wall -Wno-comment -Wpedantic -Wno-c99-extensions -Wno-unused-private-field -Wno-c++98-compat-pedantic)

add_executable(${TARGET} 
    src/benchmark.cxx 
    src/Pool.h 
    src/Container.h 
    src/Errors.h 
    src/types.h
    src/KeyValue.h
    src/DAOSObject.h src/DAOSObject.cxx
    src/Array.h src/Array.cxx
)

target_include_directories(${TARGET} PUBLIC ${DAOS_INSTALL_DIR}/include )

target_link_libraries(${TARGET} PUBLIC 
    ${DAOS_INSTALL_DIR}/lib64/libdaos.so 
    ${DAOS_INSTALL_DIR}/lib64/libgurt.so 
    ${DAOS_INSTALL_DIR}/lib64/libdaos_common.so 
    /usr/lib64/libuuid.so
)

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


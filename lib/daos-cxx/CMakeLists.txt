cmake_minimum_required(VERSION 3.10)

project(daos-cxx 
        DESCRIPTION "DAOS bindings for c++"
        LANGUAGES C CXX
)

option(DAOS_DIR "Location of installed libdaos" "OFF")

if(${DAOS_DIR} STREQUAL "OFF")
    set(DAOS_INSTALL_DIR "/home/mkojro/s/daos/install")
else()
    set(DAOS_INSTALL_DIR ${DAOS_DIR})
endif()


add_library(daos-cxx
    ./include/Pool.h src/Pool.cxx 
    ./include/Container.h src/Container.cxx
    ./include/Errors.h 
    ./include/types.h
    ./include/EventQueue.h src/EventQueue.cxx
    ./include/KeyValue.h src/KeyValue.cxx
    ./include/DAOSObject.h src/DAOSObject.cxx
    ./include/Array.h src/Array.cxx
    ./include/UUID.h src/UUID.cxx)

target_include_directories(daos-cxx PUBLIC ${DAOS_INSTALL_DIR}/include)
target_include_directories(daos-cxx PUBLIC include)

target_link_libraries(daos-cxx PUBLIC 
    ${DAOS_INSTALL_DIR}/lib64/libdaos.so 
    ${DAOS_INSTALL_DIR}/lib64/libgurt.so 
    ${DAOS_INSTALL_DIR}/lib64/libdaos_common.so 
    uuid
)
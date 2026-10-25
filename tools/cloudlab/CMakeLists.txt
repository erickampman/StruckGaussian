cmake_minimum_required(VERSION 3.20)
project(cloudlab CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# The object is the plugin's own ModalBank (src/Module), built in place so the
# experiments always hear what the plugin plays.
set(STRUCKGAUSSIAN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "StruckGaussian checkout (ModalBank lives in its src/Module)")
set(MODAL_DIR "${STRUCKGAUSSIAN_DIR}/src/Module")
if(NOT EXISTS "${MODAL_DIR}/ModalBank.cpp")
    message(FATAL_ERROR "ModalBank.cpp not found under ${MODAL_DIR}; set STRUCKGAUSSIAN_DIR")
endif()

add_executable(cloudlab cloudlab.cpp "${MODAL_DIR}/ModalBank.cpp")
target_include_directories(cloudlab PRIVATE "${MODAL_DIR}")
target_compile_options(cloudlab PRIVATE -O2 -Wall -Wno-unused-parameter)

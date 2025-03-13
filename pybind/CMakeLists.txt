if(NOT DEPS_FOLDER)
    set(DEPS_FOLDER ${CMAKE_SOURCE_DIR}/deps)
endif(NOT DEPS_FOLDER)

set(SOURCE
		include/chiaki-pybind.h
		src/discover.c
		src/wakeup.c)

add_library(chiaki-pybind-lib STATIC ${SOURCE})
target_include_directories(chiaki-pybind-lib PUBLIC "include/chiaki-pybind.h")
target_include_directories(chiaki-pybind-lib PUBLIC "include")
target_link_libraries(chiaki-pybind-lib chiaki-lib)

find_package(Python REQUIRED COMPONENTS Interpreter Development)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

set(PYBIND11_PYTHON_VERSION 3.10)
include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    SOURCE_DIR "${DEPS_FOLDER}/pybind11"
    GIT_TAG v2.11.1
)
FetchContent_MakeAvailable(pybind11)
include_directories(lib/pybind11/include)

pybind11_add_module(chiaki_py src/main.cpp)
target_include_directories(chiaki_py PUBLIC "include")
target_link_libraries(chiaki_py PRIVATE chiaki-lib chiaki-pybind-lib)

if(WIN32)
    target_link_libraries(chiaki-pybind-lib ws2_32)
    target_link_libraries(chiaki_py PRIVATE ws2_32)
endif()

target_compile_definitions(chiaki_py PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})
list(APPEND PYTHONPATH "${CMAKE_CURRENT_BINARY_DIR}")

add_custom_command(TARGET chiaki_py POST_BUILD
    VERBATIM COMMAND python -c "import os; import sys; sys.path.insert(1, os.getcwd()); from pybind11_stubgen import main; import chiaki_py; sys.argv[0] = 'pybind11-stubgen'; sys.argv.append('--ignore-all-errors'); sys.argv.append('-o'); sys.argv.append('.'); sys.argv.append('chiaki_py'); main()" ||(exit 0)
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Creating stubs for MC3D-TRECSIM"
)


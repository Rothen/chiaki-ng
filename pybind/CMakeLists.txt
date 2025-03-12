
set(SOURCE
		include/chiaki-pybind.h
		src/discover.c
		src/wakeup.c)

if(NOT DEPS_FOLDER)
    set(DEPS_FOLDER ${CMAKE_SOURCE_DIR}/deps)
endif(NOT DEPS_FOLDER)

add_library(chiaki-pybind-lib STATIC ${SOURCE})
target_include_directories(chiaki-pybind-lib PUBLIC "include")
target_link_libraries(chiaki-pybind-lib chiaki-lib)

set(PYBIND11_PYTHON_VERSION 3.10)

FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    SOURCE_DIR "${DEPS_FOLDER}/pybind11"
    GIT_TAG v2.11.1
)
FetchContent_MakeAvailable(pybind11)
include_directories(lib/pybind11/include)

pybind11_add_module(chiaki src/bindings.cpp)

target_compile_definitions(chiaki PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})
# target_link_libraries(chiaki PRIVATE Eigen3::Eigen)
list(APPEND PYTHONPATH "${CMAKE_CURRENT_BINARY_DIR}")

add_custom_command(TARGET chiaki POST_BUILD
    VERBATIM COMMAND python -c "import os; import sys; sys.path.insert(1, os.getcwd()); from pybind11_stubgen import main; import chiaki; sys.argv[0] = 'pybind11-stubgen'; sys.argv.append('--ignore-all-errors'); sys.argv.append('-o'); sys.argv.append('.'); sys.argv.append('chiaki'); main()" ||(exit 0)
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Creating stubs for MC3D-TRECSIM"
)

# if(COPY_TO_SOURCE)
#     SET(COPY_TO_PATH ../../src/mc3d_trecsim)
#     ADD_CUSTOM_COMMAND(TARGET chiaki POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy
#         $<TARGET_FILE:chiaki>
#         ${COPY_TO_PATH}
#         COMMENT "Copying 'chiaki' library to '${COPY_TO_PATH}/chiaki.so'")
# 
#     ADD_CUSTOM_COMMAND(TARGET chiaki POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy
#         ${CMAKE_CURRENT_BINARY_DIR}/chiaki.pyi
#         ${COPY_TO_PATH}
#         COMMENT "Copying 'chiaki' stubs to '${COPY_TO_PATH}/chiaki.pyi'")
# endif(COPY_TO_SOURCE)

# if(CHIAKI_CLI_ARGP_STANDALONE)
# 	find_package(Argp REQUIRED)
# 	target_link_libraries(chiaki-pybind-lib Argp::Argp)
# endif()

add_executable(chiaki-pybind src/main.cpp)
target_link_libraries(chiaki-pybind chiaki-pybind-lib)
install(TARGETS chiaki-pybind)

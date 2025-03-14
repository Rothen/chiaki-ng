set(SOURCE
    include/chiaki-pybind.h
    src/py_discover.c
    src/py_wakeup.c)

add_library(chiaki-pybind-lib STATIC ${SOURCE})
target_include_directories(chiaki-pybind-lib PUBLIC "include")
target_link_libraries(chiaki-pybind-lib chiaki-lib)

if(NOT DEPS_FOLDER)
    set(DEPS_FOLDER ${CMAKE_SOURCE_DIR}/deps)
endif(NOT DEPS_FOLDER)

# Include directories
find_package(Python REQUIRED COMPONENTS Interpreter Development)
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
# find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(fmt REQUIRED)

set(PYBIND11_PYTHON_VERSION 3.11)
include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    SOURCE_DIR "${DEPS_FOLDER}/pybind11"
    GIT_TAG v2.11.1
)
FetchContent_MakeAvailable(pybind11)
include_directories(${DEPS_FOLDER}/pybind11)
include_directories(${CMAKE_SOURCE_DIR}/pybind/include)

# Source files
set(SOURCES
    include/py_controllermanager.h
    include/py_elapsed_timer.h
    include/py_exception.h
    include/py_host.h
    include/py_sessionlog.h
    include/py_settings.h
    include/py_streamsession.h
    include/py_timer.h
    src/py_host.cpp
    src/py_controllermanager.cpp
    src/py_sessionlog.cpp
    src/py_settings.cpp
    src/py_streamsession.cpp
    src/bindings.cpp
)

# Create the shared library (Python module)
pybind11_add_module(chiaki_py ${SOURCES})

# Link against chiaki-lib
target_link_libraries(chiaki_py PRIVATE chiaki-pybind-lib chiaki-lib fmt::fmt)

if(WIN32)
    target_link_libraries(chiaki-pybind-lib ws2_32)
    target_link_libraries(chiaki_py PRIVATE wsock32 ws2_32 bcrypt iphlpapi)
endif()

add_custom_command(TARGET chiaki_py POST_BUILD
    VERBATIM COMMAND python -c "import os; import sys; sys.path.insert(1, os.getcwd()); from pybind11_stubgen import main; import chiaki_py; sys.argv[0] = 'pybind11-stubgen'; sys.argv.append('--ignore-all-errors'); sys.argv.append('-o'); sys.argv.append('.'); sys.argv.append('chiaki_py'); main()" ||(exit 0)
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Creating stubs for MC3D-TRECSIM"
)
# set(SOURCE
#     include/chiaki-pybind.h
#     src/py_discover.c
#     src/py_wakeup.c)

# add_library(chiaki-pybind-lib STATIC ${SOURCE})
# target_include_directories(chiaki-pybind-lib PUBLIC "include")
# target_link_libraries(chiaki-pybind-lib chiaki-lib)

# find_package(Python REQUIRED COMPONENTS Interpreter Development)
# find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

# set(PYBIND11_PYTHON_VERSION 3.10)
# include(FetchContent)
# FetchContent_Declare(
#     pybind11
#     GIT_REPOSITORY https://github.com/pybind/pybind11.git
#     SOURCE_DIR "${DEPS_FOLDER}/pybind11"
#     GIT_TAG v2.11.1
# )
# FetchContent_MakeAvailable(pybind11)
# include_directories(lib/pybind11/include)

# find_package(Boost REQUIRED COMPONENTS filesystem)
# find_package(fmt REQUIRED)
# pybind11_add_module(chiaki_py src/bindings.cpp src/bindings.cpp src/py_controllermanager.cpp src/py_host.cpp src/py_sessionlog.cpp src/py_settings.cpp src/py_streamsession.cpp)
# target_include_directories(chiaki_py PUBLIC "include")
# target_link_libraries(chiaki_py PRIVATE chiaki-lib chiaki-pybind-lib Boost::filesystem fmt::fmt)

# if(WIN32)
#     target_link_libraries(chiaki-pybind-lib ws2_32)
#     target_link_libraries(chiaki_py PRIVATE ws2_32)
# endif()

# target_compile_definitions(chiaki_py PRIVATE VERSION_INFO=${EXAMPLE_VERSION_INFO})
# list(APPEND PYTHONPATH "${CMAKE_CURRENT_BINARY_DIR}")

# add_custom_command(TARGET chiaki_py POST_BUILD
#     VERBATIM COMMAND python -c "import os; import sys; sys.path.insert(1, os.getcwd()); from pybind11_stubgen import main; import chiaki_py; sys.argv[0] = 'pybind11-stubgen'; sys.argv.append('--ignore-all-errors'); sys.argv.append('-o'); sys.argv.append('.'); sys.argv.append('chiaki_py'); main()" ||(exit 0)
#     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#     COMMENT "Creating stubs for MC3D-TRECSIM"
# )


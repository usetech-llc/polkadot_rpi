# General project setup
cmake_minimum_required (VERSION 3.1)
project (PolkadotGUI C CXX)
message("Building project ${PROJECT_NAME}")

include(FindPkgConfig)

# Set compiler flags
set(CMAKE_C_FLAGS "-ggdb -ldl")
set(CMAKE_BUILD_TYPE Debug)

# Setup source files directories
file(GLOB_RECURSE src ${PROJECT_SOURCE_DIR}/src/*.cpp)

# Setup output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


###############################################################################
# Dependencies:
#   - Polka C++ API
#   - GTK
#   - libpthread
#   - Boost
#   - OpenSSL
#

set (WEBSOCKETPP_BOOST_LIBS system thread)
set (Boost_FIND_REQUIRED TRUE)
set (Boost_FIND_QUIETLY TRUE)
set (Boost_DEBUG FALSE)
set (Boost_USE_MULTITHREADED TRUE)
set (Boost_ADDITIONAL_VERSIONS "1.39.0" "1.40.0" "1.41.0" "1.42.0" "1.43.0" "1.44.0" "1.46.1") # todo: someone who knows better specify these!
find_package (Boost 1.39.0 COMPONENTS "${WEBSOCKETPP_BOOST_LIBS}")
if (NOT Boost_FOUND)
    message (FATAL_ERROR "Failed to find required dependency: boost")
endif ()

find_package(OpenSSL)
if (NOT OpenSSL_FOUND)
    message (FATAL_ERROR "Failed to find required dependency: OpenSSL")
endif ()

# Take care of libpthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(PkgConfig)
PKG_CHECK_MODULES(GTK3 REQUIRED gtk+-3.0)
INCLUDE_DIRECTORIES(${GTK3_INCLUDE_DIRS})
LINK_DIRECTORIES(${GTK3_LIBRARY_DIRS})
ADD_DEFINITIONS(${GTK3_CFLAGS_OTHER})

LINK_DIRECTORIES("/usr/lib/polkacpp")

include_directories ("/usr/include/polkacpp")
include_directories (${Boost_INCLUDE_DIR})

set (DEPENDENCY_LIBS polkacpp ${Boost_LIBRARIES} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY} ${GTK3_LIBRARIES} Threads::Threads sr25519crust dl)

# End of Dependencies
#########################################################


###############################################################################
# GUI Executable

# Build example sources as executables
add_executable( polkaui ${src} )

# Link dependency libraries to example project
target_link_libraries( polkaui ${DEPENDENCY_LIBS} )

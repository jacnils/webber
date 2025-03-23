cmake_minimum_required(VERSION 3.30)
project(webber)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(WEBBER_ENABLE_SQLITE ON)
set(WEBBER_ENABLE_POSTGRESQL ON)
set(PROJECT_SOURCE_FILES
        src/main.cpp
        src/db_abstract.cpp
        src/pages.cpp
        src/scrypto.cpp
        src/account.cpp
        src/file.cpp
)

include_directories(include)
include_directories(libs)
add_subdirectory(libs/libbcrypt)
include_directories(libs/libbcrypt/include)
include_directories(libs/limhamn/include)
include_directories(libs/maddy/include)

find_package(yaml-cpp REQUIRED)
find_package(Boost REQUIRED CONFIG COMPONENTS system)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_compile_definitions(LIMHAMN_DATABASE_ICONV)
if (WEBBER_ENABLE_SQLITE)
    add_compile_definitions(WEBBER_ENABLE_SQLITE)
    add_compile_definitions(LIMHAMN_DATABASE_SQLITE3)
    find_package(SQLite3 REQUIRED)
endif()
if (WEBBER_ENABLE_POSTGRESQL)
    add_compile_definitions(WEBBER_ENABLE_POSTGRESQL)
    add_compile_definitions(LIMHAMN_DATABASE_POSTGRESQL)
    find_package(PostgreSQL REQUIRED)
endif()
if (!WEBBER_ENABLE_SQLITE AND !WEBBER_ENABLE_POSTGRESQL)
    message(FATAL_ERROR "No database backend enabled")
endif()

add_compile_definitions(WEBBER_VERSION="${PROJECT_VERSION}")
add_compile_definitions(LIMHAMN_LOGGER_IMPL)
add_compile_definitions(LIMHAMN_ARGUMENT_MANAGER_IMPL)
add_compile_definitions(LIMHAMN_DATABASE_IMPL)
add_compile_definitions(LIMHAMN_HTTP_SERVER_IMPL)
add_compile_definitions(LIMHAMN_HTTP_UTILS_IMPL)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(WEBBER_DEBUG)
endif()

add_executable(webber ${PROJECT_SOURCE_FILES})
target_link_libraries(webber
    PRIVATE
    bcrypt
    yaml-cpp::yaml-cpp
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)
if (WEBBER_ENABLE_SQLITE)
    target_link_libraries(webber PRIVATE SQLite::SQLite3)
endif()
if (WEBBER_ENABLE_POSTGRESQL)
    target_link_libraries(webber PRIVATE PostgreSQL::PostgreSQL)
endif()
# some systems require explicit linking to iconv, others don't
# this is a bit hacky but seems to work well on both ubuntu and macos
find_library(ICONV_LIB iconv)
if (ICONV_LIB)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${ICONV_LIB})
endif()

# in include/prebuilt.hpp, replace:
# @INDEX_PAGE@ -> contents of prebuilt/index_page.html
# @SETUP_PAGE@ -> contents of prebuilt/setup_page.html
# @INDEX_JAVASCRIPT@ -> contents of prebuilt/main.js
# @INDEX_STYLESHEET@ -> contents of prebuilt/main.css
file(READ "${CMAKE_SOURCE_DIR}/prebuilt/index_page.html" INDEX_PAGE)
file(READ "${CMAKE_SOURCE_DIR}/prebuilt/setup_page.html" SETUP_PAGE)
file(READ "${CMAKE_SOURCE_DIR}/prebuilt/main.js" INDEX_JAVASCRIPT)
file(READ "${CMAKE_SOURCE_DIR}/prebuilt/main.css" INDEX_STYLESHEET)

set(TEMPLATE_FILE "${CMAKE_SOURCE_DIR}/include/prebuilt.hpp.in")
file(READ "${TEMPLATE_FILE}" TEMPLATE_CONTENTS)
string(REPLACE "@INDEX_PAGE@" "${INDEX_PAGE}" TEMPLATE_CONTENTS "${TEMPLATE_CONTENTS}")
string(REPLACE "@SETUP_PAGE@" "${SETUP_PAGE}" TEMPLATE_CONTENTS "${TEMPLATE_CONTENTS}")
string(REPLACE "@INDEX_JAVASCRIPT@" "${INDEX_JAVASCRIPT}" TEMPLATE_CONTENTS "${TEMPLATE_CONTENTS}")
string(REPLACE "@INDEX_STYLESHEET@" "${INDEX_STYLESHEET}" TEMPLATE_CONTENTS "${TEMPLATE_CONTENTS}")

file(WRITE "${CMAKE_BINARY_DIR}/include/prebuilt.hpp" "${TEMPLATE_CONTENTS}")
include_directories("${CMAKE_BINARY_DIR}/include")

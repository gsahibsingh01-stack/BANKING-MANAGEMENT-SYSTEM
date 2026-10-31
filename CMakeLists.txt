cmake_minimum_required(VERSION 3.15)
project(ApexBankingManagementSystem C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    dependencies
    dependencies/sqlite
    backend/include
)

set(SOURCES
    backend/src/main.cpp
    backend/src/database.cpp
    backend/src/auth.cpp
    backend/src/controllers.cpp
    dependencies/sqlite/sqlite3.c
)

add_executable(bank_server ${SOURCES})

if (WIN32)
    target_link_libraries(bank_server PRIVATE ws2_32 crypt32 advapi32)
else()
    target_link_libraries(bank_server PRIVATE pthread dl)
endif()

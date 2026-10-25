cmake_minimum_required(VERSION 3.20)

project(RSA
    VERSION 1.0.0
    LANGUAGES C
)

# Use standard C17
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Generate compile_commands.json for editor tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable testing
include(CTest)

# -----------------------
# Big Integer Library
# -----------------------

add_library(bigint STATIC
    src/bigint/bigint.c
)

target_include_directories(bigint
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_options(bigint
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
)

# -----------------------
# RSA Executable
# -----------------------

# Build the RSA executable
add_executable(rsa
    src/main.c
    src/rsa.c
)

# Make headers accessible
target_include_directories(rsa
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(rsa
    PRIVATE
        bigint
)

# Enable useful compiler warnings
target_compile_options(rsa
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
)

if(BUILD_TESTING)

    # RSA Tests
    add_executable(test_rsa
        tests/test_rsa.c
        src/rsa.c
    )


    target_include_directories(test_rsa
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    target_link_directories(test_rsa
        PRIVATE
            bigint
    )


    target_compile_options(test_rsa
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
    )

    add_test(NAME rsa_tests COMMAND test_rsa)

    # Big Integer Tests
    add_executable(test_bigint
        tests/test_bigint.c
        src/bigint/bigint.c
    )

    target_include_directories(test_bigint
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
    )

    target_link_libraries(test_bigint
        PRIVATE
            bigint
    )

    add_test(NAME bigint_tests COMMAND test_bigint)

endif()
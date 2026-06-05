cmake_minimum_required(VERSION 3.16)
project(NitroDrive LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
    SFML
    URL                   https://codeload.github.com/SFML/SFML/zip/refs/tags/2.6.1
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    # Brak URL_HASH — hash GitHub ZIP nie jest stabilny miedzy pobraniami
)
set(SFML_BUILD_AUDIO      OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK    OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC        OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES   OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SFML)


set(SOURCES
    src/main.cpp
    src/Difficulty.hpp
    src/SaveSystem.cpp
    src/SaveSystem.hpp
)

add_executable(NitroDrive ${SOURCES})

target_include_directories(NitroDrive PRIVATE src)

target_link_libraries(NitroDrive PRIVATE
    sfml-graphics
    sfml-window
    sfml-system
)


if(WIN32)
    set_target_properties(NitroDrive PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
endif()


if(WIN32)
    add_custom_command(TARGET NitroDrive POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:sfml-graphics>
            $<TARGET_FILE:sfml-window>
            $<TARGET_FILE:sfml-system>
            $<TARGET_FILE_DIR:NitroDrive>
        COMMENT "Kopiowanie DLL SFML..."
    )
endif()


add_custom_command(TARGET NitroDrive POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:NitroDrive>/assets"
    COMMENT "Kopiowanie assets..."
)

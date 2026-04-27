cmake_minimum_required(VERSION 3.21)

project(camera_manager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network OpenGL OpenGLWidgets)
find_package(OpenGL REQUIRED)

set(SPINNAKER_ROOT "" CACHE PATH "Root directory of a Spinnaker SDK installation")

set(_camera_manager_use_system_spinnaker_headers_default OFF)
if(APPLE)
    set(_camera_manager_use_system_spinnaker_headers_default ON)
elseif(SPINNAKER_ROOT AND (
    EXISTS "${SPINNAKER_ROOT}/include/spinnaker/System.h" OR
    EXISTS "${SPINNAKER_ROOT}/include/Spinnaker/System.h"
))
    set(_camera_manager_use_system_spinnaker_headers_default ON)
endif()

option(CAMERA_MANAGER_USE_SYSTEM_SPINNAKER_HEADERS
    "Use Spinnaker headers from the installed SDK instead of the bundled headers in this repository"
    ${_camera_manager_use_system_spinnaker_headers_default}
)

set(SPINNAKER_BUNDLED_INCLUDE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(SPINNAKER_BUNDLED_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")

file(GLOB CAMERA_MANAGER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3DProject/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/3DProject/*.h"
)

list(APPEND CAMERA_MANAGER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/CameraManager/cameramanager.qrc"
)

qt_add_executable(camera_manager
    ${CAMERA_MANAGER_SOURCES}
)

target_include_directories(camera_manager PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)

target_link_libraries(camera_manager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

if(TARGET OpenGL::GL)
    target_link_libraries(camera_manager PRIVATE OpenGL::GL)
elseif(TARGET OpenGL::OpenGL)
    target_link_libraries(camera_manager PRIVATE OpenGL::OpenGL)
endif()

function(camera_manager_copy_runtime_glob target pattern)
    file(GLOB _camera_manager_runtime_matches ${pattern})
    foreach(_camera_manager_runtime_file IN LISTS _camera_manager_runtime_matches)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_camera_manager_runtime_file}"
                    "$<TARGET_FILE_DIR:${target}>"
        )
    endforeach()
endfunction()

set(_spinnaker_include_search_paths)
set(_spinnaker_library_search_paths)
set(_spinnaker_runtime_search_paths)

if(SPINNAKER_ROOT)
    list(APPEND _spinnaker_include_search_paths
        "${SPINNAKER_ROOT}"
        "${SPINNAKER_ROOT}/include"
    )
    list(APPEND _spinnaker_library_search_paths
        "${SPINNAKER_ROOT}/lib"
        "${SPINNAKER_ROOT}/lib64"
        "${SPINNAKER_ROOT}/lib64/vs2017"
        "${SPINNAKER_ROOT}/lib64/vs2019"
        "${SPINNAKER_ROOT}/lib64/vs2022"
    )
    list(APPEND _spinnaker_runtime_search_paths
        "${SPINNAKER_ROOT}/bin"
        "${SPINNAKER_ROOT}/bin64"
        "${SPINNAKER_ROOT}/bin64/vs2017"
        "${SPINNAKER_ROOT}/bin64/vs2019"
        "${SPINNAKER_ROOT}/bin64/vs2022"
        "${SPINNAKER_ROOT}/lib"
        "${SPINNAKER_ROOT}/lib64"
    )
endif()

if(CAMERA_MANAGER_USE_SYSTEM_SPINNAKER_HEADERS)
    find_path(SPINNAKER_SYSTEM_INCLUDE_DIR
        NAMES spinnaker/System.h Spinnaker/System.h
        PATHS ${_spinnaker_include_search_paths}
            /usr/local/include
            /opt/homebrew/include
        REQUIRED
    )
    target_compile_definitions(camera_manager PRIVATE CAMERA_MANAGER_USE_SYSTEM_SPINNAKER_HEADERS=1)
    target_include_directories(camera_manager PRIVATE "${SPINNAKER_SYSTEM_INCLUDE_DIR}")
    if(EXISTS "${SPINNAKER_SYSTEM_INCLUDE_DIR}/spinnaker")
        target_include_directories(camera_manager PRIVATE "${SPINNAKER_SYSTEM_INCLUDE_DIR}/spinnaker")
    endif()
    if(EXISTS "${SPINNAKER_SYSTEM_INCLUDE_DIR}/Spinnaker")
        target_include_directories(camera_manager PRIVATE "${SPINNAKER_SYSTEM_INCLUDE_DIR}/Spinnaker")
    endif()
else()
    target_include_directories(camera_manager PRIVATE
        "${SPINNAKER_BUNDLED_INCLUDE_ROOT}"
        "${SPINNAKER_BUNDLED_INCLUDE_ROOT}/include/Spinnaker"
        "${SPINNAKER_BUNDLED_INCLUDE_ROOT}/include/FlyCapture"
    )
endif()

if(WIN32)
    find_library(SPINNAKER_LIBRARY
        NAMES Spinnaker_v143 Spinnaker_v142 Spinnaker_v141 Spinnaker_v140 Spinnaker
        PATHS ${_spinnaker_library_search_paths} "${SPINNAKER_BUNDLED_LIB_DIR}"
        REQUIRED
    )
    target_link_libraries(camera_manager PRIVATE
        "${SPINNAKER_LIBRARY}"
        glu32
        opengl32
    )

    foreach(_spinnaker_runtime_dir IN LISTS _spinnaker_runtime_search_paths)
        if(EXISTS "${_spinnaker_runtime_dir}")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/Spinnaker*.dll")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/GenApi*.dll")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/GCBase*.dll")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/log4cpp*.dll")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/Log*.dll")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/MathParser*.dll")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/NodeMapData*.dll")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/XmlParser*.dll")
        endif()
    endforeach()
elseif(APPLE)
    target_compile_definitions(camera_manager PRIVATE GL_SILENCE_DEPRECATION)
    list(APPEND _spinnaker_library_search_paths
        /usr/local/lib
        /opt/homebrew/lib
    )
    list(APPEND _spinnaker_runtime_search_paths
        /usr/local/lib
        /opt/homebrew/lib
        /opt/homebrew/opt/libomp/lib
    )
    set_target_properties(camera_manager PROPERTIES
        BUILD_RPATH "${_spinnaker_runtime_search_paths}"
        INSTALL_RPATH "${_spinnaker_runtime_search_paths}"
    )

    find_library(SPINNAKER_LIBRARY
        NAMES Spinnaker
        PATHS ${_spinnaker_library_search_paths}
        REQUIRED
    )
    target_link_libraries(camera_manager PRIVATE "${SPINNAKER_LIBRARY}")

    foreach(_spinnaker_runtime_dir IN LISTS _spinnaker_runtime_search_paths)
        if(EXISTS "${_spinnaker_runtime_dir}")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/libSpinnaker*.dylib")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/libGenApi*.dylib")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/libGCBase*.dylib")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/liblog4cpp*.dylib")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/libLog*.dylib")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/libMathParser*.dylib")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/libNodeMapData*.dylib")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/libXmlParser*.dylib")
        endif()
    endforeach()
else()
    list(APPEND _spinnaker_library_search_paths
        "${SPINNAKER_BUNDLED_LIB_DIR}"
        /usr/local/lib
        /usr/lib
        /opt/spinnaker/lib
        /opt/spinnaker/lib64
    )
    list(APPEND _spinnaker_runtime_search_paths
        "${SPINNAKER_BUNDLED_LIB_DIR}"
        /usr/local/lib
        /opt/spinnaker/lib
        /opt/spinnaker/lib64
    )
    set_target_properties(camera_manager PROPERTIES
        BUILD_RPATH "${_spinnaker_runtime_search_paths}"
        INSTALL_RPATH "${_spinnaker_runtime_search_paths}"
    )

    find_library(SPINNAKER_LIBRARY
        NAMES Spinnaker
        PATHS ${_spinnaker_library_search_paths}
        REQUIRED
    )
    target_link_libraries(camera_manager PRIVATE
        "${SPINNAKER_LIBRARY}"
    )

    if(TARGET OpenGL::GLU)
        target_link_libraries(camera_manager PRIVATE OpenGL::GLU)
    else()
        target_link_libraries(camera_manager PRIVATE GLU)
    endif()

    foreach(_spinnaker_runtime_dir IN LISTS _spinnaker_runtime_search_paths)
        if(EXISTS "${_spinnaker_runtime_dir}")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/libSpinnaker.so*")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/libGenApi*.so*")
            camera_manager_copy_runtime_glob(camera_manager "${_spinnaker_runtime_dir}/libGCBase*.so*")
        endif()
    endforeach()
endif()

add_custom_command(TARGET camera_manager POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/CameraManager"
            "$<TARGET_FILE_DIR:camera_manager>/CameraManager"
)

install(TARGETS camera_manager
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/CameraManager"
    DESTINATION .
)

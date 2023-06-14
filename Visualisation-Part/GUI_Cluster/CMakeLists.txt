cmake_minimum_required(VERSION 3.14)

project(GUI_Cluster VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Quick Quick3D)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Quick Quick3D)

set(PROJECT_SOURCES
    main.cpp
    mongodata.h
    mongodata.cpp
    cluster_node.h
    cluster_node.cpp
    cluster_core.h
    cluster_core.cpp
    database_connection.h
    database_connection.cpp
    main.qml
    Cores2D.qml
    Cores3D.qml
    Sidebar.qml
    3D-Scene/Lights.qml
)

QT_ADD_RESOURCES(RESOURCES_SOURCES resources.qrc)

qt_add_resources(QML_RESOURCES
        main.qml
        Cores2D.qml
        Cores3D.qml
        Sidebar.qml
        3D-Scene/Lights.qml
)

add_custom_target(GUI_Cluster_QmlFiles ALL
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/main.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/Cores2D.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/Cores3D.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/Sidebar.qml
        ${CMAKE_CURRENT_SOURCE_DIR}/3D-Scene/Lights.qml
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(GUI_Cluster
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET GUI_Cluster APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(GUI_Cluster SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(GUI_Cluster
          ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(GUI_Cluster
  PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Quick
  Qt${QT_VERSION_MAJOR}::Gui
  Qt${QT_VERSION_MAJOR}::Quick3D
)



install(TARGETS GUI_Cluster
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

find_package(mongocxx REQUIRED)
find_package(bsoncxx REQUIRED)
include_directories(${LIBMONGOCXX_INCLUDE_DIR})
include_directories(${LIBBSONCXX_INCLUDE_DIR})
include_directories("/usr/local/include/mongocxx/v_noabi")
include_directories("/usr/local/include/bsoncxx/v_noabi")
include_directories("/usr/local/include/libmongoc-1.0")
include_directories("/usr/local/include/libbson-1.0")
include_directories("/usr/local/lib")

target_link_libraries(GUI_Cluster PRIVATE mongo::bsoncxx_shared)
target_link_libraries(GUI_Cluster PRIVATE mongo::mongocxx_shared)


if(QT_VERSION_MAJOR EQUAL 6)
    qt_import_qml_plugins(GUI_Cluster)
    qt_finalize_executable(GUI_Cluster)
endif()






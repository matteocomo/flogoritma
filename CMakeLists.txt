cmake_minimum_required(VERSION 3.20)
project(Flogoritma VERSION 1.0.0 LANGUAGES CXX)

include(CTest)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Qml Svg)

add_library(flogoritma_nodes INTERFACE)
target_include_directories(flogoritma_nodes INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(flogoritma_nodes INTERFACE cxx_std_17)

add_executable(flogoritma_nodes_demo examples/nodes_demo.cpp)
target_link_libraries(flogoritma_nodes_demo PRIVATE flogoritma_nodes)

add_library(flogoritma_graphics src/area_disegno.cpp)
target_include_directories(flogoritma_graphics PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(flogoritma_graphics PUBLIC cxx_std_17)
target_link_libraries(flogoritma_graphics PUBLIC Qt6::Widgets)

add_executable(flogoritma_graphics_demo examples/graphics_demo.cpp)
target_link_libraries(flogoritma_graphics_demo PRIVATE flogoritma_graphics)

add_library(flogoritma_runtime src/interpreter.cpp)
target_include_directories(flogoritma_runtime PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_compile_features(flogoritma_runtime PUBLIC cxx_std_17)
target_link_libraries(flogoritma_runtime PUBLIC Qt6::Qml)

add_executable(flogoritma main.cpp src/editor_window.cpp assets/flogoritma.qrc)
set_target_properties(flogoritma PROPERTIES AUTORCC ON)
target_include_directories(flogoritma PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(flogoritma PRIVATE flogoritma_graphics flogoritma_runtime
    Qt6::Widgets Qt6::Svg
)

add_library(flogoritma_exporter src/code_exporter.cpp)
target_include_directories(flogoritma_exporter PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(flogoritma_exporter PUBLIC cxx_std_17)
target_link_libraries(flogoritma_exporter PUBLIC flogoritma_nodes)

add_executable(flogoritma_export_demo examples/export_demo.cpp)
target_link_libraries(flogoritma_export_demo PRIVATE flogoritma_exporter)

if(BUILD_TESTING)
    add_executable(flogoritma_exporter_tests tests/code_exporter_tests.cpp)
    target_link_libraries(flogoritma_exporter_tests PRIVATE flogoritma_exporter)
    add_test(NAME flogoritma_exporter_tests COMMAND flogoritma_exporter_tests)

    add_executable(flogoritma_model_tests tests/flowchart_model_tests.cpp)
    target_include_directories(flogoritma_model_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(flogoritma_model_tests PRIVATE Qt6::Core)
    add_test(NAME flogoritma_model_tests COMMAND flogoritma_model_tests)

    add_executable(flogoritma_graphics_tests tests/graphics_auto_connect_tests.cpp)
    target_link_libraries(flogoritma_graphics_tests PRIVATE flogoritma_graphics)
    add_test(NAME flogoritma_graphics_tests COMMAND flogoritma_graphics_tests)
    set_tests_properties(flogoritma_graphics_tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )

    add_executable(flogoritma_runtime_tests tests/interpreter_tests.cpp)
    target_include_directories(flogoritma_runtime_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(flogoritma_runtime_tests PRIVATE
        FLOGORITMA_EXAMPLE_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/esempio.flog\"
    )
    target_link_libraries(flogoritma_runtime_tests PRIVATE flogoritma_runtime)
    add_test(NAME flogoritma_runtime_tests COMMAND flogoritma_runtime_tests)
endif()

install(TARGETS flogoritma RUNTIME DESTINATION bin)
install(FILES assets/flogoritma.desktop DESTINATION share/applications)
install(FILES assets/flogoritma.svg DESTINATION share/icons/hicolor/scalable/apps)
install(FILES assets/flogoritma.xml DESTINATION share/mime/packages)
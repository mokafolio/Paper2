cmake_minimum_required (VERSION 2.8.11)
project (Paper)
set(CMAKE_CXX_FLAGS "-std=c++11 -fno-exceptions")

option(BuildSubmodules "BuildSubmodules" OFF)
option(AddTests "AddTests" ON)

find_package(OpenGL REQUIRED)

include_directories (${CMAKE_CURRENT_SOURCE_DIR} /usr/local/include /usr/local/include/pugixml-1.9 ${CMAKE_CURRENT_SOURCE_DIR}/Paper2/Libs)

link_directories(/usr/local/lib ${CMAKE_INSTALL_PREFIX}/lib /usr/local/lib/pugixml-1.9)

set (PAPERDEPS Stick pugixml ${OPENGL_LIBRARIES})

set (PAPERINC 
Paper2/BasicTypes.hpp
Paper2/Constants.hpp
Paper2/Document.hpp
Paper2/Group.hpp
Paper2/Item.hpp
Paper2/Paint.hpp
Paper2/Path.hpp
Paper2/RenderData.hpp
Paper2/RenderInterface.hpp
Paper2/Symbol.hpp
Paper2/Private/BooleanOperations.hpp
Paper2/Private/ContainerView.hpp
Paper2/Private/JoinAndCap.hpp
Paper2/Private/PathFitter.hpp
Paper2/Private/PathFlattener.hpp
Paper2/Private/Shape.hpp
Paper2/SVG/SVGExport.hpp
Paper2/SVG/SVGImport.hpp
Paper2/SVG/SVGImportResult.hpp
Paper2/Tarp/TarpRenderer.hpp
)

set (PAPERSRC 
Paper2/Document.cpp
Paper2/Group.cpp
Paper2/Item.cpp
Paper2/Paint.cpp
Paper2/Path.cpp
Paper2/RenderInterface.cpp
Paper2/Symbol.cpp
Paper2/Libs/GL/gl3w.c
Paper2/Private/BooleanOperations.cpp
Paper2/Private/JoinAndCap.cpp
Paper2/Private/PathFitter.cpp
Paper2/Private/PathFlattener.cpp
Paper2/Private/Shape.cpp
Paper2/SVG/SVGExport.cpp
Paper2/SVG/SVGImport.cpp
Paper2/SVG/SVGImportResult.cpp
Paper2/Tarp/TarpRenderer.cpp
)

#avoid opengl deprecation warnings on mojave +
if (APPLE)
add_compile_options(-D GL_SILENCE_DEPRECATION)
endif()

add_library (Paper2 SHARED ${PAPERSRC})
add_library (Paper2Static STATIC ${PAPERSRC})
target_link_libraries(Paper2 ${PAPERDEPS})
target_link_libraries(Paper2Static ${PAPERDEPS})

install (TARGETS Paper2 Paper2Static DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
foreach ( file ${PAPERINC} )
    get_filename_component( dir ${file} DIRECTORY )
    install( FILES ${file} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${dir} )
endforeach()

if(AddTests)
    add_subdirectory (Tests)
endif()

add_subdirectory (Playground)

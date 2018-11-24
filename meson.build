project('Paper2', 'cpp', 'c', default_options : ['cpp_std=c++14', 'c_std=c99'])

if meson.is_subproject() == false
    # disable this bogus warning (should be deprecated in newer clang versions)
    add_global_arguments('-Wno-missing-braces', '-fno-exceptions', language: 'cpp')
else
    message('Building Paper as subproject.')
endif

cppArgs = ['-DPUGIXML_NO_EXCEPTIONS']
if host_machine.system() == 'darwin'
    cppArgs += '-DGL_SILENCE_DEPRECATION'
endif

paperInc = [
    'Paper2/BasicTypes.hpp',
    'Paper2/Constants.hpp',
    'Paper2/Document.hpp',
    'Paper2/Group.hpp',
    'Paper2/Item.hpp',
    'Paper2/Paint.hpp',
    'Paper2/Path.hpp',
    'Paper2/RenderData.hpp',
    'Paper2/RenderInterface.hpp',
    'Paper2/Symbol.hpp'
]
install_headers(paperInc, subdir: 'Paper2')

paperPrivateInc = [
    'Paper2/Private/BooleanOperations.hpp',
    'Paper2/Private/ContainerView.hpp',
    'Paper2/Private/JoinAndCap.hpp',
    'Paper2/Private/PathFitter.hpp',
    'Paper2/Private/PathFlattener.hpp',
    'Paper2/Private/Shape.hpp'
]
install_headers(paperPrivateInc, subdir: 'Paper2/Private')

paperSVGInc = [
    'Paper2/SVG/SVGExport.hpp',
    'Paper2/SVG/SVGImport.hpp',
    'Paper2/SVG/SVGImportResult.hpp'
]
install_headers(paperSVGInc, subdir: 'Paper2/SVG')

paperPugiInc = [
    'Paper2/Libs/pugixml/pugixml.hpp',
    'Paper2/Libs/pugixml/pugiconfig.hpp'
]
install_headers(paperPugiInc, subdir: 'Paper2/Libs/pugixml')

install_headers('Paper2/Libs/GL/gl3w.h', subdir: 'Paper2/Libs/GL')
install_headers('Paper2/Tarp/TarpRenderer.hpp', subdir: 'Paper2/Tarp')


paperSrc = [
    'Paper2/Document.cpp',
    'Paper2/Group.cpp',
    'Paper2/Item.cpp',
    'Paper2/Paint.cpp',
    'Paper2/Path.cpp',
    'Paper2/RenderInterface.cpp',
    'Paper2/Symbol.cpp',
    'Paper2/Libs/GL/gl3w.c',
    'Paper2/Libs/pugixml/pugixml.cpp',
    'Paper2/Private/BooleanOperations.cpp',
    'Paper2/Private/JoinAndCap.cpp',
    'Paper2/Private/PathFitter.cpp',
    'Paper2/Private/PathFlattener.cpp',
    'Paper2/Private/Shape.cpp',
    'Paper2/SVG/SVGExport.cpp',
    'Paper2/SVG/SVGImport.cpp',
    'Paper2/SVG/SVGImportResult.cpp',
    'Paper2/Tarp/TarpRenderer.cpp'
]

cc = meson.get_compiler('cpp')
glDep = dependency('GL')
if get_option('buildSubmodules') == false
    stickDep = cc.find_library('Stick')
    paperDeps = [stickDep, dependency('threads'), glDep]
else
    stickProj = subproject('Stick')
    paperDeps = [stickProj.get_variable('stickDep'), glDep]
endif

incDirs = include_directories('.', 'Paper2/Libs', 'Paper2/Libs/pugixml')

paper = library('Paper2', 
        paperSrc,
        dependencies: paperDeps, 
        include_directories : incDirs, 
        cpp_args: cppArgs,
        install: true)

paperDep = declare_dependency(link_with : paper, 
    dependencies: paperDeps, 
    include_directories: incDirs)

# only build tests if not build as a subproject.
# Otherwise it seems like the test meson function uses the wrong tests???
if meson.is_subproject() == false
    subdir('Playground') #definately dont wanna build these in subproj case
    subdir('Tests')
endif

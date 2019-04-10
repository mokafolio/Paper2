playgrounds = [
    'NestedClipping',
    'PaperPlayground',
    'SVGExportPlayground',
    'SVGImportPlayground',
    'BinaryFormatPlayground'
    ]

deps = [paperDep, dependency('glfw3')]

#we manually need to link glfw3 dependencies...hmm :/
if host_machine.system() == 'darwin'
    deps += [dependency('Cocoa'), dependency('IOKit'), dependency('CoreFoundation'), dependency('CoreVideo')]
elif host_machine.system() == 'linux'
    cc = meson.get_compiler('c')
    deps += [dependency('x11'), 
            cc.find_library('dl', required : true),
            cc.find_library('m', required : true),
            dependency('threads')]
endif

foreach p : playgrounds
    pg = executable(p, p + '.cpp', 
            dependencies: deps, 
            cpp_args : cppArgs + ['-fsanitize=address'],
            link_args : '-fsanitize=address')
endforeach
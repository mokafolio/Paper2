tests = executable('PaperTests', 'PaperTests.cpp', 
    dependencies: paperDep, 
    include_directories : incDirs,
    cpp_args : '-fsanitize=address',
    link_args : '-fsanitize=address')
test('Paper Tests', tests, workdir: meson.current_build_dir())
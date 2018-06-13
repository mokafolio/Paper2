add_executable (PaperTests PaperTests.cpp)
target_link_libraries(PaperTests Paper2 ${PAPERDEPS})
add_custom_target(check COMMAND PaperTests)

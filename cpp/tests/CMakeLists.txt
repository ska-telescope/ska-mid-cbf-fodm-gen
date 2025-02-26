link_libraries(
  ${TARGET_LIB}
)
add_executable( fodm_test
  fodm_test.cpp
)
target_include_directories( fodm_test
	PRIVATE
	${PROJECT_SOURCE_DIR}
)
target_link_libraries(
  fodm_test
  GTest::gtest_main
)

gtest_discover_tests(fodm_test)
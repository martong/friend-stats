add_custom_target(FriendStatsUnitTest)
set_target_properties(FriendStatsUnitTest PROPERTIES FOLDER "Friend Statistics Unit Test")

function(add_friend_stats_unittest test_dirname)
  add_unittest(FriendStatsUnitTest ${test_dirname} ${ARGN})
endfunction()

add_friend_stats_unittest(FriendStatsSimpleTests
  FriendStatsTest.cpp
  )

target_link_libraries(FriendStatsSimpleTests
  clangTooling
  clangBasic
  clangASTMatchers
  )

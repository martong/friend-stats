set(LLVM_LINK_COMPONENTS support)
set(LLVM_USED_LIBS clangTooling clangBasic clangAST)

add_clang_executable(friend-stats
  FriendStats.cpp
  )
target_link_libraries(friend-stats
  clangTooling
  clangBasic
  clangASTMatchers
  )

add_subdirectory(ut)

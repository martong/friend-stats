# friend-stats

## Build
```
# First clone llvm and clang.
# You can find the llvm/clang versions I used, here:
# https://github.com/martong/friend-stats/blob/master/measures/results/clang_3eec7e6/version
cd llvm/tools/clang/tools/extra
git clone git@github.com:martong/friend-stats.git
echo "add_subdirectory(friend-stats)" >> CMakeLists.txt
```

## Usage 
```
friend-stats -p /path/to/compile_db /path/to/source/file
OR
friend-stats -p /path/to/compile_db -db dummy_argument
```
In case of segmentation fault try to increase the stack size.
E.g. on OSX set it to the maximum: 
`ulimit -s 65532`
RecursiveASTVisitor can eat up the stack in case of complicated program structures.

### Usage examples
#### Stats for one file
```
friend-stats -p . llvm/tools/clang/tools/extra/clang-modernize/LoopConvert/LoopActions.cpp
```
#### Stats for a whole build (uses compile db). Supply "-db" and a dummy argument. 
```
friend-stats -p . -db xyz 2>/dev/null | tee ../measure/2015_02_21/clang.result
```

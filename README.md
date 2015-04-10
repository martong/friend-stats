# friend-stats

## Build
```
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
### Usage examples
#### Stats for one file
```
friend-stats -p . llvm/tools/clang/tools/extra/clang-modernize/LoopConvert/LoopActions.cpp
```
#### Stats for a whole build (uses compile db). Supply "-db" and a dummy argument. 
```
friend-stats -p . -db xyz 2>/dev/null | tee ../measure/2015_02_21/clang.result
```

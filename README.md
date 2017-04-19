# Warnings and statistics about friend declarations

This tool can be used to collect some statistics and to provide warnings about incorrect friend declarations.
It is based on Clang's libTooling library.

## Usage

### Warnings
To get all the useful warnings for a project:
```
friend-stats -no_stats -if -host_classes_with_zero_priv -incorrect_friend_classes -db path_to_compile_db_of_your_project 2>/dev/null
```
The `-no_stats` instructs the tool to not emit any statistics, since we want to have warnings only.
The `-if` switch will result in printing all the friend functions which might not be friend because they do not access any private entities.
These functions might be declared as friend by mistake.
`-host_classes_with_zero_priv` lists all those classes which have at least one friend declaration but itself does not have any private entites.
These classes shall not have any friend declarations because all their members are public.
`-incorrect_friend_classes` displays all those friend classes whose all member functions do not access any private entities in the befriending classes.
These classes might be declared as friend by mistake.
With the `-db` switch we specify the path to the directory where the compilation_commands.json file is.
(See https://clang.llvm.org/docs/JSONCompilationDatabase.html).
To run only on a few files instead of all the files in the compilation db then use the `-p` switch instead of the `-db`:
```
friend-stats ... -p /path/to/compile_db /path/to/source/file
```

The output might contain entries like this:
```
Warning: possibly incorrect friend function instance:
befriending class: boost::asio::ip::address
friendly function: boost::asio::ip::operator!=
friendDeclLoc: ./boost/asio/ip/address.hpp:134:15
defLoc: ./boost/asio/ip/address.hpp:134:15
diagName: boost::asio::ip::operator!=
usedPrivateVarsCount: 0
parentPrivateVarsCount: 3
usedPrivateMethodsCount: 0
parentPrivateMethodsCount: 0
types.usedPrivateCount: 0
types.parentPrivateCount: 0
============================================================================
Warning: possibly incorrect friend class:
diagName: boost::detail::future_waiter
defLoc: ./boost/thread/future.hpp:1064:15
friendDeclLoc: ./boost/thread/future.hpp:1904:16
============================================================================
Warning: befriending class with zero private entities:
defLoc: ./boost/asio/detail/kqueue_reactor.hpp:59:10
diagName: boost::asio::detail::kqueue_reactor::descriptor_state
============================================================================
```

### Statistics
To get statistics about the friend usage:
```
friend-stats -db /path/to/compile_db
```

### Examples
Statstics for one file:
```
friend-stats -p . llvm/tools/clang/tools/extra/clang-modernize/LoopConvert/LoopActions.cpp
```
Statstics for a whole build (uses compile db):
```
friend-stats -db . 2>/dev/null | tee ../measure/2015_02_21/clang.result
```

### Problems
In case of segmentation fault, increase the stack size.
E.g. on OSX set it to the maximum:
`ulimit -s 65532`
RecursiveASTVisitor can eat up the stack in case of complicated program structures.

## Build
Clone llvm, clang, libcxx and clang-tools-extra from github.
Note https://clang.llvm.org/get_started.html.
Then checkout the appropriate versions:
```
mkdir ~/friends && cd ~/friends

git clone http://llvm.org/git/llvm.git
cd llvm
git checkout 895316336e4a774a47b1a4daada070e0ac97f46e

cd projects
git clone http://llvm.org/git/libcxx
git checkout 453a50040bb9cda453d85863570e2710c3616faa
cd -

cd tools
git clone http://llvm.org/git/clang.git
cd clang
git checkout 3eec7e69d00b4488fd766f9d2a5ee9cecc470a22
cd tools
git clone http://llvm.org/git/clang-tools-extra.git extra
cd extra
git checkout 4e38769a0097beb3a8d6f1715c176732b64d87fa
```
Clone this tool:
```
cd ~/friends/llvm/tools/clang/tools/extra
git clone git@github.com:martong/friend-stats.git
echo "add_subdirectory(friend-stats)" >> CMakeLists.txt
```
Setup and call Cmake. Note, this will build the unittest as well.
```
mkdir ~/friends/build
cd ~/friends/build
cmake ../llvm -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DLLVM_BUILD_TESTS=1
ninja
```

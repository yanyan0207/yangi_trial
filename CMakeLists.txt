project(yangi)
cmake_minimum_required(VERSION 3.17)

set(CMAKE_CXX_STANDARD 17)
 
add_definitions(-g)

 #ファイル検索:リストアップ ターケットファイルリストの生成
set(Target_Files
    Util.cpp
    test/read_csa.cpp
)

 #実行ファイルの生成
 add_executable(yangi ${Target_Files})
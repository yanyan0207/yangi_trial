project(yangi)
cmake_minimum_required(VERSION 3.17)

set(CMAKE_CXX_STANDARD 17)

add_definitions(-g)

# ファイル検索:リストアップ ターケットファイルリストの生成
set(Target_Files Util.cpp test/read_csa.cpp test/create_csa_list.cpp)

# 実行ファイルの生成
add_executable(create_csa_list ${Target_Files})

# ファイル検索:リストアップ ターケットファイルリストの生成
set(Target_Files Util.cpp test/read_csa.cpp test/create_feature.cpp)

# 実行ファイルの生成
add_executable(create_feature ${Target_Files})

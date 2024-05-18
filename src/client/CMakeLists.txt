# 定义了一个SRC_LIST变量，包含了该目录下所有的源文件
aux_source_directory(. SRC_LIST)

# 指定可生成文件
add_executable(ChatClient ${SRC_LIST} )

# 指定可执行文件链接时需要依赖的库文件
target_link_libraries(ChatClient pthread)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
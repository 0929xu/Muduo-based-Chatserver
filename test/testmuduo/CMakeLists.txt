# cmake的最低版本
cmake_minimum_required(VERSION 3.0)  
#
project(main)
# 假如一条编译指令是：
# g++ -o server -g muduo_server.cpp xxx.cpp -I/usr/lib -L/usr/lib -lmuduo_net -lmuduo_base -lpthread
#        ______ __ ________________________ __________ __________ __________________________________
#          1     2             3                 4          5                     6     

# 配置编译选项 上面的2
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g) # -g 表示编译出来的可执行文件是可调试的，没有-g非可调试

# 配置头文件搜索路径 上面的4
# include_directories()
# 配置库文件搜索路径 上面的5
# link_directories()

# 上面的3
# 设置需要编译的源文件列表
set(SRC_LIST ./muduo_server.cpp)  # ./muduo_server.cpp 表示当前路径下的muduo_server.cpp
# 假如有100源文件，都要像上面一样列出来吗，不用,用aux
# 把.指定路径下的所有源文件名字放入变量名SRC_LIST里面
# aux_source_directory(. SRC_LIST)

# 设置可执行文件最终存储的路径,PROJECT_SOURCE_DIR是工程的根目录,bin文件夹存储可执行文件
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 生成可执行文件：通过SRC_LIST定义的源文件编译生成可执行文件server
add_executable(server ${SRC_LIST})

# 链接库 上面的6 表示server这个目标程序，需要链接server muduo_net muduo_base pthread这三个库
target_link_libraries(server muduo_net muduo_base pthread)

# cmake文件的固定写法的就是CMakeList.txt，大小写一定要准确
# 执行的时候到该CMakeLists.txt文件的文件夹下，使用命令"cmake .",就会在当前目录下找CMakeLists.txt根据这里面配置的东西进行编译
# 编译出来就会得到Makefile文件（cmake的作用就是简化获取Makefile文件的过程，因为自己写Makefile非常复杂）
# 得到Makefile文件后就使用命令"make"，就会自动找Makefile文件，然后开始编译，进度100%之后就能得到我们需要的可执行文件
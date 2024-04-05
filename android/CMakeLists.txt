#[[
    @Copyright : Yangrongcan
]]
# cmake最低版本
cmake_minimum_required(VERSION 3.15)

#debug 模式
set(CMAKE_BUILD_TYPE Debug)

# set the project name and version
project(cwmpclient VERSION 1.0.0)
string(TIMESTAMP COMPILE_TIME %Y%m%d-%H%M%S)

# PROJECT_SOURCE_DIR 宏对应的值就是我们在使用cmake命令时，后面紧跟的目录，一般是工程的根目录
configure_file(${PROJECT_SOURCE_DIR}/../src/CwmpConfig.h.in ${PROJECT_SOURCE_DIR}/../include/CwmpConfig.h)

include_directories(${PROJECT_SOURCE_DIR}/../include ${PROJECT_SOURCE_DIR}/../include/cwmpclient ${PROJECT_SOURCE_DIR}/../include/data_model ${PROJECT_SOURCE_DIR}/../include/data_model/model)

# CMAKE_CURRENT_SOURCE_DIR 宏表示当前访问的 CMakeLists.txt 文件所在的路径
# src/cwmpclient下的所有文件
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../src/cwmpclient CWMPCLIENT_LIST)
# src下的所有文件
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../src UTILS_LIST)

# data_model下的所有.c文件
file(GLOB_RECURSE DATA_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../src/data_model/*.c)

#说明动态库存放位置
# link_directories(${PROJECT_SOURCE_DIR}/lib)

# 操作参数的程序
add_executable(cwmpdata ${DATA_SRC} ${UTILS_LIST})
include_directories(D:/project/curl-8.7.1/include)
link_directories(D:/project/curl-8.7.1/build/lib)
target_link_libraries(cwmpdata curl)

# 从DATA_SRC中移除指定文件
list(REMOVE_ITEM DATA_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/data_model/cwmpdata.c)

# add the executable 主程序
add_executable(${PROJECT_NAME} ${CWMPCLIENT_LIST} ${DATA_SRC} ${UTILS_LIST})
target_link_libraries(${PROJECT_NAME} curl)

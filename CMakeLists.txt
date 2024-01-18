#[[
    @Copyright : Yangrongcan
]]
# cmake最低版本
cmake_minimum_required(VERSION 3.15)

#debug 模式
set(CMAKE_BUILD_TYPE Debug)

# set the project name and version
project(cwmp VERSION 1.0.0)
string(TIMESTAMP COMPILE_TIME %Y%m%d-%H%M%S)

# PROJECT_SOURCE_DIR 宏对应的值就是我们在使用cmake命令时，后面紧跟的目录，一般是工程的根目录
configure_file(${PROJECT_SOURCE_DIR}/src/CwmpConfig.h.in ${PROJECT_SOURCE_DIR}/include/cwmp/CwmpConfig.h)

include_directories(${PROJECT_SOURCE_DIR}/include/cwmp ${PROJECT_SOURCE_DIR}/include/data_model ${PROJECT_SOURCE_DIR}/include/data_model/model)
# CMAKE_CURRENT_SOURCE_DIR 宏表示当前访问的 CMakeLists.txt 文件所在的路径
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_LIST)

#说明动态库存放位置
# link_directories(${PROJECT_SOURCE_DIR}/lib)

# add the executable 主程序
add_executable(${PROJECT_NAME} ${SRC_LIST})


# PROJECT_BINARY_DIR 表示当前工程的二进制路径，即编译产物会存放到该路径，此时PROJECT_BINARY_DIR 就是 build 所在路径

# 寻找 curl 和 xml2 库
find_package(CURL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)
if(CURL_FOUND)
    message(STATUS "CURL found")
endif()

find_package(LibXml2 REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE LibXml2::LibXml2)
if(LibXml2_FOUND)
    message(STATUS "LibXml2 found")
endif()

file(GLOB_RECURSE DATA_SRC ${CMAKE_CURRENT_SOURCE_DIR}/data_model/*.c)

# 操作参数的程序
add_executable(cwmpdata ${DATA_SRC})
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
configure_file(${PROJECT_SOURCE_DIR}/src/CwmpConfig.h.in ${PROJECT_SOURCE_DIR}/include/cwmpclient/CwmpConfig.h)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/include/cwmpclient ${PROJECT_SOURCE_DIR}/include/data_model ${PROJECT_SOURCE_DIR}/include/data_model/model)

# CMAKE_CURRENT_SOURCE_DIR 宏表示当前访问的 CMakeLists.txt 文件所在的路径
# src/cwmpclient下的所有文件
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/cwmpclient CWMPCLIENT_LIST)
# src下的所有文件
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src UTILS_LIST)

# data_model下的所有.c文件
file(GLOB_RECURSE DATA_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/data_model/*.c)

#说明动态库存放位置
# link_directories(${PROJECT_SOURCE_DIR}/lib)

# 操作参数的程序
add_executable(cwmpdata ${DATA_SRC} ${UTILS_LIST})
find_package(CURL REQUIRED)
target_link_libraries(cwmpdata PRIVATE CURL::libcurl)
if(CURL_FOUND)
    message(STATUS "CURL found")
endif()

# 从DATA_SRC中移除指定文件
list(REMOVE_ITEM DATA_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/data_model/cwmpdata.c)

# add the executable 主程序
add_executable(${PROJECT_NAME} ${CWMPCLIENT_LIST} ${DATA_SRC} ${UTILS_LIST})

# PROJECT_BINARY_DIR 表示当前工程的二进制路径，即编译产物会存放到该路径，此时PROJECT_BINARY_DIR 就是 build 所在路径

# 寻找 curl 和 xml2 库
find_package(CURL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)
# if(CURL_FOUND)
#     message(STATUS "CURL found")
# endif()

# find_package(LibXml2 REQUIRED)
# target_link_libraries(${PROJECT_NAME} PRIVATE LibXml2::LibXml2)
# if(LibXml2_FOUND)
#     message(STATUS "LibXml2 found")
# endif()

# find_package(tinyxml2 CONFIG REQUIRED)
# target_link_libraries(${PROJECT_NAME} PRIVATE tinyxml2::tinyxml2)

# find_package(cJSON CONFIG REQUIRED)
# target_link_libraries(${PROJECT_NAME} PRIVATE cjson)


# cmake -G "MinGW Makefiles" .. -DCMAKE_TOOLCHAIN_FILE=D:/development_environment/vcpkg/scripts/buildsystems/vcpkg.cmake
# cmake --build .
# .\cwmpdata.exe download https://dss0.bdstatic.com/5aV1bjqh_Q23odCf/static/superman/js/lib/esl-d776bfb1aa.js '3 Vendor Configuration File' 100 admin admin
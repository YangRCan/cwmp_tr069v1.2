## cwmp_tr069v1.2
TR069的1.2版本客户端程序的实现，参考了开源程序[easycwmp](https://easycwmp.org/)的设计理念, 当前程序只实现了TR069的CPE基本RPC方法和TR181基本数据模型，其中系统升级、恢复出厂、重启功能仅支持安卓平台。

## 项目中包含有两个CMakeLists.txt文件
您可将其中一个拷贝到项目根路径下并创建build文件夹在其中进行编译。
其中 **${项目根路径}/cmakelists/CMakeLists.txt**，是与vcpkg配合使用的版本，目前只需要libcurl动态库(其他运用的库cJSON、tinyXML2已将源码包含在项目中)，您可下载vcpkg并且用vcpkg安装libcurl库，请确保vcpkg使用的编译工具链，与您构建本项目时所使用的编译工具链能兼容。用cmake构建和编译的命令大致如下(仅供参考，请以您实际使用的工具和平台为准)：<br>`cmake -G "MinGW Makefiles" .. -DCMAKE_TOOLCHAIN_FILE=D:/development_environment/vcpkg/scripts/buildsystems/vcpkg.cmake`<br>`cmake --build .`

**${项目根路径}/android/CMakeLists.txt**,是通用的编译脚本，本人用Android NDK配合该cmake脚本交叉编译本项目, ${项目根路径}/android 下已经有编译好的libcurl.so(注意此版本**不支持openssl**)。此外还请注意可能需要修改该文件中 `include_directories(D:/project/curl-8.7.1/include)` 和 `target_link_directories(cwmpdata PUBLIC D:/project/curl-8.7.1/build/lib)` 所连接的动态库位置和头文件位置。<br>**建议:** 交叉编译使用cmake-gui会更方便。

## 编译结果
本项目编译成功后会输出两个可执行程序：**cwmpclient**和**cwmpdata**。cwmpclient为网管程序客户端，主要负责与ACS通信，定时Inform，和处理ACS调用的远程RPC方法。cwmpdata为CPE端命令行操作程序，主要用于修改或查看本地参数，及部分本地方法。

## ${项目根路径}/sample文件夹
该文件夹下展示了可执行程序和执行程序时所需的文件。默认运行时所需的环境结构为sample下所示，可依据运行平台在源码中进行调整。config文件需要手动创建，并按sample中的格式填入您设备信息及ACS的信息。
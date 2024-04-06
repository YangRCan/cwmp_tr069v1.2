## cwmp_tr069v1.2
TR069的1.2版本客户端程序的实现，参考了开源程序 [easycwmp](https://easycwmp.org/) 的设计理念, 当前程序只实现了TR069的CPE基本RPC方法和TR181基本数据模型，其中系统升级、恢复出厂、重启功能仅支持安卓平台。

## 项目中包含有两个CMakeLists.txt文件
您可将其中一个拷贝到项目根路径下并创建build文件夹在其中进行编译。
其中 **${项目根路径}/cmakelists/CMakeLists.txt**，是与vcpkg配合使用的版本，目前只需要libcurl动态库(其他运用的库cJSON、tinyXML2已将源码包含在项目中)，您可下载vcpkg并且用vcpkg安装libcurl库，请确保vcpkg使用的编译工具链，与您构建本项目时所使用的编译工具链能兼容。用cmake构建和编译的命令大致如下(仅供参考，请以您实际使用的工具和平台为准)：
`cmake -G "MinGW Makefiles" .. -DCMAKE_TOOLCHAIN_FILE=D:/development_environment/vcpkg/scripts/buildsystems/vcpkg.cmake`
`cmake --build .`

**${项目根路径}/android/CMakeLists.txt**,是通用的编译脚本，本人用Android NDK配合该cmake脚本交叉编译本项目, ${项目根路径}/android 下已经有编译好的libcurl.so(注意此版本**不支持openssl**)
**建议:** 交叉编译使用cmake-gui会更方便。

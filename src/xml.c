/**
 * @Copyright : Yangrongcan
*/
#include <stdio.h>
#include "xml.h"

// 定义一个通用的函数指针类型
typedef void (*GenericFunctionPointer)();

// 示例函数1
void function1() {
    printf("Function 1 called\n");
}

// 示例函数2
void function2() {
    printf("Function 2 called\n");
}

// 示例函数3，带参数和返回类型
void functionWithParams(int a, int b, char *str) {
    printf("Function with parameters called: %d + %d = %d\n", a, b, a + b);
    printf("字符串：%s", str);
}

int testXML2() {
    // 使用通用的函数指针
    GenericFunctionPointer genericFunction;

    // 将通用函数指针指向不同的函数
    genericFunction = function1;
    genericFunction();  // 调用函数1

    genericFunction = function2;
    genericFunction();  // 调用函数2

    // 将通用函数指针指向带参数和返回类型的函数
    // 注意：需要确保参数和返回类型匹配
    genericFunction = functionWithParams;
    // genericFunction();  // 这里调用时并不提供参数，实际上是未定义行为

    genericFunction(2,5,"测试");
    return 0;
}

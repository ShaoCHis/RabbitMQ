#include <iostream>
#include <ctime>

//封装一个日志宏，通过日志宏进行日志的打印，在打印的信息前带有系统时间以及文件名和行号
//      [17:25:23] [log.cpp:12] 打开文件失败！

#define DBG_LEVEL 0
#define INF_LEVEL 1
#define ERR_LEVEL 2

#define DEFAULT_LEVEL DBG_LEVEL
//...宏定义中的不定参数
// ## __VA_ARGS__ 不定参数展开，记得加上两个##----->解决如果没有参数，忽略前面的,（逗号）否则会编译错误
#define LOG(lev_str,level,format,...){\
    if(level>=DEFAULT_LEVEL){\
        time_t t = time(nullptr);\
        struct tm* ptm = localtime(&t);\
        char time_str[32];\
        strftime(time_str,31,"%H:%M:%S",ptm);\
        printf("[%s][%s][%s:%d]\t" format "\n",lev_str,time_str,__FILE__,__LINE__,## __VA_ARGS__);\
    }\
}

#define DLOG(format,...) LOG("DBG",DBG_LEVEL,format,## __VA_ARGS__)
#define ILOG(format,...) LOG("INF",INF_LEVEL,format,## __VA_ARGS__)
#define ELOG(format,...) LOG("ERR",ERR_LEVEL,format,## __VA_ARGS__)

int main(){
    // time_t t = time(nullptr);
    // //获取系统时间
    // struct tm* ptm = localtime(&t);
    // char time_str[32];
    // //空一个字节保存 \0
    // strftime(time_str,31,"%H:%M:%S",ptm);

    // //__FILE__,__LINE__ 编译器所做的工作，宏替换
    // printf("[%s][%s:%d] Hello World!\n",time_str,__FILE__,__LINE__);
    DLOG("hello %s -- %d","world",20);
    return 0;
}
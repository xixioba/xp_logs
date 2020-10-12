#ifndef XP_LOGS_H
#define XP_LOGS_H

#include "xp_ringbuff.h"
#include <thread>
class xp_logs {
    Ringbuf* cachebuff;
    int Cachesize;
    int Daily;
    char logpath[512];
    int maxtotalsize = 10 * 1024 * 1024; //单位 K,默认，10*1024*1024=10GB
    int maxlogsize = 1 * 1024 * 1024; //单位 K，默认，1*1024*1024=1GB
    char Suffix[512];
    std::thread sub_thread;

public:
    xp_logs(
        const char* path, const char* suffix = ".log", int daily = 1,
        int cachesize = 30 * 1024 * 1024); // daily{0,1}:按天重置新日志,单位BYTE,cachesize=0,普通模式，cachesize>0,启用cachepool，数据将存入缓存中，稍后自动写入,保证缓存大小可靠，减少丢包
    ~xp_logs();
    void run();
    int add(char* str, int len = 0); // len==0,写入字符串，len>0，写入char *数组
    int get_size(char* filename); //获取文件大小
    int get_dir_size(char* path = NULL); //获取文件夹大小
    int list_files(char* path); //索引当前文件夹下所有文件
    int list_all_files(char* basePath); //索引所有文件，逐层
    int creat_log(char* path = NULL); //在默认路径下创建日期命名的文件或指定名称
    char* get_first_file(char* path); //获取指定目录下第一个文件名
    char* get_last_file(char* path); //获取指定目录下最后一个文件名
    int writefile(char* path, const void* str, int len);
    // cache mode，加入缓存机制，每1s操作一次disk
    void cachepool(void); //缓存循环
    int add_cache(char* str,
        int len = 0); // len==0,写入字符串，len>0，写入char *数组
};

#endif // XP_LOGS_H

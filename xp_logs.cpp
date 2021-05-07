#include <time.h>

#include <thread>
#ifdef _WIN32
#include <direct.h>
#include <io.h>

#include "dirent.h"
#define Delay(x) Sleep(1000 * x);
#define Delayms(x) Sleep(x);
#ifdef _WIN64
// define something for Windows (64-bit only)
#else
// define something for Windows (32-bit only)
#endif
#elif defined __linux__ || defined __APPLE__
#include <dirent.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define Delay(x) sleep(x);
#define Delayms(x) usleep(x * 1000);
#else
#error "Unknown compiler"
#endif

#include "xp_logs.h"

using namespace std;

static void mkdirs(const char* __path, __mode_t __mode)
{
    if (strlen(__path) > PATH_MAX) {
        return;
    }
    int ipathLength = strlen(__path);
    int ileaveLength = 0;
    int iCreatedLength = 0;
    char szPathTemp[PATH_MAX] = { 0 };
#if defined __linux__ || defined __APPLE__
    for (int i = 0; (NULL != strchr(__path + iCreatedLength, '/')); i++)
#elif defined _WIN32
    for (int i = 0; (NULL != strchr(__path + iCreatedLength, '\\')); i++)
#endif
    {
#if defined __linux__ || defined __APPLE__
        ileaveLength = strlen(strchr(__path + iCreatedLength, '/')) - 1;
#elif defined _WIN32
        ileaveLength = strlen(strchr(__path + iCreatedLength, '\\')) - 1;
#endif
        iCreatedLength = ipathLength - ileaveLength;
#if defined __linux__ || defined __APPLE__
        if (ileaveLength + 1 == ipathLength)
            continue;
#elif defined _WIN32
        if (ileaveLength + 2 == ipathLength)
            continue;
#endif
        strncpy(szPathTemp, __path, iCreatedLength);
#if defined __linux__ || defined __APPLE__
        mkdir(szPathTemp, __mode);
#elif defined _WIN32
        mkdir(szPathTemp);
#endif
    }
    if (iCreatedLength < ipathLength) {
#if defined __linux__ || defined __APPLE__
        mkdir(__path, __mode);
#elif defined _WIN32
        mkdir(__path);
#endif
    }
}

xp_logs::xp_logs(const char* path, const char* suffix, int daily, int cachesize)
    : sub_thread()
{
    sprintf(logpath, "%s", path);
    sprintf(Suffix, "%s", suffix);
    Daily = daily;
    if (cachesize > 0) {
        Cachesize = cachesize;
        cachebuff = new Ringbuf(cachesize);
        run();
    }
}

xp_logs::~xp_logs()
{
    if (sub_thread.joinable())
        sub_thread.join();
    Cachesize = 0;
}

void xp_logs::run() { sub_thread = std::thread(&xp_logs::cachepool, this); };
void xp_logs::cachepool(void)
{
    int n = 0;
    while (Cachesize) {
        n = cachebuff->canread();
        if (n > 0) //有数据
        {
            uint8_t* readbuff = new uint8_t[n];
            cachebuff->read(readbuff, n);
            this->add((char*)readbuff, n);
            delete[] readbuff;
        }
        Delay(1);
    }
    cachebuff->~Ringbuf();
}

int xp_logs::add_cache(char* str, int len)
{
    if (len > 0)
        return cachebuff->add((uint8_t*)str, len);
    else
        return cachebuff->add((uint8_t*)str, strlen(str));
}

int xp_logs::list_all_files(char* basePath)
{
    DIR* dir;
    struct dirent* ptr;
    char base[1000];
    if ((dir = opendir(basePath)) == NULL) {
        perror("No such dir ");
        exit(1);
    }

    while ((ptr = readdir(dir)) != NULL) {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0) /// current dir OR parrent dir
            continue;
        else if (ptr->d_type == 8) /// file
        {
            // sprintf(tmp,"%s/%s %d\n",basePath,ptr->d_name);
            printf("d_name:%s/%s %d\n", basePath, ptr->d_name, get_size(ptr->d_name));
        } else if (ptr->d_type == 10) /// link file
            printf("d_name:%s/%s\n", basePath, ptr->d_name);
        else if (ptr->d_type == 4) /// dir
        {
            memset(base, '\0', sizeof(base));
            strcpy(base, basePath);
            strcat(base, "/");
            strcat(base, ptr->d_name);
            list_all_files(base);
        }
    }
    closedir(dir);
    return 1;
}

int xp_logs::list_files(char* path)
{
    struct dirent** name_list;
    char tmp[1024];
    int n = scandir(path, &name_list, 0, alphasort);
    if (n < 0) {
        printf("no such dir %s\n", path);
    } else {
        int index = 0;
        while (index < n) {
            sprintf(tmp, "%s/%s", path, name_list[index]->d_name);
            // printf("name: %s %d\n", name_list[index]->d_name,get_size(tmp));
            free(name_list[index++]);
        }
        free(name_list);
    }
    return n;
}

char* xp_logs::get_first_file(char* path)
{
    struct dirent** name_list;
    int n = scandir(path, &name_list, 0, alphasort);
    if (n < 0)
        printf("no such dir %s\n", path);
    else {
        int index = 0;
        char* name = NULL;
        if (n > 2)
            name = name_list[2]->d_name;
        while (index < n)
            free(name_list[index++]);
        free(name_list);
        return name;
    }
    free(name_list);
    return NULL;
}

char* xp_logs::get_last_file(char* path)
{
    struct dirent** name_list;
    int n = scandir(path, &name_list, 0, alphasort);
    if (n < 0)
        printf("no such dir %s\n", path);
    else {
        int index = 0;
        char* name = NULL;
        if (n > 2)
            name = name_list[n - 1]->d_name;
        while (index < n)
            free(name_list[index++]);
        free(name_list);
        return name;
    }
    free(name_list);
    return NULL;
}

int xp_logs::creat_log(char* path)
{
    FILE* fp = NULL;
    static time_t timestamp = time(NULL);
    char tmp[4096];
    struct tm* t;
    time_t now;
    time(&now);
    t = localtime(&now);
    if (path == NULL)
        sprintf(tmp, "%s/%4d-%02d-%02d %02d-%02d-%02d%s", logpath,
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min,
            t->tm_sec, Suffix);
    else
        sprintf(tmp, "%s/%s%s", logpath, path, Suffix);
    if (access(logpath, 0) < 0) {
        printf("create dir:%s\n", logpath);
#if defined __linux__ || defined __APPLE__
        mkdirs(logpath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#elif defined _WIN32
        mkdirs(logpath);
#endif
    }
    fp = fopen(tmp, "a+");
    if (fp == NULL) {
        if (time(NULL) > timestamp + 1) {
            timestamp = time(NULL);
            printf("create wrong: %s\n", tmp);
        }
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}
int xp_logs::get_size(char* filename)
{
    struct stat statbuf;
    int size;
    if (stat(filename, &statbuf) < 0) {
        printf("get size wrong\n");
        return 0;
    }
    size = (statbuf.st_size / 1024);
    size = size == 0 ? 1 : size;
    return size;
}

int xp_logs::get_dir_size(char* path)
{
    int size = 0, index = 0;
    struct dirent** name_list;
    char dirpath[1024];
    if (path == NULL)
        sprintf(dirpath, "%s", logpath);
    else
        sprintf(dirpath, "%s/%s", logpath, path);
    if (access(dirpath, 0) < 0)
        return -1;
    int n = scandir(dirpath, &name_list, 0, alphasort);
    while (index < n) {
        char tmp[1024];
        sprintf(tmp, "%s/%s", dirpath, name_list[index]->d_name);
        size += get_size(tmp);
        free(name_list[index++]);
    }
    free(name_list);
    return size;
}

int xp_logs::writefile(char* path, const void* str, int len)
{
    static time_t timestamp = time(NULL);
    char filepath[1024];
    FILE* fp = NULL;
    sprintf(filepath, "%s/%s", logpath, path);
    fp = fopen(filepath, "ab");
    if (fp == NULL) {
        if (time(NULL) > timestamp + 1) {
            timestamp = time(NULL);
            printf("open file wrong %s\n", filepath);
        }
        fclose(fp);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    fwrite(str, sizeof(char), len, fp);
    fclose(fp);
    return 0;
}
int xp_logs::add(char* str, int len)
{
    int size;
    char* name;
    char tmp[1024];
    int date[6];
    if (list_files(logpath) > 2) {
        size = get_dir_size(logpath);
        if (size >= maxtotalsize) //超过限制大小，删除最久远日志，添加新日志
        {
            name = get_first_file(logpath);
            if (name) {
                sprintf(tmp, "%s/%s", logpath, name);
                printf("remove log %s\n", tmp);
                remove(tmp);
                add(str);
            } else {
                printf("get first file wrong\n");
                return -1;
            }
        } else {
            name = get_last_file(logpath);
            if (Daily == 1) {
                if (sscanf(name, "%4d-%02d-%02d %02d-%02d-%02d.log", &date[0], &date[1],
                        &date[2], &date[3], &date[4], &date[5])
                    > 5) {
                    //判断是否同一天 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday
                    struct tm* t;
                    time_t now;
                    time(&now);
                    t = localtime(&now);
                    if ((t->tm_year + 1900) == date[0] && (t->tm_mon + 1) == date[1] && t->tm_mday == date[2]) //同一天
                        ;
                    else {
                        if (creat_log() >= 0)
                            name = get_last_file(logpath);
                    }
                }
            }
            if (name) {
                sprintf(tmp, "%s/%s", logpath, name);
                size = get_size(tmp);
                if (size >= maxlogsize) {
                    if (creat_log() >= 0)
                        add(str);
                } else {
                    if (len > 0)
                        writefile(name, str, len);
                    else
                        writefile(name, str, strlen(str));
                }
            } else {
                printf("get last file wrong\n");
                return -1;
            }
        }
    } else {
        if (creat_log() >= 0)
            add(str);
    }
    return 0;
}

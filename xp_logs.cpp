#include "xp_logs.h"
#include <thread>

using namespace std;

xp_logs::xp_logs(const char *path,int daily,int cachesize)
{
    sprintf(logpath,"%s",path);
    Daily=daily;
    if(cachesize>0)
    {
        Cachesize=cachesize;
        cachebuff=new Ringbuf(cachesize);
        std::thread t(&xp_logs::cachepool,this);
        t.detach();
    }
}

xp_logs::~xp_logs()
{
    Cachesize=0;
}

void xp_logs::cachepool(void)
{
    int n=0,num=0;
    while(Cachesize)
    {
        n=cachebuff->canread();
        if(n>0)//有数据
        {
            uint8_t *readbuff=new uint8_t[n];
            num=cachebuff->read(readbuff,n);
            this->add((char *)readbuff,n);
            delete [] readbuff;
        }
        Delay(1);
    }
    cachebuff->~Ringbuf();
}

int xp_logs::add_cache(char *str, int len)
{
    if(len>0)
        return cachebuff->add((uint8_t *)str,len);
    else
        return cachebuff->add((uint8_t *)str,strlen(str));
}

int xp_logs::list_all_files(char *basePath)
{
    DIR *dir;
    struct dirent *ptr;
    char base[1000];
    if ((dir=opendir(basePath)) == NULL)
    {
        perror("No such dir ");
        exit(1);
    }

    while ((ptr=readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
            continue;
        else if(ptr->d_type == 8)    ///file
        {
            // sprintf(tmp,"%s/%s %d\n",basePath,ptr->d_name);
            printf("d_name:%s/%s %d\n",basePath,ptr->d_name,get_size(ptr->d_name));
        }
        else if(ptr->d_type == 10)    ///link file
            printf("d_name:%s/%s\n",basePath,ptr->d_name);
        else if(ptr->d_type == 4)    ///dir
        {
            memset(base,'\0',sizeof(base));
            strcpy(base,basePath);
            strcat(base,"/");
            strcat(base,ptr->d_name);
            list_all_files(base);
        }
    }
    closedir(dir);
    return 1;
}

int xp_logs::list_files(char *path)
{
    struct dirent **name_list;
    char tmp[1024];
    int n = scandir(path,&name_list,0,alphasort);
    if(n < 0)
    {
        printf( "no such dir %s\n",path);
    }
    else
    {
        int index=0;
        while(index < n)
        {
            sprintf(tmp,"%s/%s",path,name_list[index]->d_name);
            // printf("name: %s %d\n", name_list[index]->d_name,get_size(tmp));
            free(name_list[index++]);
        }
        free(name_list);
    }
    return n;
}



char* xp_logs::get_first_file(char *path)
{
    struct dirent **name_list;
    int n = scandir(path,&name_list,0,alphasort);
    if(n < 0)
        printf( "no such dir %s\n",path);
    else
    {
        int index=0;
        char *name;
        if(n>2)
            name=name_list[2]->d_name;
        while(index < n)
            free(name_list[index++]);
        free(name_list);
        return name;
    }
    return NULL;
}

char* xp_logs::get_last_file(char *path)
{
    struct dirent **name_list;
    int n = scandir(path,&name_list,0,alphasort);
    if(n < 0)
        printf( "no such dir %s\n",path);
    else
    {
        int index=0;
        char *name;
        if(n>2)
            name=name_list[n-1]->d_name;
        while(index < n)
            free(name_list[index++]);
        free(name_list);
        return name;
    }
    return NULL;
}


int xp_logs::creat_log()
{
    FILE* fp= NULL;
    char tmp[4096];
    struct tm *t;
    time_t now;
    time(&now);
    t = localtime(&now);
    sprintf(tmp,"%s/%4d-%02d-%02d %02d-%02d-%02d.log", logpath,t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    if(access(logpath,0)<0)
    {
        printf("creat dir:%s ",logpath );
    #ifdef linux
        mkdir(logpath,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    #elif defined WIN32
        mkdir(logpath);
    #endif
    }
    fp=fopen(tmp,"a+");
    if(fp==NULL)
    {
        printf("  creat wrong\n");
        return -1;
    }
    fclose(fp);
    return 0;
}
int xp_logs::get_size(char *filename)
{
    struct stat statbuf;
    int size;
    if(stat(filename,&statbuf)<0)
    {
        printf("get size wrong\n");
        return -1;
    }
    size=(statbuf.st_size/1024);
    size=size==0?1:size;
    return size;
}

int xp_logs::get_dir_size(char *path)
{
    int size=0,index=0;
    struct dirent **name_list;
    char tmp[1024];
    if(access(logpath,0)<0)
        return -1;
    int n = scandir(path,&name_list,0,alphasort);
    while(index < n)
    {
        sprintf(tmp,"%s/%s",path,name_list[index]->d_name);
        size+=get_size(tmp);
        free(name_list[index++]);
    }
    free(name_list);
    return size;
}

int xp_logs::writefile(char* path ,const void *str,int len)
{
    char filepath[1024];
    FILE* fp= NULL;
    sprintf(filepath,"%s/%s",logpath,path);
    fp=fopen(filepath,"ab");
    if(fp==NULL)
    {
        printf("open file wrong %s\n",filepath);
        return -1;
    }
     fseek(fp,0,SEEK_END);
    fwrite(str,sizeof(char),len,fp);
    fclose(fp);
    return 0;
}
int xp_logs::add(char *str,int len)
{
    int size;
    char *name;
    char tmp[1024];
    int date[6];
    if(list_files(logpath)>2)
    {
        size=get_dir_size(logpath);
        if(size>=maxtotalsize)//超过限制大小，删除最久远日志，添加新日志
        {
            name=get_first_file(logpath);
            if(name)
            {

                sprintf(tmp,"%s/%s",logpath,name);
                printf("remove log %s\n",tmp);
                remove(tmp);
                add(str);
            }
            else
            {
                printf("get first file wrong\n");
                return -1;
            }
        }
        else
        {
            name=get_last_file(logpath);
            if(Daily==1)
            {
                if(sscanf(name,"%4d-%02d-%02d %02d-%02d-%02d.log",&date[0],&date[1],&date[2],&date[3],&date[4],&date[5])>5)
                {
                    //判断是否同一天 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday
                    struct tm *t;
                    time_t now;
                    time(&now);
                    t = localtime(&now);
                    if((t->tm_year + 1900)==date[0] && (t->tm_mon + 1)==date[1] && t->tm_mday==date[2])//同一天
                        ;
                    else
                    {
                        if(creat_log()>=0)
                            name=get_last_file(logpath);
                    }
                }
            }
            if(name)
            {
                sprintf(tmp,"%s/%s",logpath,name);
                size=get_size(tmp);
                if(size>=maxlogsize)
                {
                    if(creat_log()>=0)
                        add(str);
                }
                else
                {
                    if(len>0)
                        writefile(name,str,len);
                    else
                        writefile(name,str,strlen(str));
                }
            }
            else
            {
                printf("get last file wrong\n");
                return -1;
            }
        }
    }
    else
    {
        if(creat_log()>=0)
            add(str);
    }
    return 0;
}
// int main(int argc, char const *argv[])
// {
// 	int n=1000;
// 	xp_logs log((char *)"./logs");
// 	while(1000)
// 	{
// 		log.add("hello world\r\n");
// 		usleep(1000);
// 	}

//   //   printf("readlist\n");
//  	// log.list_all_files(basePath);
//  	// printf("print dir\n");
//  // 	log.list_files((char *)"./logs");
// 	// printf("log size %d,name %s\n",log.get_dir_size("./logs"),log.get_first_file("./logs"));

// 	return 0;
// }

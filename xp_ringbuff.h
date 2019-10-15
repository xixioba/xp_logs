#ifndef __RINGBUFF_H
#define __RINGBUFF_H

#include <stdint.h>
#include <string.h>

//面向过程编程
void ringbuff_init(int len);
int ringbuff_canwrite(void);
int ringbuff_canread(void);
int ringbuff_w(uint8_t *buff,int n);
int ringbuff_r(uint8_t *buff,int len);
int ringbuff_r_ndel(uint8_t *buff,int len);
int ringbuff_drop(int len,int enable=1);
float ringbuff_pkl(void);

//面向对象编程
class Ringbuf
{
    struct Ringbuff
    {
        uint8_t *buff;
        int read;
        int write;
        int len;
        uint64_t pk_total;
        uint64_t  pk_loss;
        float pkl;
    }Cache;
    public:
        Ringbuf(int len);//初始化内存，单位字节
        ~Ringbuf(void); 
        int canwrite(void);//获取可写长度
        int canread(void);//获取可读长度
        int add(uint8_t *buff,int n);//写入指定长度，返回N,若可写长度不够将写入失败，返回0
        int read(uint8_t *buff,int len);//读取指定长度，同上
        int browse(uint8_t *buff,int len);//浏览指定长度，不会影响可读写范围下
	    int drop(int len,int enable=1);//丢弃，默认计数
	    float pkl(void);//丢包率
};
#endif // __ringbuff_h

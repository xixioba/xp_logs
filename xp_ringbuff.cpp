#include "xp_ringbuff.h"

Ringbuf::Ringbuf(int len) {
  Cache.buff = new uint8_t[len];
  memset(Cache.buff, 0, len);
  Cache.read = 0;
  Cache.write = 0;
  Cache.len = len;
}

Ringbuf::~Ringbuf(void) { delete[] Cache.buff; }

int Ringbuf::canwrite(void) {
  int lave = 0;
  if (Cache.write > Cache.read) {
    lave = Cache.len - Cache.write + Cache.read - 1;
  } else if (Cache.write < Cache.read) {
    lave = Cache.read - Cache.write - 1;
  } else {
    lave = Cache.len - 1;
  }
  return lave;
}

int Ringbuf::canread(void) {
  int lave = 0;
  if (Cache.write > Cache.read) {
    lave = Cache.write - Cache.read;
  } else if (Cache.write < Cache.read) {
    lave = Cache.len - Cache.read + Cache.write;
  }
  return lave;
}

int Ringbuf::add(uint8_t *buff, int n) {
  int lave = 0;
  if (canwrite() >= n) {
    if (Cache.write >= Cache.read) {
      lave = Cache.len - Cache.write;
      if (lave >= n) {
        memcpy(Cache.buff + Cache.write, buff, n);
        Cache.write += n;
      } else {
        memcpy(Cache.buff + Cache.write, buff, lave);
        memcpy(Cache.buff, buff + lave, n - lave);
        Cache.write = n - lave;
      }
    } else {
      lave = Cache.read - Cache.write;
      memcpy(Cache.buff + Cache.write, buff, n);
      Cache.write += n;
    }
    Cache.pk_total += n;
    return n;
  }
  return 0;
}

int Ringbuf::read(uint8_t *buff, int len) {
  int lave;
  if (canread() >= len) {
    if (Cache.write >= Cache.read) {
      memcpy(buff, Cache.buff + Cache.read, len);
      //           memset(Cache.buff+Cache.read,0,len);
      Cache.read += len;
    } else {
      lave = Cache.len - Cache.read;
      if (lave >= len) {
        memcpy(buff, Cache.buff + Cache.read, len);
        //                memset(Cache.buff+Cache.read,0,len);
        Cache.read += len;
      } else {
        memcpy(buff, Cache.buff + Cache.read, lave);
        //               memset(Cache.buff+Cache.read,0,lave);
        memcpy(buff + lave, Cache.buff, len - lave);
        //               memset(Cache.buff,0,len-lave);
        Cache.read = len - lave;
      }
    }
    return len;
  }
  return 0;
}

int Ringbuf::browse(uint8_t *buff, int len) {
  int lave;
  if (canread() >= len) {
    if (Cache.write >= Cache.read) {
      memcpy(buff, Cache.buff + Cache.read, len);
    } else {
      lave = Cache.len - Cache.read;
      if (lave >= len) {
        memcpy(buff, Cache.buff + Cache.read, len);
      } else {
        memcpy(buff, Cache.buff + Cache.read, lave);
        memcpy(buff + lave, Cache.buff, len - lave);
      }
    }
    return len;
  }
  return 0;
}

int Ringbuf::drop(int len, int enable)  //丢弃，filter=0，使能PKL
{
  int lave;
  if (canread() >= len) {
    if (Cache.write >= Cache.read) {
      Cache.read += len;
    } else {
      lave = Cache.len - Cache.read;
      if (lave >= len) {
        Cache.read += len;
      } else {
        Cache.read = len - lave;
      }
    }
    if (enable == 1) Cache.pk_loss += len;
    return len;
  }
  return 0;
}

float Ringbuf::pkl(void) {
  Cache.pkl =
      Cache.pk_total == 0 ? 0 : (float)Cache.pk_loss / Cache.pk_total * 100.0;
  Cache.pk_total = 0;
  Cache.pk_loss = 0;
  return Cache.pkl;
}

struct Ringbuff {
  uint8_t *buff;
  int read;
  int write;
  int len;
  int pk_total;
  int pk_loss;
  float pkl;
} Cache;

void ringbuff_init(int len) {
  Cache.buff = new uint8_t[len];
  memset(Cache.buff, 0, len);
  Cache.read = 0;
  Cache.write = 0;
  Cache.len = len;
  Cache.pk_total = 0;
  Cache.pk_loss = 0;
}

int ringbuff_canwrite(void) {
  int lave = 0;
  if (Cache.write > Cache.read) {
    lave = Cache.len - Cache.write + Cache.read - 1;
  } else if (Cache.write < Cache.read) {
    lave = Cache.read - Cache.write - 1;
  } else {
    lave = Cache.len - 1;
  }
  return lave;
}
int ringbuff_canread(void) {
  int lave = 0;
  if (Cache.write > Cache.read) {
    lave = Cache.write - Cache.read;
  } else if (Cache.write < Cache.read) {
    lave = Cache.len - Cache.read + Cache.write;
  }
  return lave;
}
int ringbuff_w(uint8_t *buff, int n) {
  int lave = 0;
  if (ringbuff_canwrite() >= n) {
    if (Cache.write >= Cache.read) {
      lave = Cache.len - Cache.write;
      if (lave >= n) {
        memcpy(Cache.buff + Cache.write, buff, n);
        Cache.write += n;
      } else {
        memcpy(Cache.buff + Cache.write, buff, lave);
        memcpy(Cache.buff, buff + lave, n - lave);
        Cache.write = n - lave;
      }
    } else {
      lave = Cache.read - Cache.write;
      memcpy(Cache.buff + Cache.write, buff, n);
      Cache.write += n;
    }
    Cache.pk_total += n;
    return n;
  }
  return 0;
}

int ringbuff_r(uint8_t *buff, int len) {
  int lave;
  if (ringbuff_canread() >= len) {
    if (Cache.write >= Cache.read) {
      memcpy(buff, Cache.buff + Cache.read, len);
      //           memset(Cache.buff+Cache.read,0,len);
      Cache.read += len;
    } else {
      lave = Cache.len - Cache.read;
      if (lave >= len) {
        memcpy(buff, Cache.buff + Cache.read, len);
        //                memset(Cache.buff+Cache.read,0,len);
        Cache.read += len;
      } else {
        memcpy(buff, Cache.buff + Cache.read, lave);
        //               memset(Cache.buff+Cache.read,0,lave);
        memcpy(buff + lave, Cache.buff, len - lave);
        //               memset(Cache.buff,0,len-lave);
        Cache.read = len - lave;
      }
    }
    return len;
  }
  return 0;
}

int ringbuff_r_ndel(uint8_t *buff, int len) {
  int lave;
  if (ringbuff_canread() >= len) {
    if (Cache.write >= Cache.read) {
      memcpy(buff, Cache.buff + Cache.read, len);
    } else {
      lave = Cache.len - Cache.read;
      if (lave >= len) {
        memcpy(buff, Cache.buff + Cache.read, len);
      } else {
        memcpy(buff, Cache.buff + Cache.read, lave);
        memcpy(buff + lave, Cache.buff, len - lave);
      }
    }
    return len;
  }
  return 0;
}

int ringbuff_drop(int len, int enable)  //丢弃，filter=0，使能PKL
{
  int lave;
  if (ringbuff_canread() >= len) {
    if (Cache.write >= Cache.read) {
      Cache.read += len;
    } else {
      lave = Cache.len - Cache.read;
      if (lave >= len) {
        Cache.read += len;
      } else {
        Cache.read = len - lave;
      }
    }
    if (enable == 1) Cache.pk_loss += len;
    return len;
  }
  return 0;
}

float ringbuff_pkl(void) {
  Cache.pkl =
      Cache.pk_total == 0 ? 0 : (float)Cache.pk_loss / Cache.pk_total * 100.0;
  Cache.pk_total = 0;
  Cache.pk_loss = 0;
  return Cache.pkl;
}

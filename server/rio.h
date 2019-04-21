
#ifndef __RIO_H__
#define __RIO_H__

#include "head.h"

ssize_t rio_readn(int fd, void *usrbuf, size_t n);

ssize_t rio_writen(int fd, void *usrbuf, size_t n);

#endif
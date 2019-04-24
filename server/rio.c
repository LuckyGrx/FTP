#include "rio.h"

ssize_t rio_readn(int fd, void *usrbuf, size_t n) {
    size_t nleft = n; //剩下未读字符数
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0) {
        if ((nread = read(fd, bufp, nleft)) < 0) {
            if (errno == EINTR)  //被信号处理函数中断
                nread = 0;      //本次读到0个字符，再次读取
            else
                return -1;      //出错，errno由read设置
        } 
        else if (nread == 0) //读取到EOF
            break;              
        nleft -= nread; //剩下的字符数减去本次读到的字符数
        bufp += nread;  //缓冲区指针向右移动
    }
    //返回实际读取的字符数
    return (n - nleft);         /* return >= 0 */
}


ssize_t rio_writen(int fd, void *usrbuf, size_t n) 
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno == EINTR)  
                nwritten = 0;    
            else
                return -1;       
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}
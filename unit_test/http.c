#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096 //读缓冲区大小

// 主状态机的两种状态
enum CHECK_STATE {
    CHECK_STATE_REQUESTLINE = 0, // 当前正在分析请求行
    CHECK_STATE_HEADER           // 当前正在分析请求头
};

// 从状态机的三种可能状态,即行的读取状态
enum LINE_STATUS {
    LINE_OK = 0, // 读取到一个完整的行
    LINE_BAD,    // 读取行出错
    LINE_OPEN    // 读取行数据不完整
};

// 服务器处理HTTP请求的结果
enum HTTP_CODE {
    NO_REQUEST,         // 请求不完整,需要继续读取客户数据 
    GET_REQUEST,        // 获得了一个完整的请求
    BAD_REQUEST,        // 客户请求有语法错误
    FORBIDDEN_REQUEST,  // 客户对资源没有足够的访问权限
    INTERNAL_ERROR,     // 服务器内部错误
    CLOSED_CONNECTION   // 客户端已经关闭连接了
};

static const char* szret[] = {"I get a correct result\n", "Something wrong\n"};

// 从状态机,用于解析出一行内容
// read_index          // 当前已经读取了多少字节的客户数据
// checked_index       // 当前已经分析完了多少字节的客户数据
enum LINE_STATUS parse_line(char* buffer, int* checked_index, int* read_index) {
    char temp;

    // checked_index指向buffer(应用程序的读缓冲区)中正在分析的字节
    // read_index 指向buffer中客户数据的尾部的下一个字节
    // buffer中第0 - checked_index字节都已分析完毕
    // 第checked_index - (read_index - 1)字节由下面的循环挨个分析
    for (; *checked_index < *read_index; (*checked_index)++) {
        // 获取当前要分析的字节
        temp = buffer[*checked_index];
        if (temp == '\r') {
            if ((*checked_index + 1) == *read_index) {
                return LINE_OPEN;
            } else if (buffer[*checked_index + 1] == '\n') {
                // 将buffer中的\r\n字符设置为空字符
                buffer[*checked_index] = '\0';
                *checked_index = (*checked_index) + 1;
                buffer[*checked_index] = '\0';
                *checked_index = (*checked_index) + 1;
                return LINE_OK;
            }
            return LINE_BAD;
        } 
        /*
        else if (temp == '\n') {
            // 如果当前的字节是'\n',即换行符,则说明可能读取到一个完整的行
            if (*checked_index > 1) && buffer[*checked_index - 1] == '\r') {
                buffer[*checked_index - 1] = '\0';
                buffer[*checked_index++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        */
    }
    // 如果所有内容分析完毕也没有遇到'\r'字符,则返回LINE_OPEN,表示还需要继续读取客户数据才能进一步分析
    return LINE_OPEN;
}


// 分析请求行
enum HTTP_CODE parse_requestline(char* temp, enum CHECK_STATE* checkstate) {
    // extern char* strpbrk(char *str1, char *str2);
    // 比较字符串str1和str2中是否有相同的字符,如果有,则返回该字符在str1中的位置的指针
    // 返回: 返回指针,搜索到的字符在str1中的索引位置的指针
    char* url = strpbrk(temp, " \t"); // 
    // 如果请求行中没有空格或"\t"字符,则HTTP请求必有问题
    if (!url) {
        return BAD_REQUEST;
    }

    *url++ = '\0'; // 这里赋值空字符,利于下面strcasecmp方法比较请求方法method

    char* method = temp;
    if (strcasecmp(method, "GET") == 0) {   // 暂时支持GET方法
        printf("The request method is GET\n");
    } else {
        return BAD_REQUEST;
    }

    // size_t strspn(const char *str1, const char *str2)
    // 返回str1中第一个不在字符串str2中出现的字符下标
    url += strspn(url, " \t");  // 跳过请求方法和url之间的所有空格和制表符,使url指向真正的url起始位置
    char* version = strpbrk(url, " \t");

    if (!version) {
        return BAD_REQUEST;
    }
    printf("aaa\n");

    *version++ = '\0';
    version += strspn(version, " \t");

    //暂时支持 HTTP1.1
    if (strcasecmp(version, "HTTP/1.1") != 0) {
        return BAD_REQUEST;
    }
    // 检查URL是否合法
    if (strncasecmp(url, "http://", 7) == 0) {
        url += 7;// 跳过url前面的http://
        // extern char* strchr(const char *str, int c)
        // 返回字符串str中第一次出现字符c的位置,若没找到该字符则返回NULL
        url = strchr(url, '/');
    }
    if (!url || url[0] != '/') { // url必须包含'/'字符,一般处于末尾
        return BAD_REQUEST;
    }

    printf("The request URL is:%s\n", url);
    // HTTP请求行处理完毕,状态转移到请求头的分析

    *checkstate = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

// 分析请求头
enum HTTP_CODE parse_headers(char* temp) {
    // 遇到一个空行,说明我们得到了一个正确的HTTP请求
    if (temp[0] == '\0') {
        return GET_REQUEST;
    } else if (strncasecmp(temp, "Host:", 5) == 0) { // 处理"Host"头部字段
        temp += 5;
        temp += strspn(temp, " \t");
        printf("the request host is: %s\n", temp);
    } else {
        printf("I can not handle thie header\n");
    }
    return NO_REQUEST;
}


// 分析HTTP请求的入口函数
enum HTTP_CODE parse_content(char* buffer, int* checked_index, enum CHECK_STATE* checkstate, int* read_index, int* start_line) {
    enum LINE_STATUS linestatus = LINE_OK;    // 记录当前行的读取状态
    enum HTTP_CODE retcode = NO_REQUEST;      // 记录HTTP请求的处理结果

    // 主状态机,用于从buffer中取出所有完整的行
    while ((linestatus = parse_line(buffer, checked_index, read_index)) == LINE_OK) {
        char* temp = buffer + *start_line;  // start_line是行在buffer中的起始位置
        *start_line = *checked_index;        // 记录下一行的起始位置
        // checkstate 记录主状态机当前的状态
        switch (*checkstate) {
            case CHECK_STATE_REQUESTLINE:    // 第一个状态,分析请求行
                retcode = parse_requestline(temp, checkstate);
                if (retcode == BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            case CHECK_STATE_HEADER:         // 第二个状态,分析请求头
                retcode = parse_headers(temp);
                if (retcode == BAD_REQUEST)
                    return BAD_REQUEST;
                else if (retcode == GET_REQUEST)
                    return GET_REQUEST;
                break;
            default:
                return INTERNAL_ERROR;
        }
    }
    // 若没有读取到一个完整的行,则表示还需要继续读取客户数据才能进一步分析
    if (linestatus == LINE_OPEN)
        return NO_REQUEST;
    else
        return BAD_REQUEST;
}


int main(int argc, char** argv) {
    if (argc <= 2) {
        printf("usage: need ip_address port_number\n");
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    int ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int fd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);

    if (fd < 0)
        printf("errno is %d\n", errno);
    else {
        char buffer[BUFFER_SIZE];    //读缓冲区
        memset(buffer, '\0', BUFFER_SIZE);

        int data_read = 0;
        int read_index = 0;          // 当前已经读取了多少字节的客户数据
        int checked_index = 0;       // 当前已经分析完了多少字节的客户数据
        int start_line = 0;          // 行在buffer中的起始位置
        // 设置主状态机的初始状态
        enum CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;
        while (1) { // 循环读取客户数据并分析之
            data_read = recv(fd, buffer + read_index, BUFFER_SIZE - read_index, 0);
            if (data_read == -1) {
                printf("reading failed\n");
                break;
            } else if (data_read == 0) {
                printf("remote client has closed the connection\n");
                break;
            }
            read_index += data_read;
            // 分析目前已经获得的所有客户数据
            enum HTTP_CODE result = parse_content(buffer, &checked_index, &checkstate, &read_index, &start_line);

            if (result == NO_REQUEST)                         // 尚未得到一个完整的HTTP请求
                continue;
            else if (result == GET_REQUEST) {                 // 得到一个完整的,正确的HTTP请求
                send(fd, szret[0], strlen(szret[0]), 0);
                break;
            } else {                                          // 其他情况表示发生错误
                send(fd, szret[1], strlen(szret[1]), 0);
                break;
            }
        }
        close(fd);
    }
    close(listenfd);
    return 0;
}
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cassert> 
#include <netinet/ip.h>
#include <vector>
#include <fcntl.h>
#include <poll.h>

using namespace std;

enum{
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,
}

struct Conn{
    int fd = -1;
    uint32_t state = 0;
    size_t rbuf_size = 0;
    uint8_t rbuf[4+k_max_msg];
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4+k_max_msg];
};


static void fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) {
        die("fcntl error");
        return;
    }

    flags |= O_NONBLOCK;

    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno) {
        die("fcntl error");
    }
}

static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}
const size_t k_max_msg = 4096;

static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error, or unexpected EOF
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;  // error
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t one_request(int connfd) {
    // 4 bytes header
    char rbuf[4 + k_max_msg + 1];
    errno = 0;
    int32_t err = read_full(connfd, rbuf, 4);
    if (err) {
        if (errno == 0) {
            msg("EOF");
        } else {
            msg("read() error");
        }
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);  // assume little endian
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // request body
    err = read_full(connfd, &rbuf[4], len);
    if (err) {
        msg("read() error");
        return err;
    }

    // do something
    rbuf[4 + len] = '\0';
    printf("client says: %s\n", &rbuf[4]);

    // reply using the same protocol
    const char reply[] = "world";
    char wbuf[4 + sizeof(reply)];
    len = (uint32_t)strlen(reply); 
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply, len);
    return write_all(connfd, wbuf, 4 + len);
}
static void do_something(int fd){
    char rbuf[64] = {};
    ssize_t n = read(fd,rbuf,sizeof(rbuf)-1);
    if(n < 0){
        msg("read() error");
        return;
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(fd, wbuf, strlen(wbuf));
}
int main() {
    int fd = socket(AF_INET,SOCK_STREAM,0);
    if (fd < 0 )
    {
        die("socket()");
    }
    vector<Conn *> fd2conn;
    vector<struct pollfd> poll_args;

    fd_set_nb(fd);

     // this is needed for most server applications
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr)); // 初始化地址结构体
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1235);
    addr.sin_addr.s_addr = ntohl(0);
    //  // 使用bind函数的返回值，不需要显式转换
    // if (::bind(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
    //     die("bind error");
    // }
    int rv = int(bind(fd, (const sockaddr *)&addr, sizeof(addr)));
    if (rv){
        die("bind()");
    }
    rv = listen(fd,SOMAXCONN);
    if (rv)
    {
        /* code */
        die("listen()");
    }
    // while (true)
    // {
    //     struct sockaddr_in client_addr = {};
    //     socklen_t addrlen = sizeof(client_addr);
    //     int connfd = accept(fd,(struct sockaddr *)&client_addr,&addrlen);
    //     if(connfd < 0){
    //         continue;
    //     }
    //     while (true)
    //     {
    //         int32_t err = one_request(connfd);
    //         if (err)
    //         {
    //             break;
    //         }
    //     }
    //     // do_something(connfd);
    //     close(connfd);
    // }
    while(true){
        poll_args.clear();

        struct pollfd pfd ={fd,POLLIN,0};
        poll_args.push_back(pfd);

        for(Conn *conn:fd2conn){
            if(!conn){
                continue;
            }
            struct pollfd pfd = {};
            pdf.fd = conn->fd;
            pdf.events = (conn->state == State_REQ?POLLIN:POLLOUT);
            pdf.events = pdf.events |POLLERR;
            poll_args.push_back(pfd);
        }
        int rv = poll(poll_arges.data(),(nfds_t)poll_args.size(),1000);
        if(rv < 0){
            die("poll()");
        }

        for (size_t i = 0; i < poll_args.size(); i++)
        {
            if(poll_args[i].revents){
                Conn *conn = fd2conn[poll_args[i].fd];
                connection_io(conn);
                if(conn->state == STATE_END){
                    fd2conn[conn->fd] = NULL;
                    (void)close(conn->fd);
                    free(conn);
                }
            }
        }
        if(poll_args[0].revents){
            (void)accept_new_conn(fd2conn,fd);
        }
        
    }
    return 0;
}

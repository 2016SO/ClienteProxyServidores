#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/ftp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#define BUF_SIZE 4096

extern int sys_nerr, errno;

    char client_hostname[64];


    void set_nonblock(int fd)
    {
        int fl;
        int x;
        x = fcntl(fd, F_GETFL, &fl);
        if (x < 0) {
        exit(1);
        }
        fl |= O_NONBLOCK;
        x = fcntl(fd, F_SETFL, &fl);
        if (x < 0) {
        exit(1);
        }
    }


    int serwer_gniazdo(char *addr, int port)
    {
        int addrlen, s, on = 1, x;
        static struct sockaddr_in client_addr;

        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0)
        perror("socket"), exit(1);

        addrlen = sizeof(client_addr);
        memset(&client_addr, '\0', addrlen);
        client_addr.sin_family = AF_INET;
        client_addr.sin_addr.s_addr = inet_addr(addr);
        client_addr.sin_port = htons(port);
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, 4);
        x = bind(s, (struct sockaddr *) &client_addr, addrlen);
        if (x < 0)
        perror("bind"), exit(1);

        x = listen(s, 5);
        if (x < 0)
        perror("listen"), exit(1);

        return s;
    }

    int otworz_host(char *host, int port)
    {
        struct sockaddr_in rem_addr;
        int len, s, x;
        struct hostent *H;
        int on = 1;

        H = gethostbyname(host);
        if (!H)
        return (-2);

        len = sizeof(rem_addr);

        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0)
        return s;

        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, 4);

        len = sizeof(rem_addr);
        memset(&rem_addr, '\0', len);
        rem_addr.sin_family = AF_INET;
        memcpy(&rem_addr.sin_addr, H->h_addr, H->h_length);
        rem_addr.sin_port = htons(port);
        x = connect(s, (struct sockaddr *) &rem_addr, len);
        if (x < 0) {
        close(s);
        return x;
        }
        set_nonblock(s);
        return s;
    }

    int sock_addr_info(struct sockaddr_in addr, int len, char *fqdn)
    {
        struct hostent *hostinfo;

        hostinfo = gethostbyaddr((char *) &addr.sin_addr.s_addr, len, AF_INET);
        if (!hostinfo) {
        sprintf(fqdn, "%s", inet_ntoa(addr.sin_addr));
        return 0;
        }
        if (hostinfo && fqdn)
        sprintf(fqdn, "%s [%s]", hostinfo->h_name, inet_ntoa(addr.sin_addr));
        return 0;
    }


    int czekaj_na_polaczenie(int s)
    {
       int newsock;
    static struct sockaddr_in peer;
    socklen_t len;
    len = sizeof(struct sockaddr);
    newsock = accept(s, (struct sockaddr *) &peer, &len);
        if (newsock < 0) {
        if (errno != EINTR)
            perror("accept");
        }
        sock_addr_info(peer, len, client_hostname);
        set_nonblock(newsock);
        return (newsock);
    }

    int zapis(int fd, char *buf, int *len)
    {
        int x = write(fd, buf, *len);
        if (x < 0)
            return x;
        if (x == 0)
            return x;
        if (x != *len)
            memmove(buf, buf+x, (*len)-x);
        *len -= x;
        return x;
    }

    void klient(int cfd, int sfd)
    {
        int maxfd;
        char *sbuf;
        char *cbuf;
        int x, n;
        int cbo = 0;
        int sbo = 0;
        fd_set R;

        sbuf = (char *)malloc(BUF_SIZE);
        cbuf = (char *)malloc(BUF_SIZE);
        maxfd = cfd > sfd ? cfd : sfd;
        maxfd++;

       while (1)
       {
        struct timeval to;
        if (cbo)
            {
            if (zapis(sfd, cbuf, &cbo) < 0 && errno != EWOULDBLOCK) {
                    exit(1);
            }
        }
        if (sbo) {
            if (zapis(cfd, sbuf, &sbo) < 0 && errno != EWOULDBLOCK) {
                    exit(1);
            }
        }

        FD_ZERO(&R);
        if (cbo < BUF_SIZE)
            FD_SET(cfd, &R);
        if (sbo < BUF_SIZE)
            FD_SET(sfd, &R);

        to.tv_sec = 0;
        to.tv_usec = 1000;
        x = select(maxfd+1, &R, 0, 0, &to);
        if (x > 0) {
            if (FD_ISSET(cfd, &R)) {
            n = read(cfd, cbuf+cbo, BUF_SIZE-cbo);
            if (n > 0) {
                cbo += n;
            } else {
                close(cfd);
                close(sfd);
                _exit(0);
            }
            }
            if (FD_ISSET(sfd, &R)) {
            n = read(sfd, sbuf+sbo, BUF_SIZE-sbo);
            if (n > 0) {
                sbo += n;
            } else {
                close(sfd);
                close(cfd);
                _exit(0);
            }
            }
        } else if (x < 0 && errno != EINTR) {
            close(sfd);
            close(cfd);
            _exit(0);
        }
        }
    }


    int main(int argc, char *argv[])
    {
        char *localaddr = (char *)"127.0.0.1";
        int localport = atoi(argv[1]);
        char *remoteaddr = (char *)(argv[2]);
        int remoteport = atoi(argv[3]);
        int client, server;
        int master_sock;

        if (4 != argc)
        {
            fprintf(stderr, "usage: %s port host port\n", argv[0]);
            exit(1);
        }

        assert(localaddr);
        assert(localport > 0);
        assert(remoteaddr);
        assert(remoteport > 0);

        master_sock = serwer_gniazdo(localaddr, localport);

        for (;;)
        {
            if ((client = czekaj_na_polaczenie(master_sock)) < 0)
                continue;
            if ((server = otworz_host(remoteaddr, remoteport)) < 0)
                continue;
            if (!fork()) {
                klient(client, server);
            }


            close(client);
            close(server);        
        }

        printf("Koniec programu");

        return 0;
    }
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#define PORT "1234"
#define BACKLOG 10

void sigchld_handler(int s) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(void) {
    int sockfd, new_fd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
    }
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "Serveris: failed to bind\n");
        return 2;
    }
    freeaddrinfo(servinfo);
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror("sigaction");
        exit(1);
    }
    printf("Serveris: laukiama prisijungimo\n");
    //prekiu_sk is variable to be changed in child processes and saved in parent
    int prekiu_sk = 100, tekstas, kl_zinute;
    int kliento_sk;
    char buffer[80];

    int fd1[2];
    pipe(fd1);
    write(fd1[1],&prekiu_sk,sizeof(prekiu_sk));


    while (1) {

    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size);
    if (new_fd == -1) {
        perror("accept");
        continue;
    }
    inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *) &their_addr), s, sizeof s);
    printf("Serveris: prisijungiama adresu %s\n", s);

    if (!fork()) {
        int skaicius;
        read(fd1[0],&skaicius,sizeof(prekiu_sk));
        close(sockfd);
        if (skaicius<1)
        {
            tekstas = sprintf(buffer, "Sandelyje nebera prekiu.\nRysys nutraukiamas\n");
            send(new_fd, buffer, tekstas, 0);
            write(fd1[1],&skaicius, sizeof(skaicius));
            //close(fd1[0]);
            //close(fd1[1]);
            close(new_fd);
            exit(0);                

        }
        tekstas = sprintf(buffer, "Siuo metu sandalyje uzregistruota %d prekiu.\n", skaicius);
        send(new_fd, buffer, tekstas, 0);
        tekstas = sprintf(buffer, "Iveskite isimamu prekiu skaiciu.\n");
        send(new_fd, buffer, tekstas, 0);
        kl_zinute = recv(new_fd, buffer, sizeof (buffer), 0);
        kliento_sk = atoi(buffer);
        if ((kliento_sk<1) || (kliento_sk>skaicius))
        {
            tekstas = sprintf(buffer, "Neteisingai ivestas isimamu prekiu skaicius.\nRysys nutraukiamas\n");
            send(new_fd, buffer, tekstas, 0);
            write(fd1[1],&skaicius, sizeof(skaicius));
            //close(fd1[0]);
            //close(fd1[1]);
            close(new_fd);
            exit(0); 
        }
        printf("%d \n  %d \n %d", *fd1, kl_zinute, kliento_sk);
        //read(fd1[0],&skaicius,sizeof(prekiu_sk));
        if (skaicius<kliento_sk)
        {
            tekstas = sprintf(buffer, "Likutis sandelyje nepakankamas.\n "
                    "Kazkas pries jus sumazino prekiu kieki.\n"
                    "Rysys nutraukiamas\n");
            send(new_fd, buffer, tekstas, 0);
            write(fd1[1],&skaicius, sizeof(skaicius));
            //close(fd1[0]);
            //close(fd1[1]);
            close(new_fd);
            exit(0); 
        }
        skaicius = skaicius - kliento_sk;
        printf("%d",atoi(buffer));
        tekstas = sprintf(buffer, "Liko %d prekiu \n", skaicius);
        send(new_fd, buffer, tekstas, 0);
        write(fd1[1],&skaicius, sizeof(skaicius));
        printf("%d child %d",*fd1, skaicius);
        //close(fd1[0]);
        //close(fd1[1]);
        close(new_fd);
        exit(0);
    }
    //wait(NULL);
    read(fd1[0],&prekiu_sk,sizeof(prekiu_sk));
    write(fd1[1],&prekiu_sk,sizeof(prekiu_sk));
    //prekiu_sk=prekiu_sk-*fd1;
    printf("\n parent prekiu_sk: %d parent fd: %d \n", prekiu_sk, *fd1);
    close(new_fd);
    }
    return 0;
}
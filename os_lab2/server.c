#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 3030

static volatile sig_atomic_t got_sighup = 0;

static void sigh(int s) {
    got_sighup = 1;
}

int main(void) {
    int srv = -1;
    int cli = -1;
    int mx;
    fd_set rset;
    sigset_t block, orig, empty;
    printf("Сервер на порту %d\n", PORT);
    
    srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) {
        perror("socket");
        exit(1);
    }
    
    int opt = 1;
    if (setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(srv);
        exit(1);
    }
    
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(PORT);
    
    if (bind(srv, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
        perror("bind");
        close(srv);
        exit(1);
    }
    
    if (listen(srv, 1) < 0) {
        perror("listen");
        close(srv);
        exit(1);
    }
    
    printf("Ожидание подключений\n");
    
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = sigh;
    act.sa_flags = SA_RESTART;
    
    if (sigaction(SIGHUP, &act, NULL) < 0) {
        perror("sigaction");
        close(srv);
        exit(1);
    }
    
    sigemptyset(&block);
    sigaddset(&block, SIGHUP);
    
    if (sigprocmask(SIG_BLOCK, &block, &orig) < 0) {
        perror("sigprocmask");
        close(srv);
        exit(1);
    }
    
    sigemptyset(&empty);
    
    while (1) {
        FD_ZERO(&rset);
        FD_SET(srv, &rset);
        mx = srv;
        
        if (cli != -1) {
            FD_SET(cli, &rset);
            if (cli > mx) mx = cli;
        }
        
        if (got_sighup) {
            printf("Получен SIGHUP\n");
            got_sighup = 0;
        }
        
        int n = pselect(mx + 1, &rset, NULL, NULL, NULL, &orig);
        
        if (n < 0) {
            if (errno == EINTR) {
                printf("Прервано сигналом\n");
                continue;
            }
            perror("pselect");
            break;
        }
        
        if (FD_ISSET(srv, &rset)) {
            struct sockaddr_in ca;
            socklen_t len = sizeof(ca);
            
            int new = accept(srv, (struct sockaddr*)&ca, &len);
            if (new < 0) {
                perror("accept");
                continue;
            }
            
            char ip[16];
            inet_ntop(AF_INET, &ca.sin_addr, ip, sizeof(ip));
            printf("Новый клиент %s:%d (сокет %d)\n", 
                   ip, ntohs(ca.sin_port), new);
            
            if (cli != -1) {
                printf("Закрываем новое соединение (клиент уже есть)\n");
                close(new);
            } else {
                cli = new;
                printf("Клиент подключен\n");
            }
        }
        
        if (cli != -1 && FD_ISSET(cli, &rset)) {
            char buf[1024];
            ssize_t rd = read(cli, buf, sizeof(buf));
            
            if (rd > 0) {
                printf("Получено: %zd байт\n", rd);
            } else if (rd == 0) {
                printf("Клиент отключился (сокет %d)\n", cli);
                close(cli);
                cli = -1;
            } else {
                perror("read");
                close(cli);
                cli = -1;
            }
        }
    }
    
    if (cli != -1) close(cli);
    close(srv);
    
    printf("Сервер остановлен\n");
    return 0;
}
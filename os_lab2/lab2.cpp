#include <iostream>
#include <cstring>
#include <csignal>
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 3030

static volatile sig_atomic_t got_sighup = 0;

static void sigh(int) {
    got_sighup = 1;
}

class ClientConnection {
public:
    ClientConnection() : fd(-1) {}

    bool isActive() const { return fd != -1; }
    int getFd() const { return fd; }
    void setFd(int f) { fd = f; }

    void closeClient() {
        if (fd != -1) {
            std::cout << "Отключение клиента (сокет " << fd << ")\n";
            close(fd);
            fd = -1;
        }
    }

    void handleRead() {
        char buf[1024];
        ssize_t rd = read(fd, buf, sizeof(buf));

        if (rd > 0) {
            std::cout << "Получено: " << rd << " байт\n";
        } else if (rd == 0) {
            std::cout << "Клиент отключился (сокет " << fd << ")\n";
            closeClient();
        } else {
            perror("read");
            closeClient();
        }
    }

private:
    int fd;
};

class Server {
public:
    Server(int port) : port(port), srv(-1) {}

    bool init() {
        std::cout << "Сервер на порту " << port << "\n";

        srv = socket(AF_INET, SOCK_STREAM, 0);
        if (srv < 0) {
            perror("socket");
            return false;
        }

        int opt = 1;
        if (setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt");
            return false;
        }

        struct sockaddr_in sa{};
        sa.sin_family = AF_INET;
        sa.sin_addr.s_addr = htonl(INADDR_ANY);
        sa.sin_port = htons(port);

        if (bind(srv, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
            perror("bind");
            return false;
        }

        if (listen(srv, 1) < 0) {
            perror("listen");
            return false;
        }

        std::cout << "Ожидание подключений\n";

        installSignalHandler();
        prepareSigMasks();

        return true;
    }

    void run() {
        while (true) {
            fd_set rset;
            FD_ZERO(&rset);
            FD_SET(srv, &rset);
            int mx = srv;

            if (client.isActive()) {
                FD_SET(client.getFd(), &rset);
                if (client.getFd() > mx) mx = client.getFd();
            }

            if (got_sighup) {
                std::cout << "Получен SIGHUP\n";
                got_sighup = 0;
            }

            int n = pselect(mx + 1, &rset, nullptr, nullptr, nullptr, &origMask);

            if (n < 0) {
                if (errno == EINTR) {
                    std::cout << "Прервано сигналом\n";
                    continue;
                }
                perror("pselect");
                break;
            }

            if (FD_ISSET(srv, &rset)) {
                handleNewClient();
            }

            if (client.isActive() && FD_ISSET(client.getFd(), &rset)) {
                client.handleRead();
            }
        }
    }

    ~Server() {
        client.closeClient();
        if (srv != -1) close(srv);
        std::cout << "Сервер остановлен\n";
    }

private:
    int port;
    int srv;
    ClientConnection client;
    sigset_t origMask;

    void installSignalHandler() {
        struct sigaction act{};
        act.sa_handler = sigh;
        act.sa_flags = SA_RESTART;
        sigaction(SIGHUP, &act, nullptr);
    }

    void prepareSigMasks() {
        sigset_t block;
        sigemptyset(&block);
        sigaddset(&block, SIGHUP);

        if (sigprocmask(SIG_BLOCK, &block, &origMask) < 0) {
            perror("sigprocmask");
            exit(1);
        }
    }

    void handleNewClient() {
        struct sockaddr_in ca{};
        socklen_t len = sizeof(ca);

        int newfd = accept(srv, (struct sockaddr*)&ca, &len);
        if (newfd < 0) {
            perror("accept");
            return;
        }

        char ip[16];
        inet_ntop(AF_INET, &ca.sin_addr, ip, sizeof(ip));
        std::cout << "Новый клиент " << ip << ":" << ntohs(ca.sin_port)
                  << " (сокет " << newfd << ")\n";

        if (client.isActive()) {
            std::cout << "Уже есть клиент — новое соединение закрыто\n";
            close(newfd);
        } else {
            client.setFd(newfd);
            std::cout << "Клиент подключен\n";
        }
    }
};

int main() {
    Server server(PORT);
    if (!server.init()) {
        std::cerr << "Ошибка инициализации сервера\n";
        return 1;
    }
    server.run();
    return 0;
}


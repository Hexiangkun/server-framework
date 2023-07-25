#include "io_manager.h"
#include "log.h"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

hxk::Logger::_ptr g_logger = GET_ROOT_LOGGER();

void test_fiber()
{
    for(int i=0;i<3;i++) {
        LOG_INFO(g_logger, "hello_test");
        hxk::Fiber::yieldToHold();
    }
}

void TEST_CreateIOManager()
{
    char buffer[1024];
    const char msg[] = "ni hao";
    hxk::IOManager io_manager(2);
    io_manager.schedule(test_fiber);
    int sockfd;
    sockaddr_in server_addr;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(1);
    }
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8800);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if((connect(sockfd, (struct sockaddr*)(&server_addr), sizeof(server_addr))) == -1) {
        perror("connect error");
        exit(1);
    }
    else {
        fcntl(sockfd, F_SETFL, O_NONBLOCK);
        LOG_INFO(g_logger, "start");
        io_manager.addEventListener(sockfd, hxk::FDEventType::READ, [&](){
            recv(sockfd, buffer, sizeof(buffer), 0);
            LOG_FORMAT_INFO(g_logger, "server response: %s", buffer);
            io_manager.cancelAll(sockfd);
            close(sockfd);
        });
        io_manager.addEventListener(sockfd, hxk::FDEventType::WRITE, [&](){
            memcpy(buffer, msg, sizeof(buffer));
            LOG_FORMAT_INFO(g_logger, "send to server: %s", buffer);
            send(sockfd, buffer, sizeof(buffer), 0);
        });
    }
}

void TEST_timer()
{
    hxk::IOManager io_manager(2);
    io_manager.addTimer(1000, [](){
        LOG_INFO(g_logger, "sleep(1000)");
    }, true);
    io_manager.addTimer(500, [](){
        LOG_INFO(g_logger, "sleep(500)");
    }, true);
}


int main()
{
    // TEST_CreateIOManager();
    TEST_timer();
    return 0;
}
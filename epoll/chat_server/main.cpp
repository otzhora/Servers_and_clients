#include <iostream>


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <map>
#include <algorithm>
#include <sys/epoll.h>
#include <string>
#include <sstream>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_EVENTS 128

using namespace std;


int set_nonblock(int fd)
{
    int flags;
#if defined(O_NONBLOCK)
    if(-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif

}


struct user{
    string name;
};
int main() {

    map<int, user> clients;

    int MasterSocket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr;
    addr.sin_port = htons(8000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_family = AF_INET;

    bind(MasterSocket, (struct sockaddr*)&addr, sizeof(sockaddr));

    set_nonblock(MasterSocket);

    listen(MasterSocket, SOMAXCONN);

    int epoll = epoll_create1(0);

    struct epoll_event e;
    e.data.fd = MasterSocket;
    e.events = EPOLLIN;

    epoll_ctl(epoll, EPOLL_CTL_ADD, MasterSocket, &e);

    bool stop = false;

    while(!stop)
    {
        struct epoll_event events[MAX_EVENTS];
        int size =  epoll_wait(epoll, events, MAX_EVENTS, -1);

        for(int i = 0; i < size; i++)
        {
            if(events[i].data.fd == MasterSocket)
            {
                struct sockaddr_in saddr;
                socklen_t len = sizeof(saddr);

                int slave = accept(MasterSocket, (struct sockaddr*)&saddr, &len);
                set_nonblock(slave);
                struct epoll_event e;
                e.data.fd = slave;
                e.events = EPOLLIN;
                epoll_ctl(epoll, EPOLL_CTL_ADD, slave, &e);

                stringstream str;
                str << inet_ntoa(saddr.sin_addr) << ":" << htons(saddr.sin_port);
                user newUser;
                newUser.name = str.str();

                str << " connected" << endl;

                for(auto i = clients.begin(); i != clients.end(); i++)
                {
                    send(i->first, str.str().data(), str.str().length(), MSG_NOSIGNAL);
                }

                clients[slave] = newUser;
            }
            else
            {
                char Buffer[1024];

                ssize_t recvSize = recv(events[i].data.fd, Buffer, 1024, MSG_NOSIGNAL);

                if(recvSize == 0 && errno != EAGAIN)
                {
                    shutdown(events[i].data.fd, SHUT_RDWR);
                    close(events[i].data.fd);

                    stringstream str;
                    str << clients[events[i].data.fd].name << " disconnected" << endl;
                    clients.erase(events[i].data.fd);
                    for(auto i = clients.begin(); i != clients.end(); i++)
                    {
                        send(i->first, str.str().data(), str.str().length(), MSG_NOSIGNAL);
                    }
                }
                else if(recvSize > 0)
                {
                    Buffer[recvSize] = 0;
                    stringstream str;
                    str << clients[events[i].data.fd].name << " -> " << Buffer;
                    for(auto i = clients.begin(); i != clients.end(); i++)
                    {
                        send(i->first, str.str().data(), str.str().length(), MSG_NOSIGNAL);
                    }
                }
            }

        }


    }
    for(auto i = clients.begin(); i != clients.end(); i++)
    {
        shutdown(i->first, SHUT_RDWR);
        close(i->first);
    }

    shutdown(MasterSocket, SHUT_RDWR);
    close(MasterSocket);
    return 0;
}
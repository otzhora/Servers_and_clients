#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <set>
#include <algorithm>
#include <unistd.h>
#include <poll.h>

#define Poll_size 2048


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


int main() {
    int MasterSocket = socket(AF_INET, SOCK_STREAM, 0);
    set<int> SlaveSockets;
    if(MasterSocket == -1)
    {
        cout << "SOCKET ERROR" << endl;
        return 1;
    }

    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(8000);
    SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);


    if(bind(MasterSocket, (sockaddr*)&SockAddr, sizeof(SockAddr)) == -1)
    {
        cout << "bind Err" << endl;
        return 1;
    }

    set_nonblock(MasterSocket);

    listen(MasterSocket, SOMAXCONN);

    struct pollfd Set[Poll_size];

    Set[0].fd = MasterSocket;
    Set[0].events = POLLIN;
    while(true)
    {
        unsigned int Index = 1;

        for(auto Iter = SlaveSockets.begin(); Iter != SlaveSockets.end(); Iter++)
        {
            Set[Index].fd = *Iter;
            Set[Index].events = POLLIN;
            Index++;
        }

        unsigned int SetSize = 1 + SlaveSockets.size();

        poll(Set, SetSize, -1);

        for(unsigned int i = 0; i < SetSize; i++)
        {
            if(Set[i].revents == POLLIN)
            {
                if(i)
                {
                    char Buffer[1024];
                    int Recvsize = recv(Set[i].fd, Buffer, 1024, MSG_NOSIGNAL);
                    if(Recvsize == 0 && errno != EAGAIN)
                    {
                        shutdown(Set[i].fd, SHUT_RDWR);
                        close(Set[i].fd);
                        SlaveSockets.erase(Set[i].fd);
                    }
                    else if(Recvsize > 0)
                    {
                        send(Set[i].fd, Buffer, 1024, MSG_NOSIGNAL);
                    }

                }
                else
                {
                    int SlaveSocket = accept(MasterSocket, 0, 0);
                    set_nonblock(SlaveSocket);
                    SlaveSockets.insert(SlaveSocket);
                }

            }

        }
    }

    return 0;
}
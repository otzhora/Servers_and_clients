#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <set>
#include <algorithm>
#include <unistd.h>


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


    while(true)
    {
        fd_set Set;
        FD_ZERO(&Set);
        FD_SET(MasterSocket, &Set);

        for(auto Iter = SlaveSockets.begin(); Iter != SlaveSockets.end(); Iter++)
        {
            FD_SET(*Iter, &Set);
        }

        int Max = std::max(MasterSocket, *max_element(SlaveSockets.begin(), SlaveSockets.end()));

        select(Max + 1, &Set, NULL, NULL, NULL);

        for(auto Iter = SlaveSockets.begin(); Iter != SlaveSockets.end(); Iter++)
        {
            if(FD_ISSET(*Iter, &Set))
            {
                char Buffer[1024];
                int RecvSize = recv(*Iter, Buffer, 1024, MSG_NOSIGNAL);
                if(RecvSize == 0 && errno != EAGAIN)
                {
                    shutdown(*Iter, SHUT_RDWR);
                    close(*Iter);
                    SlaveSockets.erase(*Iter);

                } else if(RecvSize != 0)
                {
                    send(*Iter, Buffer, 1024, MSG_NOSIGNAL);
                }

            }

        }
        if(FD_ISSET(MasterSocket, &Set))
        {
            int SlaveSocet = accept(MasterSocket, 0, 0);
            set_nonblock(SlaveSocet);
            SlaveSockets.insert(SlaveSocet);
        }


    }

    return 0;
}
#include <iostream>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

using namespace std;


int main() {

    int s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in sa;
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(8000);

    if(connect(s, (sockaddr*)&sa, sizeof(sa)) == -1) {
        shutdown(s, SHUT_RDWR);
        close(s);
        return 1;
    }

    char Buffer[1024];


    while(true)
    {
        cin >> Buffer;
        if(strcmp(Buffer, "end") == 0)
        {
            shutdown(s, SHUT_RDWR);
            close(s);
            return 0;
        }

        ssize_t SendedBytes = send(s, Buffer, 1024, MSG_NOSIGNAL);
        if(SendedBytes <= 0)
        {
            shutdown(s, SHUT_RDWR);
            close(s);
            return 1;
        }
        ssize_t RecvBytes = recv(s, Buffer, 1024, MSG_NOSIGNAL);
        if(RecvBytes <= 0)
        {
            shutdown(s, SHUT_RDWR);
            close(s);
            return 1;
        }
        cout << Buffer << endl;
    }


    return 0;
}
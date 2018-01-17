#include <netdb.h>
#include <netinet/in.h>

#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <signal.h>
#include <atomic>

using std::string;
using std::cout;
using std::endl;

// Max connections to be handled
const int MAX_CONNECTIONS = 2;
// catch the Ctrl-C event and handle accordingly
void sigintHandler(int);
// function to be processed by a spawned thread, each time a connection is established
void processThread(int newsockfd);


void sigintHandler(int sig_num)
{
    // signal(SIGINT, sigintHandler);
    cout << "\n cleaning up before terminating with Ctrl+C " << sig_num << endl;
    int n = write(sig_num,"Bye\n",4);
    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }
    exit(0);
}

struct  SafeCounter
{
private:
    std::atomic<int32_t> value;
public:
    SafeCounter() : value(0) {}
    void increment() { ++value; }
//    void decrement() { --value; }
    int get() { value.load() ;}
};

void processThread(int newsockfd)
{
       char buffer[256];
    // We have an established connection at this point, trying to read
       bzero(buffer,256);
       int n = read( newsockfd,buffer,255 );

       if (n < 0) {
           perror("ERROR reading from socket");
           exit(1);
       }

       if (buffer[0] == '\n')
       {
           cout << "close request " << endl;
           close(newsockfd);
           return ;
       }

       cout << "Message received " << buffer  << endl;

       /* Write a response to the client */
       n = write(newsockfd,"ACK! Msg Received!",18);
        signal(SIGINT, sigintHandler);
       if (n < 0) {
           perror("ERROR writing to socket");
           exit(1);
       }

       // close socket
       close(newsockfd);
}

int main( int argc, char *argv[] ) {

    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr;
    int  n;

    /* First call to socket() function */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
    }

    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 5001;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    /* Now bind the host address using bind() call.*/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    /* Listening to incoming clients connections with respect to max connections allowed */

    listen(sockfd,MAX_CONNECTIONS);


    /* Accept actual connection from the client */
    while (1)
    {
        sockaddr_in cli_addr;
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (sockaddr *)&cli_addr, &clilen);

        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }
        else
        {

            // create thread
            std::thread th(processThread, newsockfd);
            th.join();
        }



    }
    close(sockfd);
    return 0;
}

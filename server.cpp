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
using std::to_string;

// Struct for ID generation
struct  SafeCounter
{
private:
    static std::atomic<int32_t> value;
public:
    static void increment() { ++value; }

    int get() { value.load() ;}
};

// struct for connected clients
struct  ClientCounter
{
private:
    std::atomic<int32_t> value;
public:
    ClientCounter() : value(0) {}
    void increment() { ++value; }
    void decrement() { --value; }
   int get() { value.load() ;}
};


// Max connections to be handled
const int MAX_CONNECTIONS = 2;
// catch the Ctrl-C event and handle accordingly
void sigintHandler(int);
// function to be processed by a spawned thread, each time a connection is established
void processThread(int newsockfd, ClientCounter &cc);


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

std::atomic<int32_t> SafeCounter::value{0};
void processThread(int newsockfd, ClientCounter & cc )
{
       char buffer[256];
       // We have an established connection at this point, increment value to get ID
       SafeCounter safeC;
       safeC.increment();

       bzero(buffer,256);
       int n = read( newsockfd,buffer,255 );

       if (n < 0) {
           perror("ERROR reading from socket");
           exit(1);
       }

       if (buffer[0] == '\n')
       {
           cout << "clients connected " << cc.get()  << endl;
           int clients = cc.get();
           string response = string("clients connected: ") + std::to_string(clients);
           cout << response << response.length() << endl;
           n = write(newsockfd,response.c_str(),response.length());
           return ;
       }



       cout << "Message received " << buffer  << endl;

       /* Write a response to the client */
       string response = "ACK! Msg Received! Your Id is: " + safeC.get();
       n = write(newsockfd,response.c_str(),response.length());
        signal(SIGINT, sigintHandler);
       if (n < 0) {
           perror("ERROR writing to socket");
           exit(1);
       }

       // decrement client count
       cc.decrement();
       // close socket
       close(newsockfd);
}

int main( int argc, char *argv[] ) {

    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr;
    int  n;
    ClientCounter cc;

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
            // client connected increment var
            cc.increment();
            // create thread
            auto socketThread = std::make_shared<int>(newsockfd);
            std::thread th(processThread, *socketThread, std::ref(cc));
            th.join();
            cc.decrement();
        }



    }
    close(sockfd);
    return 0;
}

#include <utils/TCPServer.hpp>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <utils/logger.h>
#include <network/net.h>

#define wiiu_errno (*__gh_errno_ptr())

TCPServer::TCPServer(int32_t port,int32_t priority) {
    this->port = port;
    this->sockfd = -1;
    this->clientfd = -1;
    memset(&(this->sock_addr),0,sizeof(this->sock_addr));

    pThread = CThread::create(TCPServer::DoTCPThread, (void*)this, CThread::eAttributeAffCore2,priority);
    pThread->resumeThread();
}

TCPServer::~TCPServer() {
    CloseSockets();
    //DEBUG_FUNCTION_LINE("Thread will be closed");
    exitThread = 1;

    ICInvalidateRange((void*)&exitThread, 4);
    DCFlushRange((void*)&exitThread, 4);

    if(pThread != NULL) {
        //DEBUG_FUNCTION_LINE("Deleting it!");
        delete pThread;
    }
    //DEBUG_FUNCTION_LINE("Thread done");
    pThread = NULL;
}

void TCPServer::CloseSockets() {
    if (this->sockfd != -1) {
        socketclose(this->sockfd);
    }
    if (this->clientfd != -1) {
        socketclose(this->clientfd);
    }
    this->sockfd = -1;
    this->clientfd = -1;
}

void TCPServer::ErrorHandling() {
    CloseSockets();
    OSSleepTicks(OSMicrosecondsToTicks(1000*1000*2));
}

void TCPServer::DoTCPThreadInternal() {
    int32_t ret;
    socklen_t len;
    connected = false;
    while (1) {
        if(exitThread) {
            break;
        }
        memset(&(this->sock_addr),0,sizeof(sock_addr));
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_port = this->port;
        sock_addr.sin_addr.s_addr = 0;

        this->sockfd = ret = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(ret == -1) {
            ErrorHandling();
            continue;
        }
        int32_t enable = 1;

        setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

        ret = bind(this->sockfd, (sockaddr *)&sock_addr, 16);
        if(ret < 0) {
            ErrorHandling();
            continue;
        }
        ret = listen(this->sockfd, 1);
        if(ret < 0) {
            ErrorHandling();
            continue;
        }

        do {
            DEBUG_FUNCTION_LINE("Waiting for a connection");
            if(exitThread) {
                break;
            }
            len = 16;
            clientfd = ret = accept(sockfd, (sockaddr *)&(sock_addr), &len);

            if(ret == -1) {
                ErrorHandling();
                break;
            }

            if(!acceptConnection()) {
                ErrorHandling();
                break;
            }

            connected = true;

            DEBUG_FUNCTION_LINE("Connection accepted");

            whileLoop();

            DEBUG_FUNCTION_LINE("Client disconnected");

            if(clientfd != -1) {
                socketclose(clientfd);
            }
            clientfd = -1;
        } while(0);
        DEBUG_FUNCTION_LINE("Closing TCPServer");
        connected = false;
        onConnectionClosed();
        CloseSockets();
        continue;
    }
    DEBUG_FUNCTION_LINE("Ending DoTCPThreadInternal");
}

void TCPServer::DoTCPThread(CThread *thread, void *arg) {
    TCPServer * args = (TCPServer * )arg;
    return args->DoTCPThreadInternal();
}
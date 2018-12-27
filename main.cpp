#include <iostream>
#include <cstdint>
#include "lodepng.h"

//TCP foo
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

//int to HEX
#include <sstream>

//hardcoded parameters
unsigned int threadNum = 8;
unsigned int screenWidth = 1920;
unsigned int screenHeight = 1080;
const char hex_lookup[] = "0123456789abcdef";

std::vector<int> sockfds;
std::vector<std::string> imgCmds;//rrggbb -> size 6
std::vector<unsigned char> image; //the raw pixels
unsigned width, height;

std::pair<unsigned int, unsigned int> iToXY(unsigned int i) {
    return std::make_pair(i / width, i % width);
}


void sendToSocket(unsigned int tid, std::string const& msg) {
    int ret = -1;
    unsigned int sent = 0;

    do {
        ret = send(sockfds[tid], msg.c_str() + sent, msg.length() - sent, 0);

        if(ret == -1) {
            perror("send");
            break;
        }

        sent += ret;
    }
    while(msg.length() - sent > 0);
}

void sendThread(unsigned int lower, unsigned int upper, unsigned int tid) {
    int numbytes;
    char buf[256];
    struct hostent* he;
    struct sockaddr_in their_addr; /* connector's address information */

    if((he = gethostbyname("151.217.40.82")) == NULL) { /* get the host info */
        herror("gethostbyname");
        exit(1);
    }

    if((sockfds[tid] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    their_addr.sin_family = AF_INET;      /* host byte order */
    their_addr.sin_port = htons(1234);    /* short, network byte order */
    their_addr.sin_addr = *((struct in_addr*)he->h_addr);
    bzero(&(their_addr.sin_zero), 8);     /* zero the rest of the struct */

    if(connect(sockfds[tid], (struct sockaddr*)&their_addr, \
               sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

    for(unsigned int i = 0; i < 1920; i++) {
        for(unsigned int j = 0; j < 1080; j++)
            sendToSocket(tid, "PX " + std::to_string(i) + " " + std::to_string(j) + " 000000");
    }
}

int main(int argc, char* argv[]) {
    sockfds = std::vector<int>(threadNum);

    unsigned error = lodepng::decode(image, width, height, std::string(argv[1]));
    std::cout << width << std::endl << height << std::endl;
    std::cout << iToXY(256).first << " " << iToXY(256).second << " " << iToXY(513).first << " " << iToXY(513).second << " " << " " << std::endl << height << std::endl;

    std::stringstream cmd;

    /*for(unsigned int i = 0; i < 1920; i++) {
        for(unsigned int j = 0; j < 1080; j++) {

        }

        //std::pair<unsigned int, unsigned int> xy = iToXY(i);
        cmd << std::hex << (image[i] < 16 ? "0" : "") << static_cast<unsigned int>(image[i])
            << (image[i + 1] < 16 ? "0" : "") << static_cast<unsigned int>(image[i + 1])
            << (image[i + 2] < 16 ? "0" : "") << static_cast<unsigned int>(image[i + 2]);
        imgCmds.push_back(cmd.str());

        cmd.clear();

        if(i % 1000 == 0)
            std::cout << i << std::endl;

    }*/

//for(size_t i = 0; i < width * height; i += 4)
//std::cout << "R: " << static_cast<unsigned int>(image[i]) << std::endl;

//sendThread(0, height * width - 1, 0);
}
#include <iostream>
#include <cstdint>
#include <atomic>
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

#include "concurrentqueue.h"

//hardcoded parameters
unsigned int threadNum = 8;
unsigned int screenWidth = 1920;
unsigned int screenHeight = 1080;
std::string host = "127.0.0.1";//"151.217.40.82"
unsigned int port = 1337;//1234
const char hex_lookup[] = "0123456789abcdef";

std::vector<int> sockfds;
std::vector<std::string> imgCmds;//rrggbb -> size 6
std::vector<unsigned char> image; //the raw pixels
unsigned width, height;
std::vector<std::string> iToStr(std::max(screenWidth, screenHeight) + 1);
std::vector<std::string> iToHex(256);

moodycamel::ConcurrentQueue<unsigned int> nextJobs;
std::atomic<unsigned int> currIn(0);

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

void sendThread(unsigned int tid) {
    int numbytes;
    struct hostent* he;
    struct sockaddr_in their_addr; /* connector's address information */

    if((he = gethostbyname(host.c_str())) == NULL) { /* get the host info */
        herror("gethostbyname");
        exit(1);
    }

    if((sockfds[tid] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    their_addr.sin_family = AF_INET;      /* host byte order */
    their_addr.sin_port = htons(port);    /* short, network byte order */
    their_addr.sin_addr = *((struct in_addr*)he->h_addr);
    bzero(&(their_addr.sin_zero), 8);     /* zero the rest of the struct */

    if(connect(sockfds[tid], (struct sockaddr*)&their_addr, \
               sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }

    while(true) {
        unsigned int i;

        if(!nextJobs.try_dequeue(i))
            continue;

        currIn.fetch_sub(1);

        if(image[i + 3] > 0)
            sendToSocket(tid, "PX " + iToStr[i / 4 / width] + " " + iToStr[i / 4 % width] + " " + iToHex[image[i]] + iToHex[image[i + 1]] + iToHex[image[i + 2]] + iToHex[image[i + 3]] + "\n");

        //std::cout << "PX " + std::to_string(i) + " " + std::to_string(j) + " 000000\n" << std::endl;
    }
}


int main(int argc, char* argv[]) {
    for(int i = 0; i <= std::max(screenWidth, screenHeight); i++)
        iToStr[i] = std::to_string(i);

    for(int i = 0; i <= 255; i++) {
        iToHex[i] = "";
        iToHex[i] += hex_lookup[i / 16];
        iToHex[i] += hex_lookup[i % 16];
        std::cout << i << " " << iToHex[i] << std::endl;
    }

    sockfds = std::vector<int>(threadNum);

    unsigned error = lodepng::decode(image, width, height, std::string(argv[1]));
    std::cout << width << std::endl << height << std::endl;
    std::cout << iToXY(256).first << " " << iToXY(256).second << " " << iToXY(513).first << " " << iToXY(513).second << " " << " " << std::endl << height << std::endl;


    std::thread producer([&]() {
        while(true) {
            for(int i = 0; i < width * height; ++i) {
                if(currIn < width * height * 10)
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                else {
                    nextJobs.enqueue(i * 4);
                    currIn.fetch_add(1);
                }
            }

            std::cout << currIn << std::endl;
        }
    });

    std::vector<std::thread> consumers;

    for(size_t i = 0; i < threadNum; i++) {
        consumers.push_back(std::thread([&]() {
            sendThread(i);
        }));
    }

    producer.join();

    for(size_t i = 0; i < consumers.size(); i++)
        consumers[i].join();

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
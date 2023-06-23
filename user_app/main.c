#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define BUF_LEN 10

const char* RED = "RED";
const char* YELLOW = "YELLOW";
const char* GREEN = "GREEN";
const char* LED_DRIVER = "/dev/led_driver";

const int RED_SLEEP = 5; 
const int YELLOW_SLEEP = 2;
const int GREEN_SLEEP = 4;  

int send_to_drivers(const char* msg){
    int led_fd = open(LED_DRIVER, O_RDWR);
    if(led_fd < 0){
        printf("Error, %s not opened\n", LED_DRIVER);
        return -1;
    }
    write(led_fd, msg, BUF_LEN);
    close(led_fd);
    return 0;
}

int main()
{
    while(1){
        if(send_to_drivers(RED) == -1){
            perror("FATAL ERROR: Failed writing to drivers");
            return -1;
        }
        sleep(RED_SLEEP);
        if(send_to_drivers(YELLOW) == -1){
            perror("FATAL ERROR: Failed writing to drivers");
            return -1;
        }
        sleep(YELLOW_SLEEP);
        if(send_to_drivers(GREEN) == -1){
            perror("FATAL ERROR: Failed writing to drivers");
            return -1;
        }
        sleep(GREEN_SLEEP);
    }
    return 0;
}

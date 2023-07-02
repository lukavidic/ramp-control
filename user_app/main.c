#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define BUF_LEN 10 /* Char buffer length holding messages for LED drvier */

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; /* Mutex controlling servo movement critical section */

/* LED driver messages */
const char* RED = "RED";
const char* YELLOW = "YELLOW";
const char* GREEN = "GREEN";
/* Servo driver messages */
const char* MOV_UP = "b"; // SAME FOR BUZZER TO BUZZ
const char* MOV_DOWN = "e";

/* Paths to char driver files */
const char* LED_DRIVER = "/dev/led_driver";
const char* PWM_DRIVER = "/dev/pwm_driver";
const char* BUZZ_DRIVER = "/dev/buzz_driver";
const char* ADC_DRIVER = "/dev/adc_driver";

/* Sleep times for different semaphore lights, customizable */
const int RED_SLEEP = 5; 
const int YELLOW_SLEEP = 2;
const int GREEN_SLEEP = 4;  

/* File descriptors for all driver files after opening */
int led_fd, pwm_fd, buzz_fd, adc_fd;

/* Flag to indicate when sensor has detected an object, used to restart the semaphore */
char flag = 0x00;

/* SIGINT handler function, closes driver files */
void kill_handler(int signo, siginfo_t *info, void *context){
    if(signo==SIGINT){
        close(led_fd);
        close(pwm_fd);
        close(buzz_fd);
        close(adc_fd);
        exit(1);
    }
}

/* Function that sends a message to LED driver and moves servo in correct direction depending on the message being sent */
void send_to_drivers(const char* msg){
    pthread_mutex_lock(&mtx);
        if(flag > 0)
            return;
        write(led_fd, msg, BUF_LEN);
        if(strcmp(RED,msg) == 0)
            write(pwm_fd, MOV_DOWN, strlen(MOV_DOWN));
        else if(strcmp(GREEN,msg) == 0)
            write(pwm_fd, MOV_UP, strlen(MOV_UP));
    pthread_mutex_unlock(&mtx);
    return;
}

/* Function that opens all device files and checks for errors */
int open_drivers(void){
    led_fd = open(LED_DRIVER, O_RDWR);
    pwm_fd = open(PWM_DRIVER, O_RDWR);
    buzz_fd = open(BUZZ_DRIVER, O_RDWR);
    adc_fd = open(ADC_DRIVER, O_RDWR);
    if(led_fd < 0 || pwm_fd < 0 || buzz_fd < 0 || adc_fd < 0)
        return -1;
    return 0;
}

/* 
    Thread function reading data from ADC (sensor), comparing it to threshold value, and determining if object in close enough for 
    servo to go upand buzzer to buzz
*/
void* sensor_controller_fun(void* param){
    char data[2];
    char thrs = 0x07;
    while(1){
        read(adc_fd, data, 2);
        if(data[0] > thrs){
            pthread_mutex_lock(&mtx);
                write(pwm_fd, MOV_UP, strlen(MOV_UP));
                write(buzz_fd, MOV_UP, strlen(MOV_UP));
                write(led_fd, "", 1);
                flag = 1;
                sleep(RED_SLEEP); // Sleep for same as red light
            pthread_mutex_unlock(&mtx);
        }
    }
}

/* Main thread, controlling nominal work of servo and LEDs */
int main()
{
    pthread_t sensor_controller_th;
    struct sigaction act;
    memset(&act,0,sizeof(act));
    act.sa_sigaction=kill_handler;
    act.sa_flags=SA_SIGINFO;
    sigaction(SIGINT,&act,NULL);

    if(open_drivers() < 0){
        perror("FATAL ERROR: Failed opening device files !!\n");
        return -1;
    }

    pthread_create(&sensor_controller_th, NULL, sensor_controller_fun, NULL);

    while(1){
        goback:
        send_to_drivers(RED);
        if(flag > 0){
            flag = 0;
            goto goback;
        }
        sleep(RED_SLEEP);
        send_to_drivers(YELLOW);
        if(flag > 0){
            flag = 0;
            goto goback;
        }
        sleep(YELLOW_SLEEP);
        send_to_drivers(GREEN);
        if(flag > 0){
            flag = 0;
            goto goback;
        }
        sleep(GREEN_SLEEP);
        send_to_drivers(YELLOW);
        if(flag > 0){
            flag = 0;
            goto goback;
        }
        sleep(YELLOW_SLEEP);
    }
    return 0;
}

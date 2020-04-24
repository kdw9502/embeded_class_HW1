#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>

#include <sys/mman.h>

#define BUFF_SIZE 64

#define KEY_RELEASE 0
#define KEY_PRESS 1

#define BACK_KEY_CODE 158
#define VOLUME_UP 115
#define VOLUME_DOWN 114

#define CLOCK_MODE 0
#define COUNTER_MODE 1
#define TEXT_MODE 2
#define DRAW_MODE 3
#define MODE_COUNT 4

#define INPUT_SEQ 0
#define MAIN_SEQ 1
#define OUTPUT_SEQ 2

#define DELAY 200000

#define MAX_BUTTON 9

int button_mid, mode_mid, value_mid;

void input_process() {

    while (1) {
        read_hw_key(mode_mid);
        read_fpga_key(button_mid);
        usleep(DELAY);
    }
    return 0;
}

void main_process() {
    while (1) {
        int *button_addr;
        int *mode_addr;
        button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);
        mode_addr = (int *) shmat(mode_mid, (int*)NULL,0);

        usleep(DELAY);
    }
}

void output_process(int button_mid, int mode_mid) {
    int *button_addr;
    int *mode_addr;
    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);
    mode_addr = (int *) shmat(mode_mid, (int*)NULL,0);
}



void read_hw_key() {
    char* device = "/dev/input/event0";
    int* mode;
    mode = (int*)shmat(mode_mid, (int*) NULL,0);

    if ((fd = open(device, O_RDONLY | O_NONBLOCK)) == -1) {
        printf("%s is not a vaild device\\n", device);
    }

    if ((rd = read(fd, ev, size * BUFF_SIZE)) < size) {
        printf("read()");
        continue;
    }
    if (ev[0].type == 1 && ev[0].value == 1) // on key press
    {
        if (ev[0].code == BACK_KEY_CODE) {
            close(fd);
            exit(0);
        } else {
            if (code == VOLUME_UP) {
                mode[0] = mode[0] + 1 % MODE_COUNT;
            } else if (code == VOLUME_DOWN) {
                mode[0] = mode[0] + MODE_COUNT - 1 % MODE_COUNT;
            }
        }
    }
    close(fd);
}

void read_fpga_key() {
    int i;
    int dev;
    int buff_size;
    unsigned char *button_addr;

    if (dev = open("/dev/fpga_push_switch", O_RDWR) == -1)
    {
        printf("Device Open Error\n");
        close(dev);
        return;
    }

    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);

    read(dev, &button_addr, buff_size);

    for(i=0;i<MAX_BUTTON;i++) {
        printf("[%d] ",button_addr[i]);
    }
    printf("\n");
    shmdt(button_addr);
    close(dev);
}

void clock() {

}

int main() {
    button_mid = shmget(IPC_PRIVATE, 50, IPC_CREAT | 0644);
    mode_mid = shmget(IPC_PRIVATE, 10, IPC_CREAT | 0644);
    value_mid = shmget(IPC_PRIVATE, 1000, IPC_CREAT | 0644);
    if (button_mid == -1) {
        perror("shmget");
        exit(1);
    }


    switch (fork()) {
        case -1: //fail
            perror("fork");
            exit(1);
            break;
        case 0: // child
            input_process();
            break;
        default: //parent
            switch (fork()) {
                case -1: //fail
                    perror("second fork");
                    exit(1);
                    break;
                case 0: // child
                    output_process();
                    break;
                default:
                    main_process();
            }
            break;
    }
}


#include "main.h"

void input_process() {
    if (fpga_switch_device = open("/dev/fpga_push_switch", O_RDWR) == -1)
    {
        printf("Device Open Error\n");
        return;
    }
    while (1) {
        read_hw_key(mode_mid);
        //read_fpga_key(button_mid);
        usleep(DELAY);
    }
}

void main_process() {
    int *mode_addr;
    mode_addr = (int *) shmat(mode_mid, (int*)NULL,0);
    mode_addr[0] = CLOCK_MODE + MODE_CHANGED;


    while (1) {
        mode_addr = (int *) shmat(mode_mid, (int*)NULL,0);

        if (mode_addr[0] > MODE_CHANGED)
        {
            mode_addr[0] -= MODE_CHANGED;
            reset_value(mode_addr[0]);
        }

        switch (mode_addr[0])
        {
            case CLOCK_MODE:
                clock_process();
            case COUNTER_MODE:
                counter_process();
            case TEXT_MODE:
                text_editor_process();
            case DRAW_MODE:
                draw_board_process();
        }

        usleep(DELAY);
    }
}

void output_process() {
//    int *button_addr;
//    int *mode_addr;
//    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);
//    mode_addr = (int *) shmat(mode_mid, (int*)NULL,0);
}

void reset_value(int mode)
{
    int *mode_addr = (int *) shmat(mode_mid, (int *) NULL, 0);
    void* value_addr = (void*) shmat(value_mid, (void*) NULL,0);
    switch (mode)
    {
        case CLOCK_MODE:
            value_addr = malloc(sizeof(clock_values));
            clock_values *a = (clock_values*)value_addr;
            a->time = 0;
            a->bonus_time = 0;

            break;
        case COUNTER_MODE:
            value_addr = malloc(sizeof(counter_values));
            counter_values *b = (counter_values*)value_addr;
            b->exponent = 10;
            b->value = 0;
            break;
        case TEXT_MODE:
            value_addr = malloc(sizeof(text_editor_values));
            text_editor_values *c = (text_editor_values*)value_addr;
            c->count=0;
            c->is_letter_mode=True;
            c->length=0;
            c->prev_value=0;
            c->string = (char*)malloc(sizeof(char*)*8);
            c->string[0] = '\0';
            break;
        case DRAW_MODE:
            value_addr = malloc(sizeof(draw_board_values));
            draw_board_values *d = (draw_board_values*)value_addr;
            d->count = 0;
            d->board = (char*)malloc(sizeof(char*)*200);
            d->cursor_point = 0;
            break;
    }
    shmdt(mode_addr);
    shmdt(value_addr);
}

void read_hw_key() {
#ifdef DEBUG
    printf("read_hw\n");
#endif
    char* device = "/dev/input/event0";
    int* mode;
    struct input_event ev[BUFF_SIZE];
    int fd, rd, size = sizeof (struct input_event);
    mode = (int*)shmat(mode_mid, (int*) NULL,0);

    if ((fd = open(device, O_RDONLY | O_NONBLOCK)) == -1) {
        printf("%s is not a vaild device\n", device);
        return;
    }

    if ((rd = read(fd, ev, size * BUFF_SIZE)) < size) {
#ifdef DEBUG
        printf("hw wait return\n");
#endif
        close(fd);
        return;
    }
    if (ev[0].type == 1 && ev[0].value == 1) // on key press
    {
        if (ev[0].code == BACK_KEY_CODE) {
            close(fd);
            exit(0);
        } else {
            if (ev[0].code == VOLUME_UP) {
                mode[0] = mode[0] + 1 % MODE_COUNT + MODE_CHANGED;
            } else if (ev[0].code == VOLUME_DOWN) {
                mode[0] = mode[0] + MODE_COUNT - 1 % MODE_COUNT + MODE_CHANGED;
            }
        }
    }
#ifdef DEBUG
    printf("mode is %d", mode[0]);
#endif
    shmdt(mode);
    close(fd);
}

void read_fpga_key() {
    int i;
    int buff_size;
    unsigned char *button_addr;

    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);

    printf("%d", fpga_switch_device);
    read(fpga_switch_device, &button_addr, buff_size);
#ifdef DEBUG
    for(i=0;i<MAX_BUTTON;i++) {
        printf("[%d] ",button_addr[i]);
    }
#endif
    printf("\n");
    shmdt(button_addr);
}

void clock_process() {
    int *button_addr;
    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);
    clock_values* clockValues;
    clockValues = (clock_values *) shmat(value_mid, (clock_values*)NULL, 0);

    if (button_addr[2] == 1)
        clockValues->bonus_time = 0;

    if (button_addr[3] == 1)
        clockValues->bonus_time += 3600;

    if (button_addr[4] == 1)
        clockValues->bonus_time += 60;



    char command[30];
    sprintf(command, "date -d \"+%s hours\" \"+%s min\"");
    if (button_addr[1] == 1)
        system(command);
}

void counter_process()
{

}

void text_editor_process()
{

}

void draw_board_process()
{
    
}

void set_fnd(int value)
{
    int dev;
    unsigned char data[5];
    memset(data,0,sizeof(data));

    dev = open("/dev/fpga_fnd", O_RDWR);

    sprintf(data,"%04d",value);

    write(dev,&data,4);
    close(dev);
}

void clock_output()
{
    clock_values* clockValues;
    clockValues = (clock_values *) shmat(value_mid, (clock_values*)NULL, 0);
    time_t now;
    now = time(NULL) + clockValues->bonus_time;

    int hour = now / 3600 % 24;
    int min = now % 60;

    set_fnd(hour*100 + min);
}

int main() {
    button_mid = shmget(IPC_PRIVATE, 50, IPC_CREAT | 0644);
    mode_mid = shmget(IPC_PRIVATE, 10, IPC_CREAT | 0644);
    value_mid = shmget(IPC_PRIVATE, 1000, IPC_CREAT | 0644);
    if (button_mid == -1 || mode_mid == -1 || value_mid == -1) {
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


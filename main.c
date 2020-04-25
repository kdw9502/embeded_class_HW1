#include "main.h"

void input_process() {
    printf("1\n");
    if ((fpga_switch_device = open("/dev/fpga_push_switch", O_RDWR)) == -1)
    {
        printf("Switch Device Open Error\n");
        return;
    }
    if ((hw_button_device = open("/dev/input/event0",O_RDONLY | O_NONBLOCK)) == -1)
    {
        printf("Hardware key Device Open Error\n");
        return;
    }
    while (1) {
        printf("2\n");
        read_hw_key(mode_mid);
        //read_fpga_key(button_mid);
        usleep(DELAY);
    }
}

void main_process() {
    printf("3\n");
    int *mode_addr;
    while (1) {
        printf("4\n");
        mode_addr = (int *) shmat(mode_mid, (int*)NULL,0);

        if (mode_addr[0] >= MODE_CHANGED)
        {
            printf("5\n");
            mode_addr[0] -= MODE_CHANGED;
            reset_value(mode_addr[0]);
        }
        printf("15\n");
        switch (mode_addr[0])
        {
            case CLOCK_MODE:
                printf("16\n");
                clock_process();
            case COUNTER_MODE:
                counter_process();
            case TEXT_MODE:
                text_editor_process();
            case DRAW_MODE:
                draw_board_process();
        }
        printf("17\n");
        shmdt(mode_addr);

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
    printf("6\n");
    void* value_addr = (void*) shmat(value_mid, (void*) NULL,0);
    printf("7\n");
    switch (mode)
    {
        case CLOCK_MODE:
            printf("77\n");
            value_addr = malloc(sizeof(clock_values));
            printf("8\n");
            clock_values *a = (clock_values*)value_addr;
            printf("9\n");
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
    printf("10\n");
    shmdt(value_addr);
    printf("11\n");
}

void read_hw_key() {
    char* device = "/dev/input/event0";
    int* mode;
    struct input_event ev[BUFF_SIZE];
    int rd, size = sizeof (struct input_event);
    mode = (int*)shmat(mode_mid, (int*) NULL,0);

    if ((rd = read(hw_button_device, ev, size * BUFF_SIZE)) < size) {
        return;
    }

    if (ev[0].type == 1 && ev[0].value == 1) // on key press
    {
        if (ev[0].code == BACK_KEY_CODE) {
            exit(0);
        } else {
            if (ev[0].code == VOLUME_UP) {
                mode[0] = (mode[0] + 1) % MODE_COUNT + MODE_CHANGED;
            } else if (ev[0].code == VOLUME_DOWN) {
                mode[0] = (mode[0] + MODE_COUNT - 1) % MODE_COUNT + MODE_CHANGED;
            }
        }
    }
#ifdef DEBUG
    printf("mode is %d\n", mode[0]);
#endif
    shmdt(mode);
}

void read_fpga_key() {
    int i;
    int buff_size;
    unsigned char *button_addr;

    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);

    printf("fpga_switch_device :%d\n", fpga_switch_device);
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
    unsigned char *button_addr;
    printf("18\n");
    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);
    printf("19\n");
    clock_values* clockValues;
    clockValues = (clock_values *) shmat(value_mid, (clock_values*)NULL, 0);
    printf("20\n");

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

    sprintf(data,"%04d\n",value);

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

    int *mode_addr;
    mode_addr = (int *) shmat(mode_mid, (int*)NULL,0);
    mode_addr[0] = CLOCK_MODE + MODE_CHANGED;
    shmdt(mode_addr);

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
                    break;
            }
            break;
    }
}


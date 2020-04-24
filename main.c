#include "main.h"

void input_process() {

    while (1) {
        read_hw_key(mode_mid);
        read_fpga_key(button_mid);
        usleep(DELAY);
    }
}

void main_process() {
    int *mode_addr;
    mode_addr = (int *) shmat(mode_mid, (int*)NULL,0);
    mode_addr[0] = CLOCK_MODE + MODE_CHANGED;

    while (1) {
        int *button_addr;

        button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);
        mode_addr = (int *) shmat(mode_mid, (int*)NULL,0);

        if (mode_addr[0] > MODE_CHANGED)
        {
            mode_addr[0] -= MODE_CHANGED;
            reset_value(mode_addr[0]);
        }

        switch (mode_addr[0])
        {
            case CLOCK_MODE:
                clock();
            case COUNTER_MODE:
                counter();
            case TEXT_MODE:
                text_editor();
            case DRAW_MODE:
                draw_board();
        }

        usleep(DELAY);
    }
}

void output_process(int button_mid, int mode_mid) {
    int *button_addr;
    int *mode_addr;
    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);
    mode_addr = (int *) shmat(mode_mid, (int*)NULL,0);
}

void reset_value(int mode)
{
    int *mode_addr = (int char *) shmat(mode_mid, (unsigned char *) NULL, 0);
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
            c->prev_value='';
            c->string = (char*)malloc(sizeof(char*)*8);
            c->string[0] = '\0';
            break;
        case DRAW_MODE:
            value_addr = main(sizeof(draw_board_values));
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
                mode[0] = mode[0] + 1 % MODE_COUNT + MODE_CHANGED;
            } else if (code == VOLUME_DOWN) {
                mode[0] = mode[0] + MODE_COUNT - 1 % MODE_COUNT + MODE_CHANGED;
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


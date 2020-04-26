#include "main.h"

void input_process() {
    printf("init input process\n");
    if ((fpga_switch_device = open("/dev/fpga_push_switch", O_RDWR | O_NONBLOCK)) == -1) {
        printf("Switch Device Open Error\n");
//        return;
    }
    if ((hw_button_device = open("/dev/input/event0", O_RDONLY | O_NONBLOCK)) == -1) {
        printf("Hardware key Device Open Error\n");
//        return;
    }
    while (1) {
        read_hw_key(mode_mid);
        read_fpga_key(button_mid);
        usleep(DELAY);
    }
}

void main_process() {
    printf("init main process\n");
    int *mode_addr;
    text_editor_map_setting();

    while (1) {
        mode_addr = (int *) shmat(mode_mid, (int *) NULL, 0);
        if (mode_addr[0] >= MODE_CHANGED) {
            mode_addr[0] -= MODE_CHANGED;
            printf("mode changed : %d\n", mode_addr[0]);
            reset_value(mode_addr[0]);
        }
//        printf("mode main:%d\n", mode_addr[0]);
        switch (mode_addr[0]) {
            case CLOCK_MODE:
                clock_process();
                break;
            case COUNTER_MODE:
                counter_process();
                break;
            case TEXT_MODE:
                text_editor_process();
                break;
            case DRAW_MODE:
                draw_board_process();
                break;
        }
        //shmdt(mode_addr);

        usleep(DELAY);
    }
}

void output_process() {
    printf("init output process\n");
    int *mode_addr;

    if ((fpga_led_mmap = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
        printf("led disabled\n");
        return;
    }

    if ((fpga_fnd_device = open("/dev/fpga_fnd", O_RDWR)) == -1) {
        printf("fnd disabled\n");
        return;
    }

    if ((fpga_lcd_device = open("/dev/fpga_text_lcd", O_WRONLY)) == -1){
        printf("lcd disabled\n");
        return;
    }


    while (1) {
        mode_addr = (int *) shmat(mode_mid, (int *) NULL, 0);
//        printf("output mode : %d",mode_addr[0]);

        switch (mode_addr[0]) {
            case CLOCK_MODE:
                clock_output();
                break;
            case COUNTER_MODE:
                counter_output();
                break;
            case TEXT_MODE:
                text_editor_output();
                break;
            case DRAW_MODE:
                draw_board_output();
                break;
        }
        //shmdt(mode_addr);
        usleep(DELAY * 2);
    }
}

void reset_value(int mode) {
    // reset memory to 0
    unsigned char *value_addr = (unsigned char *) shmat(value_mid, (unsigned char *) NULL, 0);
    memset(value_addr,0,1000);

    clock_values *a;
    counter_values *b;
    text_editor_values *c;
    draw_board_values *d;
    switch (mode) {
        case CLOCK_MODE:
//            printf("reset to clock");
            a = (clock_values *) value_addr;
            a->time = 0;
            a->bonus_time = 0;
            a->editable = False;
            break;
        case COUNTER_MODE:
//            printf("reset to counter");
            b = (counter_values *) value_addr;
            b->exponent = 10;
            b->value = 0;
            break;
        case TEXT_MODE:
            c = (text_editor_values *) value_addr;
            c->count = 0;
            c->is_letter_mode = True;
            c->prev_value = -1;
            c->string[0] = '\0';
            c->editing_index = 0;
            break;
        case DRAW_MODE:
            d = (draw_board_values *) value_addr;
            d->count = 0;
            d->cursor_point = 0;
            d->board[0] = '\0';
            break;
    }
    //shmdt(value_addr);
}

void read_hw_key() {
    char *device = "/dev/input/event0";
    int *mode;
    struct input_event ev[BUFF_SIZE];
    int rd, size = sizeof(struct input_event);
    mode = (int *) shmat(mode_mid, (int *) NULL, 0);

    if ((rd = read(hw_button_device, ev, size * BUFF_SIZE)) < size) {
        return;
    }

    if (ev[0].type == 1 && ev[0].value == 1) // on key press
    {
        if (ev[0].code == BACK_KEY_CODE) {
            mode[0] = EXIT;
        } else if (ev[0].code == VOLUME_UP) {
            mode[0] = (mode[0] + 1) % MODE_COUNT + MODE_CHANGED;
        } else if (ev[0].code == VOLUME_DOWN) {
            mode[0] = (mode[0] + MODE_COUNT - 1) % MODE_COUNT + MODE_CHANGED;
        }
    }
#ifdef DEBUG
//    printf("mode is %d\n", mode[0]);
#endif
    //shmdt(mode);
}

void read_fpga_key() {
    int i;
    int buff_size;
    unsigned char *button_addr;
    unsigned char buffer[MAX_BUTTON];

    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);

    read(fpga_switch_device, &buffer, sizeof(buffer));
    for (i = 0; i < MAX_BUTTON; i++) {
        button_addr[i] = buffer[i];
    }

#ifdef DEBUG
//    for (i = 0; i < MAX_BUTTON; i++) {
//        printf("[%d] ", button_addr[i]);
//    }
//    printf("\n");
#endif

    //shmdt(button_addr);
}

void clock_process() {
    struct timeval timeval;
    unsigned char *button_addr;
    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);
    clock_values *clockValues;
    clockValues = (clock_values *) shmat(value_mid, (clock_values *) NULL, 0);


    if (button_addr[0] == 1) {
        if (clockValues->editable == True) {
            clockValues->editable = False;

            gettimeofday(&timeval, NULL);
            timeval.tv_sec += clockValues->bonus_time;
            settimeofday(&timeval, NULL);
            clockValues->bonus_time = 0;
        } else {
            clockValues->editable = True;
        }
    }

    if (button_addr[1] == True && clockValues->editable)
        clockValues->bonus_time = 0;

    if (button_addr[2] == True && clockValues->editable)
        clockValues->bonus_time += 3600;

    if (button_addr[3] == True && clockValues->editable)
        clockValues->bonus_time += 60;

    //shmdt(button_addr);
    //shmdt(clockValues);
}

unsigned char next_exp_map[11] = {0, 0, 10, 0, 2, 0, 0, 0, 4, 0, 8};

void counter_process() {
    unsigned char *button_addr;
    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);
    counter_values *counterValues;
    counterValues = (counter_values *) shmat(value_mid, (counter_values *) NULL, 0);

    if (button_addr[0] == True) {
        counterValues->exponent = next_exp_map[counterValues->exponent];
    } else if (button_addr[1] == True) {
        counterValues->value += counterValues->exponent * counterValues->exponent;
    } else if (button_addr[2] == True) {
        counterValues->value += counterValues->exponent;
    } else if (button_addr[3] == True) {
        counterValues->value += 1;
    }

    printf("main val, exp : %d %d\n", counterValues->value, counterValues->exponent);


    //shmdt(button_addr);
    //shmdt(counterValues);
}



void text_editor_map_setting()
{
    start_text_map[0] = '.';
    next_value_map['.'] = 'Q';
    next_value_map['Q'] = 'Z';
    next_value_map['Z'] = '.';

    start_text_map[1] = 'A';
    next_value_map['A'] = 'B';
    next_value_map['B'] = 'C';
    next_value_map['C'] = 'A';

    start_text_map[2] = 'D';
    next_value_map['D'] = 'E';
    next_value_map['E'] = 'F';
    next_value_map['F'] = 'D';

    start_text_map[3] = 'G';
    next_value_map['G'] = 'H';
    next_value_map['H'] = 'I';
    next_value_map['I'] = 'G';

    start_text_map[4] = 'J';
    next_value_map['J'] = 'K';
    next_value_map['K'] = 'L';
    next_value_map['L'] = 'J';

    start_text_map[5] = 'M';
    next_value_map['M'] = 'N';
    next_value_map['N'] = 'O';
    next_value_map['O'] = 'M';

    start_text_map[6] = 'P';
    next_value_map['P'] = 'R';
    next_value_map['R'] = 'S';
    next_value_map['S'] = 'P';

    start_text_map[7] = 'T';
    next_value_map['T'] = 'U';
    next_value_map['U'] = 'V';
    next_value_map['V'] = 'T';

    start_text_map[8] = 'W';
    next_value_map['W'] = 'X';
    next_value_map['X'] = 'Y';
    next_value_map['Y'] = 'W';
}

void text_editor_process() {
    unsigned char *button_addr;
    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);
    text_editor_values *val;
    val = (text_editor_values *) shmat(value_mid, (text_editor_values *) NULL, 0);
    int i;

    if (button_addr[1] == True && button_addr[2] == True) {
        //초기화
        printf("reset text\n");
        val->string[0] = '\0';
        val->prev_value = -1;
        val->editing_index = 0;
    } else if (button_addr[4] == True && button_addr[5] == True){
        // 입력모드 변경
        // swap 0, 1
        printf("swap text mode\n");
        val->is_letter_mode = 1 - val->is_letter_mode;
        val->prev_value = -1;
    } else if (button_addr[7] == True && button_addr[8] == True){
        // 한칸 띄우기
        printf("text spacing\n");
        val->editing_index++;
        val->string[val->editing_index] = ' ';
        val->string[val->editing_index + 1] = '\0';

        val->prev_value = -1;
    } else{
        // 일반 입력
        for (i=0;i<MAX_BUTTON; i++)
        {
            if(button_addr[i] == True)
            {
                // 문자열 모드
                if (val->is_letter_mode == True){
                    printf("text %d pressed\n",i);
                    if(val->prev_value == i)
                    {
                        char now_editing_char = val->string[val->editing_index];
                        val->string[val->editing_index] = next_value_map[now_editing_char];
                        val->string[val->editing_index + 1] = '\0';
                    }
                    else{
                        val->editing_index ++;
                        val->string[val->editing_index] = start_text_map[i];
                        val->string[val->editing_index + 1] = '\0';
                    }
                    val->prev_value = i;
                }
                //숫자모드
                else{
                    printf("digit %d pressed\n",i);
                    val->prev_value = -1;
                    val->editing_index ++;
                    val->string[val->editing_index] = '0' + i + 1;
                    val->string[val->editing_index + 1] = '\0';
                }
            }
        }
    }

    // 입력 버튼만큼 카운트 증가
    for (i=0;i<MAX_BUTTON; i++)
    {
        val->count += button_addr[i];
    }

    // 입력 영역 초과시 한칸 앞으로
    if (strlen(val->string) > MAX_BUFF)
    {
        strcpy(val->string, val->string + 1);
        val->editing_index--;
    }
}

void draw_board_process() {

}

void set_fnd(int value) {
    unsigned char data[4] = {0,};

    data[0] = value / 1000 % 10;
    data[1] = value / 100 % 10;
    data[2] = value / 10 % 10;
    data[3] = value % 10;

    write(fpga_fnd_device, &data, 4);

    read(fpga_fnd_device, &data, 4);
}

int int_to_four_digit(int value, int exponent) {
    int result = 0;

    result += value / (exponent * exponent * exponent) % exponent * 1000;
    result += value / (exponent * exponent) % exponent * 100;
    result += value / exponent % exponent * 10;
    result += value % exponent;

    return result;
}

void set_led(unsigned char binary_data) {
    unsigned long *fpga_addr = 0;
    unsigned char *led_addr = 0;
    fpga_addr = (unsigned long *) mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fpga_led_mmap,
                                       FPGA_BASE_ADDRESS);
    if (fpga_addr == MAP_FAILED) {
        printf("mmap error!\n");
        close(fpga_led_mmap);
        return;
    }

    led_addr = (unsigned char *) ((void *) fpga_addr + LED_ADDR);

    *led_addr = binary_data;
}

void set_lcd_text(char * string)
{
    unsigned char buffer[MAX_BUFF+1];
    int len = strlen(string);
    memset(buffer, ' ',MAX_BUFF);
    buffer[MAX_BUFF] = '\0';

    if(len>0) {
        strncat(buffer,string,len);
        buffer[len] = ' ';
    }

    write(fpga_lcd_device, buffer, MAX_BUFF);
}

void clock_output() {
    clock_values *clockValues;
    clockValues = (clock_values *) shmat(value_mid, (clock_values *) NULL, 0);
    time_t now;
    now = time(NULL) + clockValues->bonus_time;
    int hour = now / 3600 % 24;
    int min = now / 60 % 60;

    set_fnd(hour * 100 + min);

    if (clockValues->editable == False) {
        set_led(0b10000000);
    } else {
        if (now % 2 == 1) {
            set_led(0b00100000);
        } else {
            set_led(0b00010000);
        }
    }
    set_lcd_text(" ");
    //shmdt(clockValues);
}

void counter_output() {
    counter_values *values;
    values = (counter_values *) shmat(value_mid, (counter_values *) NULL, 0);
    int exp = values->exponent;
    int val = values->value;

    char buffer[100];
    int fnd_val = 0;

    if (exp == 10) {
        set_led(0b01000000);
        set_fnd(val % 1000);
    } else if (exp == 2) {
        set_led(0b10000000);
        fnd_val = int_to_four_digit(val, 2);
        set_fnd(fnd_val);
    } else if (exp == 4){
        set_led(0b00010000);
        fnd_val = int_to_four_digit(val, 4);
        set_fnd(fnd_val);
    } else if (exp == 8){
        set_led(0b00100000);
        fnd_val = int_to_four_digit(val, 8);
        set_fnd(fnd_val);
    }
    set_lcd_text(" ");
    //shmdt(values);
}

void text_editor_output()
{
    char buffer[MAX_BUFF+1];
    text_editor_values *val;
    val = (text_editor_values *) shmat(value_mid, (text_editor_values *) NULL, 0);

    strcpy(buffer, val->string);

    set_lcd_text(buffer);
    set_fnd(val->count);
}



void draw_board_output()
{

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
    mode_addr = (int *) shmat(mode_mid, (int *) NULL, 0);
    mode_addr[0] = CLOCK_MODE + MODE_CHANGED;
    //shmdt(mode_addr);

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


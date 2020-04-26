#include "main.h"

void input_process()
{
    printf("init input process\n");
    // 디바이스 로드
    if ((fpga_switch_device = open("/dev/fpga_push_switch", O_RDWR | O_NONBLOCK)) == -1)
    {
        printf("Switch Device Open Error\n");
//        return;
    }
    if ((hw_button_device = open("/dev/input/event0", O_RDONLY | O_NONBLOCK)) == -1)
    {
        printf("Hardware key Device Open Error\n");
//        return;
    }
    while (1)
    {
        read_hw_key(mode_mid);
        read_fpga_key(button_mid);
        usleep(DELAY);
    }
}

void main_process()
{
    printf("init main process\n");
    int *mode_addr;

    // 텍스트 에디터용 설정 초기화
    text_editor_map_setting();

    while (1)
    {
        mode_addr = (int *) shmat(mode_mid, (int *) NULL, 0);
        // 종료 입력시 프로그램 종료
        if (mode_addr[0] == EXIT)
        {
            exit(0);
        }
        // 모드가 변경되었을 때를 감지하여 초기값을 설정한다.
        if (mode_addr[0] >= MODE_CHANGED)
        {
            mode_addr[0] -= MODE_CHANGED;
            printf("mode changed : %d\n", mode_addr[0]);
            reset_value(mode_addr[0]);
        }

        // 현재 모드에 따라 연산한다.
        switch (mode_addr[0])
        {
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

void output_process()
{
    printf("init output process\n");
    int *mode_addr;

    // 디바이스 로드
    if ((fpga_led_mmap = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
    {
        printf("led disabled\n");
        return;
    }

    if ((fpga_fnd_device = open("/dev/fpga_fnd", O_RDWR)) == -1)
    {
        printf("fnd disabled\n");
        return;
    }

    if ((fpga_lcd_device = open("/dev/fpga_text_lcd", O_WRONLY)) == -1)
    {
        printf("lcd disabled\n");
        return;
    }

    if ((fpga_dot_device = open("/dev/fpga_dot", O_WRONLY)) == -1)
    {
        printf("dot matrix disalbed\n");
        return;
    }


    while (1)
    {
        mode_addr = (int *) shmat(mode_mid, (int *) NULL, 0);
//        printf("output mode : %d",mode_addr[0]);

        // 현재 모드에 따라 출력한다.
        switch (mode_addr[0])
        {
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
        usleep(DELAY);
    }
}

void reset_value(int mode)
{
    // 모드가 변경되었을 때, 각 모드에서 사용하는 정보를 초기화한다.
    int i, j;
    unsigned char *value_addr = (unsigned char *) shmat(value_mid, (unsigned char *) NULL, 0);
    memset(value_addr, 0, 1000);

    clock_values *a;
    counter_values *b;
    text_editor_values *c;
    draw_board_values *d;
    switch (mode)
    {
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
            c->editing_index = -1;
            break;
        case DRAW_MODE:
            d = (draw_board_values *) value_addr;
            d->count = 0;
            d->cursor_point = 0;
            d->is_show_cursor = True;
            for (i = 0; i < BOARD_ROW; i++)
                for (j = 0; j < BOARD_COL; j++)
                    d->board[i * BOARD_COL + j] = 0;
            break;
    }
    //shmdt(value_addr);
}

void read_hw_key()
{
    char *device = "/dev/input/event0";
    int *mode;
    struct input_event ev[BUFF_SIZE];
    int rd, size = sizeof(struct input_event);
    mode = (int *) shmat(mode_mid, (int *) NULL, 0);

    if ((rd = read(hw_button_device, ev, size * BUFF_SIZE)) < size)
    {
        return;
    }

    // 키 입력에 따라 모드를 변경하고, 모드가 변경되었음을 알리는 MOD_CHANGED 를 더해준다.
    if (ev[0].type == 1 && ev[0].value == 1)
    {
        if (ev[0].code == BACK_KEY_CODE)
        {
            mode[0] = EXIT;
        }
        else if (ev[0].code == VOLUME_UP)
        {
            mode[0] = (mode[0] + 1) % MODE_COUNT + MODE_CHANGED;
        }
        else if (ev[0].code == VOLUME_DOWN)
        {
            mode[0] = (mode[0] + MODE_COUNT - 1) % MODE_COUNT + MODE_CHANGED;
        }
    }
#ifdef DEBUG
//    printf("mode is %d\n", mode[0]);
#endif
    //shmdt(mode);
}

void read_fpga_key()
{
    int i;
    int buff_size;
    unsigned char *button_addr;
    unsigned char buffer[MAX_BUTTON];

    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);

    // push button device 에서 현재 눌린 버튼을 읽어와 저장한다.
    read(fpga_switch_device, &buffer, sizeof(buffer));
    for (i = 0; i < MAX_BUTTON; i++)
    {
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

void clock_process()
{
    struct timeval timeval;
    unsigned char *button_addr;
    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);
    clock_values *clockValues;
    clockValues = (clock_values *) shmat(value_mid, (clock_values *) NULL, 0);

    if (button_addr[0] == 1)
    {
        // 시간 수정 모드에서 1번 입력시 현재 시간 값을 시스템시간으로 설정하고 초기화한다.
        if (clockValues->editable == True)
        {
            clockValues->editable = False;

            gettimeofday(&timeval, NULL);
            timeval.tv_sec += clockValues->bonus_time;
            settimeofday(&timeval, NULL);
            clockValues->bonus_time = 0;
        }
            // 수정 모드가 아닐 경우 수정모드로 진입한다.
        else
        {
            clockValues->editable = True;
        }
    }

    // 시간을 더해주는 연산을 bonus_time 이라는 변수에 추가시간을 저장하여 구현했다.
    if (button_addr[1] == True && clockValues->editable)
        clockValues->bonus_time = 0;

    if (button_addr[2] == True && clockValues->editable)
        clockValues->bonus_time += 3600;

    if (button_addr[3] == True && clockValues->editable)
        clockValues->bonus_time += 60;

    //shmdt(button_addr);
    //shmdt(clockValues);
}

// counter 모드에서 현재 진수의 다음 진수를 알기위한 배열, next_exp_map[2] 는 2진수 다음으로 설정될 진수를 나타낸다.
unsigned char next_exp_map[11] = {0, 0, 10, 0, 2, 0, 0, 0, 4, 0, 8};

void counter_process()
{
    unsigned char *button_addr;
    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);
    counter_values *counterValues;
    counterValues = (counter_values *) shmat(value_mid, (counter_values *) NULL, 0);

    if (button_addr[0] == True)
    {
        // 표기 진수 변경
        counterValues->exponent = next_exp_map[counterValues->exponent];
    }
    else if (button_addr[1] == True)
    {
        // 현재 표기의 백의 자리 증가
        counterValues->value += counterValues->exponent * counterValues->exponent;
    }
    else if (button_addr[2] == True)
    {
        // 현재 표기의 십의 자리 증가
        counterValues->value += counterValues->exponent;
    }
    else if (button_addr[3] == True)
    {
        counterValues->value += 1;
    }

    printf("main val, exp : %d %d\n", counterValues->value, counterValues->exponent);


    //shmdt(button_addr);
    //shmdt(counterValues);
}

void text_editor_map_setting()
{
    // 현재 입력중인 문자의 다음 값과 다른 버튼을 눌렀을 때 나올 첫 값에 대한 설정
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

void text_editor_process()
{
    unsigned char *button_addr;
    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);
    text_editor_values *val;
    val = (text_editor_values *) shmat(value_mid, (text_editor_values *) NULL, 0);
    int i;

    if (button_addr[1] == True && button_addr[2] == True)
    {
        //초기화
        printf("reset text\n");
        val->string[0] = '\0';
        val->prev_value = -1;
        val->editing_index = -1;
    }
    else if (button_addr[4] == True && button_addr[5] == True)
    {
        // 입력모드 변경
        printf("swap text mode\n");
        val->is_letter_mode = 1 - val->is_letter_mode;
        val->prev_value = -1;
    }
    else if (button_addr[7] == True && button_addr[8] == True)
    {
        // 한칸 띄우기
        printf("text spacing\n");
        val->editing_index++;
        val->string[val->editing_index] = ' ';
        val->string[val->editing_index + 1] = '\0';

        val->prev_value = -1;
    }
    else
    {
        // 일반 입력
        for (i = 0; i < MAX_BUTTON; i++)
        {
            if (button_addr[i] == True)
            {
                // 문자열 모드
                if (val->is_letter_mode == True)
                {
                    printf("text %d pressed\n", i);
                    if (val->prev_value == i)
                    {
                        // 이전에 눌렀던 스위치를 누를 경우 마지막 값을 변경
                        char now_editing_char = val->string[val->editing_index];
                        val->string[val->editing_index] = next_value_map[now_editing_char];
                        val->string[val->editing_index + 1] = '\0';
                    }
                    else
                    {
                        // 이전에 눌렀던 스위치와 다른 스위치 입력시 새로운 문자 추가
                        val->editing_index++;
                        val->string[val->editing_index] = start_text_map[i];
                        val->string[val->editing_index + 1] = '\0';
                    }
                    val->prev_value = i;
                }
                    //숫자모드
                else
                {
                    printf("digit %d pressed\n", i);
                    val->prev_value = -1;
                    val->editing_index++;
                    val->string[val->editing_index] = '0' + i + 1;
                    val->string[val->editing_index + 1] = '\0';
                }
            }
        }
    }

    // 입력 버튼만큼 카운트 증가
    for (i = 0; i < MAX_BUTTON; i++)
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

void draw_board_process()
{
    int i, j;
    int row, col;
    unsigned char *button_addr;
    button_addr = (unsigned char *) shmat(button_mid, (unsigned char *) NULL, 0);
    draw_board_values *val;
    val = (draw_board_values *) shmat(value_mid, (draw_board_values *) NULL, 0);

    // 입력 버튼만큼 카운트 증가
    for (i = 0; i < MAX_BUTTON; i++)
    {
        val->count += button_addr[i];
    }

    // 전부 초기화
    if (button_addr[0] == True)
    {
        memset(val->board, False, BOARD_ROW * BOARD_COL);
        val->count = 0;
        val->cursor_point = 0;
        val->is_show_cursor = True;
    }
        // 커서 깜박임 제어
    else if (button_addr[2] == True)
    {
        val->is_show_cursor = 1 - val->is_show_cursor;
    }
        // 그림만 초기화
    else if (button_addr[6] == True)
    {
        memset(val->board, False, BOARD_ROW * BOARD_COL);
    }
        // 그림 반전
    else if (button_addr[8] == True)
    {
        for (i = 0; i < BOARD_ROW; i++)
        {
            for (j = 0; j < BOARD_COL; ++j)
            {
                val->board[i * BOARD_COL + j] = 1 - val->board[i * BOARD_COL + j];
            }
        }
    }
        // 상하좌우 커서 이동
    else if (button_addr[1] == True)
    {
        row = val->cursor_point / BOARD_COL;
        if (row > 0)
            val->cursor_point -= BOARD_COL;
    }
    else if (button_addr[3] == True)
    {
        col = val->cursor_point % BOARD_COL;
        if (col > 0)
            val->cursor_point -= 1;
    }
    else if (button_addr[5] == True)
    {
        col = val->cursor_point % BOARD_COL;
        if (col < BOARD_COL - 1)
            val->cursor_point += 1;
    }
    else if (button_addr[7] == True)
    {
        row = val->cursor_point / BOARD_COL;
        if (col < BOARD_ROW - 1)
            val->cursor_point += BOARD_COL;
    }
        // 현재 커서위치의 값 변경
    else if (button_addr[4] == True)
    {
        val->board[val->cursor_point] = 1 - val->board[val->cursor_point];
    }
}

// fnd에 표기될 값을 4자리 숫자를 파라미터로 호출하여 설정하는 함수
void set_fnd(int value)
{
    unsigned char data[4] = {0,};

    data[0] = value / 1000 % 10;
    data[1] = value / 100 % 10;
    data[2] = value / 10 % 10;
    data[3] = value % 10;

    write(fpga_fnd_device, &data, 4);

    read(fpga_fnd_device, &data, 4);
}

// value를 4자리 exponent 진수 표현으로 변환하여 반환하는 함수
int int_to_four_digit(int value, int exponent)
{
    int result = 0;

    result += value / (exponent * exponent * exponent) % exponent * 1000;
    result += value / (exponent * exponent) % exponent * 100;
    result += value / exponent % exponent * 10;
    result += value % exponent;

    return result;
}

// 2진수 8자리 표현식으로 led 설정하는 함수
void set_led(unsigned char binary_data)
{
    unsigned long *fpga_addr = 0;
    unsigned char *led_addr = 0;
    fpga_addr = (unsigned long *) mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fpga_led_mmap,
                                       FPGA_BASE_ADDRESS);
    if (fpga_addr == MAP_FAILED)
    {
        printf("mmap error!\n");
        close(fpga_led_mmap);
        return;
    }

    led_addr = (unsigned char *) ((void *) fpga_addr + LED_ADDR);

    *led_addr = binary_data;
}

// lcd 에 입력 string 출력
void set_lcd_text(char *string)
{
    unsigned char buffer[MAX_BUFF + 1];
    int len = strlen(string);

    memset(buffer, ' ', MAX_BUFF + 1);

    if (len > 0)
    {
        strncpy(buffer, string, len);
    }

    write(fpga_lcd_device, buffer, MAX_BUFF);
}

// unsigned char[BOARD_ROW] 2진수 형식으로 dot matrix 설정
void set_dot_matrix(unsigned char *num_matrix)
{
    write(fpga_dot_device, num_matrix, BOARD_ROW);
}

void clock_output()
{
    clock_values *clockValues;
    clockValues = (clock_values *) shmat(value_mid, (clock_values *) NULL, 0);
    time_t now;

    // clock_process 에서 추가된 bonus_time 만큼 시간 증가
    now = time(NULL) + clockValues->bonus_time;
    int hour = now / 3600 % 24;
    int min = now / 60 % 60;

    set_fnd(hour * 100 + min);

    // led 설정
    if (clockValues->editable == False)
    {
        set_led(0b10000000);
    }
    else
    {
        // 1초마다 3,4번 led점멸
        if (now % 2 == 1)
        {
            set_led(0b00100000);
        }
        else
        {
            set_led(0b00010000);
        }
    }
    set_lcd_text(" ");
    unsigned char dot[10] = {0,};
    set_dot_matrix(empty_dot_matrix);
    //shmdt(clockValues);
}

void counter_output()
{
    counter_values *values;
    values = (counter_values *) shmat(value_mid, (counter_values *) NULL, 0);
    int exp = values->exponent;
    int val = values->value;

    int fnd_val = 0;

    // 진수에 따라 표기 변환 후 fnd에 표시
    if (exp == 10)
    {
        set_led(0b01000000);
        set_fnd(val % 1000);
    }
    else if (exp == 2)
    {
        set_led(0b10000000);
        fnd_val = int_to_four_digit(val, 2);
        set_fnd(fnd_val);
    }
    else if (exp == 4)
    {
        set_led(0b00010000);
        fnd_val = int_to_four_digit(val, 4);
        set_fnd(fnd_val);
    }
    else if (exp == 8)
    {
        set_led(0b00100000);
        fnd_val = int_to_four_digit(val, 8);
        set_fnd(fnd_val);
    }
    set_lcd_text(" ");
    set_dot_matrix(empty_dot_matrix);
    //shmdt(values);
}

void text_editor_output()
{
    char buffer[MAX_BUFF + 1];
    text_editor_values *val;

    val = (text_editor_values *) shmat(value_mid, (text_editor_values *) NULL, 0);

//    printf("text output string: %s len: %d editing: %d\n", val->string, strlen(val->string), val->editing_index);

    // text_editor_process에서 변경된 텍스트를 읽어와 lcd에 표시
    strcpy(buffer, val->string);

    set_lcd_text(buffer);
    set_fnd(val->count);

    // dot matrix에 입력모드 표시
    unsigned char num_matrix[10];
    if (val->is_letter_mode == True)
    {
        // A
        num_matrix[0] = 0b0011100;
        num_matrix[1] = 0b0110110;
        num_matrix[2] = 0b1100011;
        num_matrix[3] = 0b1100011;
        num_matrix[4] = 0b1100011;
        num_matrix[5] = 0b1111111;
        num_matrix[6] = 0b1111111;
        num_matrix[7] = 0b1100011;
        num_matrix[8] = 0b1100011;
        num_matrix[9] = 0b1100011;
    }
    else
    {
        // 1
        num_matrix[0] = 0b0001100;
        num_matrix[1] = 0b0011100;
        num_matrix[2] = 0b0001100;
        num_matrix[3] = 0b0001100;
        num_matrix[4] = 0b0001100;
        num_matrix[5] = 0b0001100;
        num_matrix[6] = 0b0001100;
        num_matrix[7] = 0b0001100;
        num_matrix[8] = 0b0111111;
        num_matrix[9] = 0b0111111;
    }
    set_dot_matrix(num_matrix);
}


void draw_board_output()
{
    draw_board_values *val;
    val = (draw_board_values *) shmat(value_mid, (draw_board_values *) NULL, 0);

    set_fnd(val->count);
    set_lcd_text(" ");

    // char[row*col] 로 저장된 정보를 unsigned char[row] 로 변환
    int i, j;
    unsigned char num_matrix[10];
    for (i = 0; i < BOARD_ROW; i++)
    {
        unsigned char temp = 0;
        for (j = 0; j < BOARD_COL; ++j)
        {
            temp += val->board[i * BOARD_COL + j] << (BOARD_COL - 1 - j);
        }
        num_matrix[i] = temp;
    }

    // 1초마다 커서 표시
    if (val->is_show_cursor == True)
    {
        int row, col;
        unsigned char bit;
        time_t now = time(NULL);

        row = val->cursor_point / BOARD_COL;
        col = val->cursor_point % BOARD_COL;
        bit = 1 << (BOARD_COL - 1 - col);
        // 커서 위치 led on
        if (now % 2)
        {
            num_matrix[row] |= bit;
        }
        else // 커서 위치 led off
        {
            num_matrix[row] &= (0b1111111 - bit);
        }
    }


    set_dot_matrix(num_matrix);
}

int main()
{
    // 프로세스간 공유할 버튼,모드,값에 대한 공유메모리 넉넉하게 할당.
    button_mid = shmget(IPC_PRIVATE, 50, IPC_CREAT | 0644);
    mode_mid = shmget(IPC_PRIVATE, 10, IPC_CREAT | 0644);
    value_mid = shmget(IPC_PRIVATE, 1000, IPC_CREAT | 0644);
    if (button_mid == -1 || mode_mid == -1 || value_mid == -1)
    {
        perror("shmget");
        exit(1);
    }

    // 시작 모드 설정
    int *mode_addr;
    mode_addr = (int *) shmat(mode_mid, (int *) NULL, 0);
    mode_addr[0] = CLOCK_MODE + MODE_CHANGED;
    //shmdt(mode_addr);

    int pid;
    switch ((pid = fork()))
    {
        case -1: //fail
            perror("fork");
            exit(1);
            break;
        case 0: // child
            input_process();
            break;
        default: //parent
            // 추후 종료를 위한 pid 저장
            input_process_pid = pid;
            switch ((pid = fork()))
            {
                case -1: //fail
                    perror("second fork");
                    exit(1);
                    break;
                case 0: // child
                    output_process();
                    break;
                default:
                    // 추후 종료를 위한 pid 저장
                    output_process_pid = pid;
                    main_process();
                    break;
            }
            break;
    }
}


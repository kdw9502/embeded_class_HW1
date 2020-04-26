//
// Created by kdw on 2020-04-24.
//

#ifndef EMBEDDED_MAIN_H
#define EMBEDDED_MAIN_H

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
#include <stdlib.h>

#include <time.h>

#include <sys/mman.h>
#include "main.h"
#include <string.h>

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

#define False 0
#define True 1

#define MODE_CHANGED 100

#define EXIT -1

#define FPGA_BASE_ADDRESS 0x08000000 //fpga_base address
#define LED_ADDR 0x16

#define MAX_BUFF 32

int button_mid, mode_mid, value_mid;

void input_process();

void main_process();

void output_process();

void read_hw_key();

void read_fpga_key();

int main();

void reset_value(int mode);

void clock_process();

typedef struct _clock_values {
    int time;
    int bonus_time;
    int editable;
} clock_values;


void counter_process();

typedef struct _counter_values {
    int exponent;
    int value;
} counter_values;

void text_editor_process();

typedef struct _text_editor_values {
    int count;
    char is_letter_mode;
    char string[100];
    char prev_value;
    int editing_index;
} text_editor_values;

void draw_board_process();

typedef struct _draw_board_values {
    int count;
    int cursor_point;
    char board[200];
} draw_board_values;

int int_to_four_digit(int value, int exponent);

void set_fnd(int value);

void clock_output();

void counter_output();

void text_editor_output();

void draw_board_output();

void text_editor_map_setting();
char next_value_map[130] = {0, };
char start_text_map[10] ={0,};

int fpga_switch_device;
int hw_button_device;
int fpga_fnd_device;
int fpga_led_mmap;
int fpga_lcd_device;
#define DEBUG
#endif //EMBEDDED_MAIN_H

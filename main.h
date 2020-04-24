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

#define MODE_CHANGED 10

int button_mid, mode_mid, value_mid;

void input_process();

void main_process();

void output_process(int button_mid, int mode_mid);

void read_hw_key();

void read_fpga_key();

int main();

void reset_value(int mode);

void clock();

typedef struct _clock_values {
    int time;
    int bonus_time;
}clock_values;


void counter();

typedef struct _counter_values {
    int exponent;
    int value;
}counter_values;

void text_editor();

typedef struct _text_editor_values {
    int count;
    char is_letter_mode;
    char *string;
    char prev_value;
    int length;
}text_editor_values;

void draw_board();

typedef struct _draw_board_values {
    int count;
    int cursor_point;
    char *board;
}draw_board_values;
#endif //EMBEDDED_MAIN_H

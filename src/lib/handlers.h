#ifndef LIB_HANDLERS_H
#define LIB_HANDLERS_H

#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#include "lib.h"

void print_weird();

void create_input_thread(ShellState *state);

char *join_input_thread();

/*
 *
 *
 */
typedef struct bgExecution {
    pid_t pgid;
    Vec* child_pids;
    void (*drop)(struct bgExecution*);
    bool (*clear_child)(struct bgExecution*, pid_t);
    void (*print)(struct bgExecution*);
    char* (*fmt)(struct bgExecution*);
} BgExecution;
BgExecution* new_bg_execution(pid_t, Vec*);
void bg_execution_drop(BgExecution*);
void bg_execution_print(BgExecution*);
bool bg_execution_clear_child(BgExecution*, pid_t);
char* bg_execution_fmt(BgExecution*);
void start_bg_execution();
void end_bg_execution();

/*
 * Signal Handlers
 */
void sig_int_handler(int signal);

void sig_chld_handler(int signal);

void sig_usr_handler(int signal);

/*
 * Commands handlers
 */
pid_t basic_cmd_handler(ShellState *state, ExecArgs *exec_args,
                        bool should_wait, bool *should_continue,
                        int *status_code, bool is_background);

void unknown_cmd_info(CallResult *res, bool *should_continue,
                      int *status_code);

void sequential_cmd_handler(ShellState *state, CallGroup *call_group,
                            bool *should_continue, int *status_code, bool is_background);

void piped_cmd_handler(ShellState *state, CallGroup *call_group,
                       bool *should_continue, int *status_code, bool is_background);

#endif
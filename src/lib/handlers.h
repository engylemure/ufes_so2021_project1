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

/**
 * BgExecution
 * description: struct to store and more easily
 * handle with the background execution of processes
 * we can use a vector of child_pids to control it's execution
 * or the child_amount with the pgid considering that all children
 * would be at the same session and process group
 */
typedef struct bgExecution {
    pid_t pgid;
    Vec* child_pids;
    unsigned int child_amount;
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
#define CMD_HND_OPT_FORK 1
#define CMD_HND_OPT_BG 2
#define CMD_HND_OPT_LEADER 4
pid_t basic_cmd_handler(ShellState *state, CallGroup *call_group,
                        bool *should_continue, int *status_code);
pid_t cmd_handler(ShellState *state, ExecArgs *exec_args, bool *should_continue, int *status_code, unsigned int opts);

void unknown_cmd_info(CallResult *res, bool *should_continue,
                      int *status_code);

void sequential_cmd_handler(ShellState *state, CallGroup *call_group,
                            bool *should_continue, int *status_code);
void piped_cmd_handler(ShellState *state, CallGroup *call_group,
                       bool *should_continue, int *status_code);

#endif
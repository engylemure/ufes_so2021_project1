#ifndef LIB_HANDLERS_H
#define LIB_HANDLERS_H

#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lib.h"
#include "util/string_util/string_util.h"

void print_weird();

void create_input_thread(ShellState *state);

CallArg *join_input_thread();

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
                        int *status_code);

void unknown_cmd_info(CallResult *res, bool *should_continue,
                      int *status_code);

void sequential_cmd_handler(ShellState *state, CallGroup *call_group,
                            bool *should_continue, int *status_code);

void parallel_cmd_handler(ShellState *state, CallGroup *call_group,
                          bool *should_continue, int *status_code);

void piped_cmd_handler(ShellState *state, CallGroup *call_group,
                       bool *should_continue, int *status_code);

#endif
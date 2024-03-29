#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#include "lib/handlers.h"
#include "lib/lib.h"

int main(void) {
    char *debug_env = getenv("DEBUG");
    debug_lib(debug_env != NULL &&
              (str_equals(debug_env, "true") || str_equals(debug_env, "1")));
    signal(SIGINT, sig_int_handler);
    signal(SIGQUIT, sig_int_handler);
    signal(SIGCHLD, sig_chld_handler);
    signal(SIGUSR1, sig_usr_handler);
    signal(SIGUSR2, sig_usr_handler);

    ShellState *state = initialize_shell_state();

    bool should_continue = true;
    int status_code = 0;
    while (should_continue) {
        create_input_thread(state);
        CallArg *call_arg = join_input_thread();
        if (call_arg != NULL) {
            CallGroups *call_groups = call_arg->call_groups(call_arg);
            int i;
            for (i = 0; i < call_groups->len; i++) {
                CallGroup *call_group = call_groups->groups[i];
                switch (call_group->type) {
                    case Basic:
                        if (call_group->exec_amount)
                            basic_cmd_handler(state, call_group->exec_arr[0], true,
                                              &should_continue, &status_code);
                        break;
                    case Parallel:
                        parallel_cmd_handler(state, call_group, &should_continue,
                                             &status_code);
                        break;
                    case Sequential:
                        sequential_cmd_handler(state, call_group, &should_continue,
                                               &status_code);
                        break;
                    case Piped:
                        piped_cmd_handler(state, call_group, &should_continue, &status_code);
                        break;
                    default:
                        break;
                }
            }
            call_groups->drop(call_groups);
            call_arg->drop(call_arg);
        }
    }
    state->drop(state);
    return status_code;
}
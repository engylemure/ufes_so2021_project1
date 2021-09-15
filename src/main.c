#include <signal.h>
#include <stdbool.h>
#include <stdio.h>

#include "lib/handlers.h"
#include "lib/lib.h"

/**
 * @brief our main
 * 
 * @return int 
 */
int main(void) {
    setsid();
    start_bg_execution();
    char *debug_env = getenv("DEBUG");
    debug_lib(debug_env != NULL &&
              (str_equals(debug_env, "true") || str_equals(debug_env, "1")));
    signal(SIGINT, sig_int_handler);
    signal(SIGQUIT, sig_int_handler);
    signal(SIGCHLD, sig_chld_handler);
    signal(SIGUSR1, sig_usr_handler);
    signal(SIGUSR2, sig_usr_handler);

    ShellState *state = new_shell_state();

    bool should_continue = true;
    int status_code = 0;
    while (should_continue) {
        create_input_thread(state);
        char *shell_input = join_input_thread();
        if (shell_input != NULL) {
            CallGroups *call_groups = call_groups_from_input(shell_input);
            int i;
            for (i = 0; i < call_groups->len; i++) {
                CallGroup *call_group = call_groups->groups[i];
                switch (call_group->type) {
                    case Basic:
                        basic_cmd_handler(state, call_group, &should_continue,
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
            call_groups_drop(call_groups);
            free(shell_input);
        }
    }
    state->drop(state);
    end_bg_execution();
    return status_code;
}
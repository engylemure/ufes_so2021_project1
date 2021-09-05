#include "handlers.h"

#define BACKGROUND_MACRO(is_background, ret_val) if (is_background) {\
pid_t child_pid = fork();\
if (child_pid == -1) {\
perror("fork failed!\n");\
exit(2);\
} else if (child_pid > 0) {\
return ret_val;\
} else {\
*should_continue = false;\
}\
}

int children_in_bg = 0;
pid_t child_pgid = 0;

void sig_chld_handler(const int signal) {
    pid_t child_that_finished = wait(NULL);
    if (child_that_finished != -1) {
        if (children_in_bg != 0) {
            printf("[%d] %d Done\n", children_in_bg, child_that_finished);
            children_in_bg -= 1;
        }
    }
}

void *input_thread_func(void *arg) {
    ShellState *state = arg;
    pthread_exit(prompt_user(arg));
}

pthread_t input_thread;
bool has_input_thread;

void create_input_thread(ShellState *state) {
    if (has_input_thread) {
        pthread_cancel(input_thread);
    }
    pthread_create(&input_thread, NULL, input_thread_func, (void *) state);
    has_input_thread = true;
}

CallArg *join_input_thread() {
    CallArg *val = NULL;
    pthread_join(input_thread, (void **) &val);
    if (val == PTHREAD_CANCELED) {
        printf("\n");
        val = NULL;
    }
    has_input_thread = false;
    return val;
}

void cancel_thread() {
    if (has_input_thread) {
        pthread_cancel(input_thread);
        pthread_detach(input_thread);
        has_input_thread = false;
    }
}

void sig_int_handler(const int signal) {
    if (child_pgid) {
        killpg(child_pgid, signal);
    }
    cancel_thread();
}

void sig_usr_handler(const int signal) {
    sig_int_handler(SIGINT);
    print_weird();
}

void unknown_cmd_info(CallResult *res, bool *should_continue,
                      int *status_code) {
    if (res->is_parent) {
        printf("Unknown command %s\n", res->additional_data);
    } else {
        *should_continue = false;
        *status_code = UnknownCommand;
    }
}

pid_t basic_cmd_handler(ShellState *state, ExecArgs *exec_args, bool should_wait,
                        bool *should_continue, int *status_code, bool is_background) {
    BACKGROUND_MACRO(is_background, 0);
    CallResult *res = exec_args->call(exec_args, true, should_wait);
    switch (res->status) {
        case Continue:
            break;
        case Exit:
            *should_continue = false;
            break;
        case Cd:
            state->change_dir(state, res->additional_data);
            break;
        case UnknownCommand:
            unknown_cmd_info(res, should_continue, status_code);
            break;
    }
    pid_t child_pid = res->child_pid;
    res->drop(res);
    return child_pid;
}

void sequential_cmd_handler(ShellState *state, CallGroup *call_group,
                            bool *should_continue, int *status_code, bool is_background) {
    BACKGROUND_MACRO(is_background, );
    int i;
    for (i = 0; i < call_group->exec_amount; i++) {
        basic_cmd_handler(state, call_group->exec_arr[i], true, should_continue,
                          status_code, false);
    }
}

void parallel_cmd_handler(ShellState *state, CallGroup *call_group,
                          bool *should_continue, int *status_code) {
    int exec_amount = call_group->exec_amount;
    pid_t child_pids[exec_amount];
    int i;
    for (i = 0; i < exec_amount; i++) {
        child_pids[i] = basic_cmd_handler(state, call_group->exec_arr[i], false,
                                          should_continue, status_code, false);
        if (i < exec_amount - 1) {
            children_in_bg += 1;
            printf("[%d] %d\n", children_in_bg, child_pids[i]);
        }
        setpgid(child_pids[i], child_pids[0]);
    }
    child_pgid = child_pids[0];
    if (call_group->exec_amount > 1) {
        pid_t child_to_wait = child_pids[exec_amount - 1];
        waitpid(child_to_wait, NULL, WUNTRACED);
    }
    child_pgid = 0;
}

void piped_cmd_handler(ShellState *state, CallGroup *call_group,
                       bool *should_continue, int *status_code, bool is_background) {
    BACKGROUND_MACRO(is_background, );
    int exec_amount = call_group->exec_amount;
    int i;
    pid_t child_pgid = 0;
    int pipes_len = exec_amount - 1;
    int pipes[pipes_len][2];
    for (i = 0; i < pipes_len; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe failed!\n");
            exit(1);
        }
    }
    for (i = 0; i < exec_amount; i++) {
        ExecArgs *exec_args = call_group->exec_arr[i];
        pid_t child_pid = fork();
        if (child_pid == -1) {
            perror("fork failed!\n");
            exit(1);
        }
        if (i == 0) {
            child_pgid = child_pid ? child_pid : getpid();
        }
        if (child_pid) {
            setpgid(child_pid, child_pgid);
        } else {
            if (i < exec_amount - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][0]);
            }
            int stdin_fileno_idx = i - 1;
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][1]);
            }
            for (i = 0; i < pipes_len; i++) {
                if (stdin_fileno_idx >= 0 && stdin_fileno_idx != i) {
                    close(pipes[i][1]);
                    close(pipes[i][0]);
                }
            }
            exec_args->call(exec_args, false, false);
            *should_continue = false;
            *status_code = UnknownCommand;
            break;
        }
    }
    if (i == exec_amount) {
        int j;
        // Closing opened and unused pipes from the parent
        for (j = 0; j < pipes_len; j++) {
            close(pipes[j][1]);
            close(pipes[j][0]);
        }
        while (waitpid(-child_pgid, NULL, WUNTRACED) != -1);
    }
}

const char *WEIRD = "\n                                        .--.  .--.\n"
                    "                                       /    \\/    \\\n"
                    "                                      | .-.  .-.   \\\n"
                    "                                      |/_  |/_  |   \\\n"
                    "                                      || `\\|| `\\|    `----.\n"
                    "                                      |\\0_/ \\0_/    --,    \\_\n"
                    "                    .--\"\"\"\"\"-.       /              (` \\     `-.\n"
                    "                   /          \\-----'-.              \\          \\\n"
                    "                   \\  () ()                         /`\\          \\\n"
                    "                   |                         .___.-'   |          \\\n"
                    "                   \\                        /` \\|      /           ;\n"
                    "                    `-.___             ___.' .-.`.---.|             \\\n"
                    "                       \\| ``-..___,.-'`\\| / /   /     |              `\\\n"
                    "                        `      \\|      ,`/ /   /   ,  /\n"
                    "                                `      |\\ /   /    |\\/\n"
                    "    I feel weird...                   ,   .'`-;   '     \\/\n"
                    "                            ,    |\\-'  .'   ,   .-'`\n"
                    "                          .-|\\--;`` .-'     |\\.'\n"
                    "                         ( `\"'-.|\\ (___,.--'`'   \n"
                    "                          `-.    `\"`          _.--'\n"
                    "                             `.          _.-'`-.\n"
                    "                               `''---''``        `.";

void print_weird() { printf("%s\n", WEIRD); }
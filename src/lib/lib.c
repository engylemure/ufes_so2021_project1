#include <dirent.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "lib.h"
#include "util/string_util/string_util.h"
#include "util/vec/vec.h"

/*
 * Ansi Colors for stdout color manipulation
 */
const char BlueAnsi[] = "\033[94m";
const char JojoAnsi[] = "\033[95m";
const char EndAnsi[] = "\033[0m";
const char RedAnsi[] = "\033[91m";
const char YellowAnsi[] = "\033[93m";

void todo(char *msg) { printf("%sTODO: %s%s\n", YellowAnsi, msg, EndAnsi); }

char *read_env(char *name) {
    char *env = malloc(sizeof(char) * BUFFER_MAX_SIZE);
    char *env_value;
    if (!(env_value = getenv(name))) {
        fprintf(stderr, "The environment variable '%s' is not available!\n", name);
        exit(1);
    }
    if (snprintf(env, BUFFER_MAX_SIZE, "%s", env_value) >= BUFFER_MAX_SIZE) {
        fprintf(stderr,
                "BUFFER_MAX_SIZE wasn't enough to hold the %s env value of size "
                "'%ld'\n",
                name, strlen(env_value));
        exit(1);
    }
    return env;
}

Vec *new_vec_string() { return new_vec(sizeof(char *)); }

bool DEBUG_IS_ON = false;

void debug_lib(bool should_debug) {
    if (should_debug) {
        DEBUG_IS_ON = true;
    }
}

ShellState *initialize_shell_state() {
    char *PWD = read_env("PWD");
    char *HOME = read_env("HOME");
    ShellState *state = malloc(sizeof(ShellState));
    state->pwd = PWD;
    state->home = HOME;
    state->pretty_pwd = pretty_pwd;
    state->drop = drop_shell_state;
    state->change_dir = shell_state_change_dir;
    return state;
}

void drop_shell_state(ShellState *self) {
    free(self->home);
    free(self->pwd);
    free(self);
}

char *pretty_pwd(ShellState *self) {
    if (!strcmp(self->pwd, self->home)) {
        return strdup("~/");
    }
    char *pwd = strstr(self->pwd, self->home);
    if (pwd != NULL) {
        pwd = pwd + strlen(self->home);
        char *prettied_pwd = malloc(sizeof(char) * (strlen(pwd) + 2));
        prettied_pwd[0] = '~';
        prettied_pwd[1] = '\0';
        strcat(prettied_pwd, pwd);
        return prettied_pwd;
    }
    return strdup(self->pwd);
}

void shell_state_change_dir(ShellState *self, char *new_dir) {
    DIR *dir;
    dir = opendir(new_dir);
    char *real_dir_path = malloc(sizeof(char) * (BUFFER_MAX_SIZE >> 1));
    if (dir != NULL && realpath(new_dir, real_dir_path)) {
        free(self->pwd);
        self->pwd = real_dir_path;
        setenv("PWD", real_dir_path, 1);
        chdir(real_dir_path);
    } else {
        printf("\"%s\" is a invalid directory\n", new_dir);
        free(real_dir_path);
    }
    closedir(dir);
}

CallArg *prompt_user(ShellState *state) {
    if (state != NULL) {
        char input[BUFFER_MAX_SIZE];
        fflush(stdout);
        char *pwd = state->pretty_pwd(state);
        printf("%s%s > %s%svsh%s%s > %s", BlueAnsi, pwd, EndAnsi, JojoAnsi, EndAnsi,
               BlueAnsi, EndAnsi);
        free(pwd);
        fflush(stdin);
        fgets(input, BUFFER_MAX_SIZE, stdin);
        input[strcspn(input, "\n")] = '\0';
        CallArg *call = initialize_call_arg(input);
        return call;
    } else {
        perror("provided ShellState is NULL\n");
        exit(1);
    }
}

CallArg *initialize_call_arg(char *arg) {
    CallArg *self = malloc(sizeof(CallArg));
    self->arg = strdup(arg);
    self->call_groups = call_groups;
    self->drop = drop_call_arg;
    return self;
}

void drop_call_arg(CallArg *self) {
    if (self != NULL) {
        free(self->arg);
        free(self);
    } else {
        perror("free on NULL CallArgs!\n");
        exit(1);
    }
}

ExecArgs *exec_args_from_vec_str(Vec *vec) {
    ExecArgs *self = malloc(sizeof(ExecArgs));
    self->drop = drop_exec_args;
    self->fmt = (char *(*)(struct execArgs *self)) fmt_exec_arg;
    self->call = basic_exec_args_call;
    self->argc = vec->length;
    self->argv = malloc(sizeof(char *) * (self->argc + 1));
    int i, j;
    for (i = 0, j = 0; i < self->argc; i++) {
        if (strlen(vec->get(vec, i))) {
            self->argv[j++] = vec->get(vec, i);
        }
    }
    self->argv[j] = NULL;
    self->argc = j;
    vec->drop(vec);
    return self;
}

void drop_exec_args(ExecArgs *self) {
    int i;
    for (i = 0; i < self->argc; i++) {
        free(self->argv[i]);
    }
    free(self->argv);
    free(self);
}

Vec *new_vec_exec_args() { return new_vec(sizeof(ExecArgs *)); }

CallResult *basic_exec_args_call(ExecArgs *exec_args, bool should_fork,
                                 bool should_wait) {
    enum CallStatus status = UnknownCommand;
    char *program_name = NULL;
    bool is_parent = true;
    pid_t child_pid = 0;
    if (exec_args->argc == 0) {
        status = Continue;
    } else {
        program_name = exec_args->argv[0];
        if (str_equals(program_name, "exit")) {
            printf("Vaccine Shell was exited!\n");
            status = Exit;
        } else if (str_equals(program_name, "cd")) {
            status = Cd;
        } else {
            child_pid = should_fork ? fork() : 0;
            if (child_pid == -1) {
                perror("We can't start a new program since 'fork' failed!\n");
                exit(1);
            } else if (child_pid) {
                status = Continue;
                if (should_wait) {
                    int wait_status;
                    waitpid(child_pid, &wait_status, WUNTRACED);
                    if (WIFEXITED(wait_status) &&
                        (WEXITSTATUS(wait_status) == UnknownCommand)) {
                        status = UnknownCommand;
                    }
                }
            } else {
                execvp(program_name, exec_args->argv);
                is_parent = false;
            }
        }
    }
    char *aux_str;
    switch (status) {
        case Cd: {
            if (exec_args->argc == 1 || str_equals(exec_args->argv[1], "~")) {
                char *home_env = getenv("HOME");
                aux_str = home_env != NULL ? strdup(home_env) : NULL;
            } else {
                aux_str = strdup(exec_args->argv[1]);
            }
        }
            break;
        case UnknownCommand:
            aux_str = strdup(program_name);
            break;
        default:
            aux_str = NULL;
    }
    return new_call_result(status, is_parent, aux_str, child_pid);
}

CallResult *new_call_result(enum CallStatus status, bool is_parent,
                            char *additional_data, pid_t child_pid) {
    CallResult *res = malloc(sizeof(CallResult));
    res->drop = drop_call_res;
    res->is_parent = is_parent;
    res->status = status;
    res->additional_data = additional_data;
    res->child_pid = child_pid;
    return res;
}

void drop_call_res(CallResult *self) {
    if (self->additional_data != NULL) {
        free(self->additional_data);
        self->additional_data = NULL;
    }
    free(self);
}

CallGroup *call_group_from_vec_exec_args(Vec *vec_exec_args,
                                         enum CallType type) {
    CallGroup *self = malloc(sizeof(CallGroup));
    unsigned int count = vec_exec_args->length;
    ExecArgs **exec_arr = malloc(sizeof(ExecArgs *) * count);
    int i;
    for (i = 0; i < count; i++) {
        exec_arr[i] = vec_exec_args->get(vec_exec_args, i);
    }
    vec_exec_args->drop(vec_exec_args);
    self->type = type;
    self->exec_amount = count;
    self->exec_arr = exec_arr;
    self->drop = drop_call_group;
    self->file_name = NULL;
    return self;
}

void drop_call_group(CallGroup *self) {
    int i;
    for (i = 0; i < self->exec_amount; i++) {
        self->exec_arr[i]->drop(self->exec_arr[i]);
    }
    free(self->exec_arr);
    self->exec_arr = NULL;
    if (self->file_name != NULL) {
        free(self->file_name);
        self->file_name = NULL;
    }
    free(self);
}

Vec *new_vec_call_group() { return new_vec(sizeof(CallGroup *)); }

void drop_call_groups(CallGroups *self) {
    int i;
    for (i = 0; i < self->len; i++) {
        CallGroup *call_group = self->groups[i];
        call_group->drop(call_group);
    }
    free(self->groups);
    free(self);
}

char *fmt_char(char data) {
    char *str = malloc(sizeof(char) * 2);
    str[0] = data;
    str[1] = '\0';
    return str;
}

void drop_parse_arg_res(ParseArgRes *self) {
    if (self->arg != NULL)
        free(self->arg);
    free(self);
}

char *parse_arg_res_take_arg(ParseArgRes *self) {
    char *arg = self->arg;
    self->arg = NULL;
    return arg;
}

ParseArgRes *new_parse_arg_res(char *str, enum ArgType type) {
    ParseArgRes *val = malloc(sizeof(ParseArgRes));
    val->arg = str;
    val->type = type;
    val->take_arg = parse_arg_res_take_arg;
    val->drop = drop_parse_arg_res;
    return val;
}

char *str_from_vec_char(Vec **vec, bool should_alloc) {
    char *str = malloc(sizeof(char) * ((*vec)->length + 1));
    int i;
    for (i = 0; i < (*vec)->length; i++) {
        uintptr_t int_val = (uintptr_t) (*vec)->get(*vec, i);
        str[i] = (char) int_val;
    }
    str[i] = '\0';
    (*vec)->drop(*vec);
    if (should_alloc) {
        *vec = new_vec(sizeof(char));
    }
    return str;
}

Vec *process_call_arg(CallArg *call_arg) {
    Vec *args = new_vec(sizeof(ParseArgRes *));
    Vec *char_buffer = new_vec(sizeof(char));
    bool has_error = false;
    enum ArgParseState arg_parse_state = Ignore;
    int str_len = strlen(call_arg->arg);
    int i = 0;
    char c;
    if (str_len > 0 && (c = call_arg->arg[0]) && (c == '|' || c == '&')) {
        has_error = true;
        i = str_len;
    }
    void (*push_in_buffer)(Vec *vec, char c) =
    (void (*)(Vec *, char)) char_buffer->push;
    for (; i < str_len; i++) {
        c = call_arg->arg[i];
        switch (c) {
            case ' ':
                if (arg_parse_state == Ignore) {
                    continue;
                } else if (arg_parse_state == LeftQuote) {
                    push_in_buffer(char_buffer, c);
                } else if (char_buffer->length) {
                    args->push(args, new_parse_arg_res(
                            str_from_vec_char(&char_buffer, true), Simple));
                    arg_parse_state = Ignore;
                };
                break;
            case '|': {
                args->push(args,
                           new_parse_arg_res(str_from_vec_char(&char_buffer, true), Bar));
                arg_parse_state = Ignore;
            }
                break;
            case '&': {
                enum ArgType type = At;
                if (i + 1 < str_len && call_arg->arg[i + 1] == '&') {
                    type = DoubleAt;
                    i += 1;
                }
                args->push(
                        args, new_parse_arg_res(str_from_vec_char(&char_buffer, true), type));
                arg_parse_state = Ignore;
            }
                break;
            case '"':
                if (char_buffer->length &&
                    (char) (uintptr_t) char_buffer->get(char_buffer, char_buffer->length - 1) == '\\') {
                    char_buffer->pop(char_buffer);
                    push_in_buffer(char_buffer, c);
                } else {
                    if (arg_parse_state == LeftQuote) {
                        args->push(args, new_parse_arg_res(
                                str_from_vec_char(&char_buffer, true), Quoted));
                        arg_parse_state = Ignore;
                    } else {
                        arg_parse_state = LeftQuote;
                    }
                }
                break;
            default:
                if (arg_parse_state == Ignore) {
                    arg_parse_state = Word;
                }
                push_in_buffer(char_buffer, c);
                break;
        }
    }
    if (char_buffer->length) {
        if (arg_parse_state == Word) {
            args->push(args, new_parse_arg_res(str_from_vec_char(&char_buffer, false),
                                               Simple));
        } else {
            char_buffer->drop(char_buffer);
        }
        if (arg_parse_state == LeftQuote) {
            has_error = true;
        }
    } else {
        char_buffer->drop(char_buffer);
    }
    if (DEBUG_IS_ON)
        args->print(args, fmt_parse_arg_res);
    if (has_error) {
        args->drop(args);
        return NULL;
    }
    return args;
}

CallGroups *new_call_groups(int call_group_count, bool has_parsing_error) {
    CallGroups *self = malloc(sizeof(CallGroups));
    self->drop = *drop_call_groups;
    self->groups = call_group_count > 0 || has_parsing_error
                   ? malloc(sizeof(CallGroups *) * call_group_count)
                   : NULL;
    self->len = has_parsing_error ? 0 : call_group_count;
    self->has_parsing_error = has_parsing_error;
    return self;
}

void call_group_specific_type(enum CallType expected_type, enum CallType *type,
                              Vec **vec_str, Vec *vec_call_group,
                              Vec **vec_exec_args) {
    ExecArgs *exec_arg = exec_args_from_vec_str(*vec_str);
    *vec_str = new_vec_string();
    if (*type == Basic || *type == expected_type) {
        *type = expected_type;
        (*vec_exec_args)->push(*vec_exec_args, exec_arg);
    } else {
        (*vec_exec_args)->push(*vec_exec_args, exec_arg);
        vec_call_group->push(vec_call_group,
                             call_group_from_vec_exec_args(*vec_exec_args, *type));
        *vec_exec_args = new_vec_exec_args();
        *type = Basic;
    }
}

CallGroups *call_groups(CallArg *call_arg) {
    Vec *args = process_call_arg(call_arg);
    if (args != NULL) {
        Vec *vec_call_group = new_vec_call_group();
        Vec *vec_exec_args = new_vec_exec_args();
        Vec *vec_string = new_vec_string();
        enum CallType type = Basic;
        int len = args->length;
        ParseArgRes **args_res = (ParseArgRes **) args->take_arr(args);
        int i;
        for (i = 0; i < len; i++) {
            ParseArgRes *parse_arg_res = args_res[i];
            char *str = parse_arg_res->take_arg(parse_arg_res);
            if (strlen(str) && str[0] == '$') {
                char *env_value = read_env(str + 1);
                free(str);
                str = env_value;
            }
            switch (parse_arg_res->type) {
                case Bar:
                    call_group_specific_type(Piped, &type, &vec_string, vec_call_group,
                                             &vec_exec_args);
                    free(str);
                    break;
                case At:
                    call_group_specific_type(Parallel, &type, &vec_string, vec_call_group,
                                             &vec_exec_args);
                    free(str);
                    break;
                case DoubleAt:
                    call_group_specific_type(Sequential, &type, &vec_string, vec_call_group,
                                             &vec_exec_args);
                    free(str);
                    break;
                default:
                    vec_string->push(vec_string, str);
                    break;
            }
            parse_arg_res->drop(parse_arg_res);
        }
        free(args_res);
        if (vec_string->length) {
            vec_exec_args->push(vec_exec_args, exec_args_from_vec_str(vec_string));
        } else {
            vec_string->drop(vec_string);
        }
        vec_call_group->push(vec_call_group,
                             call_group_from_vec_exec_args(vec_exec_args, type));
        if (DEBUG_IS_ON)
            vec_call_group->print(vec_call_group, fmt_call_group);
        int call_group_count = vec_call_group->length;
        CallGroups *val = new_call_groups(call_group_count, false);
        CallGroup *call_group;
        for (i = 0; i < vec_call_group->length; i++) {
            call_group = vec_call_group->get(vec_call_group, i);
            val->groups[i] = call_group;
        }
        vec_call_group->drop(vec_call_group);
        return val;
    } else {
        return new_call_groups(0, true);
    }
}
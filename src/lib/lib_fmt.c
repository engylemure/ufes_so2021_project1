#include "lib.h"
#include "util/string_util/string_util.h"
#include <string.h>
#include <stdlib.h>

/*
 * Implementation for formatting structs into a string to more easily debug these data.
 */

// char*
char *fmt_string(void *data) {
    char *str = (char *) data;
    int fmt_len = strlen(str) + 3;
    char *fmt = malloc(fmt_len);
    fmt[0] = '"';
    fmt[1] = '\0';
    strcat(fmt, str);
    fmt[fmt_len - 2] = '"';
    fmt[fmt_len - 1] = '\0';
    return fmt;
}

// CallGroup
char *fmt_call_group(void *data) {
    CallGroup *call_group = data;
    char *formatted_call_group = malloc(sizeof(char) * BUFFER_MAX_SIZE >> 1);
    formatted_call_group[0] = '[';
    formatted_call_group[1] = '\0';
    int i;
    for (i = 0; i < call_group->exec_amount; i++) {
        char *exec_arg = fmt_exec_arg(call_group->exec_arr[i]);
        if (i == 0) {
            strcat(formatted_call_group, exec_arg);
        } else {
            strcat(formatted_call_group, ",");
            strcat(formatted_call_group, exec_arg);
        }
        free(exec_arg);
    }
    strcat(formatted_call_group, "](");
    char *call_group_type;
    switch (call_group->type) {
        case Basic:
            call_group_type = "Basic";
            break;
        case Piped:
            call_group_type = "Piped";
            break;
        case RedirectStdIn:
            call_group_type = "RedirectStdin";
            break;
        case RedirectStdout:
            call_group_type = "RedirectStdout";
            break;
        case Sequential:
            call_group_type = "Sequential";
            break;
    }
    strcat(formatted_call_group, call_group_type);
    strcat(formatted_call_group, ")");
    char bg_info[15];
    sprintf(bg_info, "(bg:%s)", call_group->is_background ? "true":"false");
    strcat(formatted_call_group, bg_info);
    return formatted_call_group;
}

// ExecArgs
char *fmt_exec_arg(void *data) {
    ExecArgs *exec_args = data;
    int argv_len = 15;
    int i;
    for (i = 0; i < exec_args->argc; i++) {
        argv_len += strlen(exec_args->argv[i]) + 1;
    }
    char *argv = malloc(sizeof(char) * argv_len);
    argv[0] = '[';
    argv[1] = '\0';
    for (i = 0; i < exec_args->argc; i++) {
        char *formatted_call = fmt_string(exec_args->argv[i]);
        strcat(argv, formatted_call);
        free(formatted_call);
        if (i != exec_args->argc - 1)
            strcat(argv, ",");
    }
    strcat(argv, "]");
    return argv;
}


// ParseArgRes
char *fmt_parse_arg_res(void *data) {
    ParseArgRes *val = (ParseArgRes *) data;
    char *str = malloc(sizeof(char) * (strlen(val->arg) + 55));
    str[0] = '\0';
    strcat(str, "ParseArgRes { type: ");
    char *type;
    switch (val->type) {
        case Simple:
            type = "Simple";
            break;
        case Quoted:
            type = "Quoted";
            break;
        case Bar:
            type = "Bar";
            break;
        case At:
            type = "At";
            break;
        default:
            type = "DoubleAt";
    }
    strcat(str, type);
    strcat(str, ", arg: \"");
    strcat(str, val->arg);
    strcat(str, "\" }");
    return str;
}
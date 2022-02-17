#include "args.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void show_help_for_add(args_context_t* ctx, int parac, const char** parav) {
    printf("This is help for --add or build");
    exit(0);
}

void cb_process_positional(int index, const char* arg) {
    printf("opening file #%d:  %s\n", index, arg);
}

int cb_process_direcive_positional(int index, int argc, const char** argv) {
    if (index == 0 && !strcmp(*argv, "build")) {
        printf("args  \n");
        for (int i = 0; i < argc; i++) {
            printf("   [%d] %s\n", i, argv[i]);
        }
        args_context_t* ctx = init_args_context();
        argparse_add_parameter(ctx, "help", 'h', "Show help message", 0, 0, 0, show_help_for_add);
        argparse_set_positional_arg_process(ctx, cb_process_positional);
        argparse_set_directive_positional_arg_process(ctx, cb_process_direcive_positional);
        if (!parse_args(ctx, argc, argv)) {
            printf("usage: test build [--help]\n");
            printf("type `%s build --help` for more information\n", argv[0]);
            deinit_args_context(ctx);
            exit(1);
        }
        deinit_args_context(ctx);

        return 1;
    }
    return 0;
}
void cb_process_add(args_context_t* parent_ctx, int parac, const char** parav) {
    printf("args  \n");
    for (int i = 0; i < parac; i++) {
        printf("   [%d] %s\n", i, parav[i]);
    }
    args_context_t* ctx = init_args_context();
    argparse_add_parameter(ctx, "help", 'h', "Show help message", 0, 0, 0, show_help_for_add);
    argparse_set_positional_arg_process(ctx, cb_process_positional);
    argparse_set_directive_positional_arg_process(ctx, cb_process_direcive_positional);
    if (!parse_args(ctx, parac, parav)) {
        argparse_print_usage(ctx, "test build");
        printf("type `%s --addg --help` for more information\n", parav[0]);
        deinit_args_context(ctx);
        exit(1);
    }
    deinit_args_context(ctx);
}

void cb_process_encoding(int parac, const char** parav) {
    printf("set encoding to %s (-E)\n", parav[0]);
}

void show_help(args_context_t* ctx, int parac, const char** parav) {
    argparse_print_help_usage(ctx, "test", NULL, NULL);
    exit(0);
}

args_context_t* ctx;
void argument_parse(int argc, const char** argv) {
    ctx = init_args_context();
    argparse_add_parameter(ctx, "read", 'r', "Read", 0, 0, 0, NULL);
    argparse_add_parameter(ctx, "write", 'w', "Write", 0, 0, 0, NULL);
    argparse_add_parameter(ctx, "binary", 'b', "Binary", 0, 0, 0, NULL);
    argparse_add_parameter(ctx, "save", 's', "ave, or save to another file(s)", 0, 100, 0, NULL);
    argparse_set_parameter_name(ctx, "FILES...");
    argparse_add_help_parameter(ctx, "test", NULL);
    argparse_add_parameter(ctx, "verbose", 'v', "Use this flag to set verbose level", 0, 0, 0, NULL);
    argparse_add_parameter(ctx, "version", 0, "Version", 0, 0, 0, NULL);
    argparse_add_parameter(ctx, NULL, 'E', "Set encoding", 1, 1, 0, NULL);
    argparse_set_parameter_name(ctx, "ENCODING");
    argparse_set_error_message(ctx, "an encoding must be provided");
    argparse_add_parameter_directive(ctx, "add", 'a', "Add", 0, cb_process_add);
    argparse_set_parameter_name(ctx, "NAME");
    argpaese_add_short_leading_parameter(ctx, 'D', "define a variable (use as -Dname=value)", 0, NULL);
    argparse_set_parameter_name(ctx, "NAME=VALUE");
    argparse_set_positional_arg_process(ctx, cb_process_positional);
    argparse_set_directive_positional_arg_process(ctx, cb_process_direcive_positional);
    argparse_set_positional_args(ctx, 1, 100);
    argparse_enable_remove_ambiguous(ctx);
    argparse_sort_parameters(ctx);
    argparse_set_positional_arg_name(ctx, "FILE", "File to open");
    if (!parse_args(ctx, argc, argv)) {
        argparse_print_usage(ctx, "test");
        printf("type `%s --help` for more information\n", argv[0]);
        deinit_args_context(ctx);
        exit(1);
    }
    parse_result_t* r = argparse_get_last_parse_result(ctx);
    printf("count of --read, -r:    %d\n", argparse_count(r, "read"));
    printf("count of --read, -r:    %d\n", argparse_count(r, "r"));
    printf("count of --write, -w:   %d\n", argparse_count(r, "w"));
    printf("verbose level:          %d\n", argparse_count(r, "v"));
    parsed_argument_t arg_save;
    argparse_get_parsed_arg(r, "save", &arg_save);
    printf("count of --save, -s:    %d\n", arg_save.count);
    printf("   -- Parameter count:  %d\n", arg_save.parac);
    for (int i=0; i<arg_save.parac; i++) {
        printf("   -- [%d]:    %s\n", i, arg_save.parav[i]);
    }
    argparse_get_parsed_arg(r, "D", &arg_save);
    printf("count of -D:            %d\n", arg_save.count);
    printf("   -- Parameter count:  %d\n", arg_save.parac);
    for (int i=0; i<arg_save.parac; i++) {
        printf("   -- [%d]:    %s\n", i, arg_save.parav[i]);
    }
    argparse_parse_result_deinit(r);
}

int main(int argc, const char** argv) {
//    __acane__argparse_unit_test();
    argument_parse(argc, argv);
    deinit_args_context(ctx);
}
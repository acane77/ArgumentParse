#include "args.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

args_context_t* ctx;

void cb_process_read(int parac, const char** parav) {
    printf("open as read (--read)\n");
}
void cb_process_write(int parac, const char** parav) {
    printf("open as writable (--write)\n");
}
void cb_process_binary(int parac, const char** parav) {
    printf("open as binary (--binary)\n");
}
void cb_process_save(int parac, const char** parav) {
    printf("save to this file (--save)\n");
    if (parac > 0) {
        printf("save to the following files  \n");
        for (int i = 0; i < parac; i++) {
            printf("   [%d] %s\n", i, parav[i]);
        }
    }
}

int verbose_level = 0;
void cb_process_verbose(int parac, const char** parav) {
    printf("set to higher verbose level (--verbose)\n");
    verbose_level++;
}
void cb_process_version(int parac, const char** parav) {
    printf("version 1.3.14\n");
    exit(0);
}
void show_help_for_add(int parac, const char** parav) {
    printf("This is help for --add or build");
    exit(0);
}
int cb_error_report(const char* __msg) {
    printf("error: %s\n", __msg);
    return 0;
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
        argparse_set_error_handle(ctx, cb_error_report);
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
void cb_process_add(int parac, const char** parav) {
    printf("args  \n");
    for (int i = 0; i < parac; i++) {
        printf("   [%d] %s\n", i, parav[i]);
    }
    args_context_t* ctx = init_args_context();
    argparse_add_parameter(ctx, "help", 'h', "Show help message", 0, 0, 0, show_help_for_add);
    argparse_set_error_handle(ctx, cb_error_report);
    argparse_set_positional_arg_process(ctx, cb_process_positional);
    argparse_set_directive_positional_arg_process(ctx, cb_process_direcive_positional);
    if (!parse_args(ctx, parac, parav)) {
        argparse_print_usage(ctx, "test build");
        printf("type `%s build --help` for more information\n", parav[0]);
        deinit_args_context(ctx);
        exit(1);
    }
    deinit_args_context(ctx);
}

void cb_process_encoding(int parac, const char** parav) {
    printf("set encoding to %s (-E)\n", parav[0]);
}

void show_help(int parac, const char** parav) {
    argparse_print_help_usage(ctx, "test", NULL, NULL);
    exit(0);
}

void argument_parse(int argc, const char** argv) {
    ctx = init_args_context();
    argparse_add_parameter(ctx, "read", 'r', "Read", 0, 0, 0, cb_process_read);
    argparse_add_parameter(ctx, "write", 'w', "Write", 0, 0, 0, cb_process_write);
    argparse_add_parameter(ctx, "binary", 'b', "Binary", 0, 0, 0, cb_process_binary);
    argparse_add_parameter(ctx, "save", 's', "ave, or save to another file(s)", 0, 100, 0, cb_process_save);
    argparse_set_parameter_name(ctx, "FILES...");
    argparse_add_parameter(ctx, "help", 'h', "Show help message", 0, 0, 0, show_help);
    argparse_add_parameter(ctx, "verbose", 'v', "Use this flag to set verbose level", 0, 0, 0, cb_process_verbose);
    argparse_add_parameter(ctx, "version", 0, "Version", 0, 0, 0, cb_process_version);
    argparse_add_parameter(ctx, NULL, 'E', "Set encoding", 1, 1, 0, cb_process_encoding);
    argparse_set_parameter_name(ctx, "ENCODING");
    argparse_add_parameter_directive(ctx, "add", 'a', "Add", 0, cb_process_add);
    argparse_set_parameter_name(ctx, "NAME");
    argparse_set_error_handle(ctx, cb_error_report);
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
}

int main(int argc, const char** argv) {
//    __acane__argparse_unit_test();
    argument_parse(argc, argv);
    if (verbose_level != 0) {
        printf("set verbose level finally to %d\n", verbose_level);
    }
    deinit_args_context(ctx);
}
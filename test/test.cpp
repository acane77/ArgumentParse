#include "args.h"
#include <stdlib.h>
#include <stdio.h>

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
void cb_process_encoding(int parac, const char** parav) {
    printf("set encoding to %s (-E)\n", parav[0]);
}
void show_help(int parac, const char** parav) {
    printf(R"help(usage: test [OPTIONS...] FILES...

Positional:
   FILES                List of files to open

Options:
   -r, --read           Read
   -w, --write          Write
   -b, --binary         Binary
   -s, --save [FILE...] Save, or save to another file(s)
   -h, --help           Show help message
   -v, --verbose        Use this flag to set verbose level, use different times
                        to set different verbose levels
       --version        Show version info
   -E ENCODING          Set file encoding
)help");
    exit(0);
}

int cb_error_report(const char* __msg) {
    printf("error: %s\n", __msg);
    return 0;
}

void cb_process_positional(int index, const char* arg) {
    printf("opening file #%d:  %s\n", index, arg);
}


void argument_parse(int argc, const char** argv) {
    args_context_t* ctx = init_args_context();
    argparse_add_parameter(ctx, "read", 'r', "Read", 0, 0, cb_process_read);
    argparse_add_parameter(ctx, "write", 'w', "Write", 0, 0, cb_process_write);
    argparse_add_parameter(ctx, "binary", 'b', "Binary", 0, 0, cb_process_binary);
    argparse_add_parameter(ctx, "save", 's', "ave, or save to another file(s)", 0, 100, cb_process_save);
    argparse_add_parameter(ctx, "help", 'h', "Show help message", 0, 0, show_help);
    argparse_add_parameter(ctx, "verbose", 'v', "Use this flag to set verbose level", 0, 0, cb_process_verbose);
    argparse_add_parameter(ctx, "version", 0, "Version", 0, 0, cb_process_version);
    argparse_add_parameter(ctx, NULL, 'E', "Set encoding", 1, 1, cb_process_encoding);
    argparse_set_error_handle(ctx, cb_error_report);
    argparse_set_positional_arg_process(ctx, cb_process_positional);
    argparse_set_positional_args(ctx, 1, 100);
    argparse_enable_remove_ambiguous(ctx);
    if (!parse_args(ctx, argc, argv)) {
        printf("usage: test [OPTIONS...] FILES...\n");
        printf("type `%s --help` for more information\n", argv[0]);
        deinit_args_context(ctx);
        exit(1);
    }
    deinit_args_context(ctx);
}

int main(int argc, const char** argv) {
    //__acane__argparse_unit_test();
    argument_parse(argc, argv);
    if (verbose_level != 0) {
        printf("set verbose level finally to %d\n", verbose_level);
    }
}
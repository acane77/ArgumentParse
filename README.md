# 命令行参数解析

## 功能

* Unix风格的命令行基本参数解析
    * 长格式的解析(e.g., `--flag`)支持缩略使用(e.g., `--f`, `--fl`, `--fla`)
    * 短格式的解析(e.g., `-f`)
    * 带参数的和可选参数的解析
    * 位置参数的解析
    * 实现了去二义性参数(即 `--`)
* 实现上下文相关的命令参数组（类似`git add [OPTIONS]`和`git commit [OPTIONS]`）
* 生成帮助信息(--help)和使用方法(usage)

## TODO

* 使用队列执行命令规定的参数（待实现）

## 使用方法

* 将include目录中的头文件加入项目文件中。
* 编译src目录中的args.c或直接使用源码依赖。

## 示例1：完全实用回调函数

* 完整示例代码：<a href="test/test.cpp">test/test.cpp</a>

```c
#include "args.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

args_context_t* ctx;

/* 
    Other functions, see test/test.cpp 
    ...........
    ...........
*/

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

/* 
    Other functions, see test/test.cpp 
    ...........
    ...........
*/

void show_help(int parac, const char** parav) {
    argparse_print_help_usage(ctx, "test", NULL, NULL);
    exit(0);
}

void argument_parse(int argc, const char** argv) {
    ctx = init_args_context();
    argparse_add_parameter(ctx, "read", 'r', "Read", 0, 0, 0, cb_process_read);
    argparse_add_parameter(ctx, "write", 'w', "Write", 0, 0, 0, cb_process_write);
    argparse_add_parameter(ctx, "binary", 'b', "Binary", 0, 0, 0, cb_process_binary);
    argparse_add_parameter(ctx, "save", 's', "Save, or save to another file(s)", 0, 100, 0, cb_process_save);
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
    argument_parse(argc, argv);
    if (verbose_level != 0) {
        printf("set verbose level finally to %d\n", verbose_level);
    }
    deinit_args_context(ctx);
}
```

样例输出

```
$ ./test_argparse -vv --verb --save 1.txt 4.txt -- 5.txt -brw 6.txt 7.txt
set to higher verbose level (--verbose)
set to higher verbose level (--verbose)
set to higher verbose level (--verbose)
save to the following files  (--save)
   [0] 1.txt
   [1] 4.txt
opening file #0:  5.txt
open as binary (--binary)
open as read (--read)
open as writable (--write)
opening file #1:  6.txt
opening file #2:  7.txt
set verbose level finally to 3
```

```
$ ./test_argparse --hel
usage: test [-bhrvw] [-E ENCODING] [-a [NAME]] [-s [FILES...]] [--version] 
            FILE ...

Positional Arguments:
  FILE                   File to open 

Arguments:
  -E ENCODING            Set encoding 
  -a, --add [NAME]       Add 
  -b, --binary           Binary 
  -h, --help             Show help message 
  -r, --read             Read 
  -s, --save [FILES...]  Save, or save to another file(s) 
  -v, --verbose          Use this flag to set verbose level 
      --version          Version 
  -w, --write            Write g
```

```
$ ./test_argparse build --help
args  
   [0] build
   [1] --help
This is help for --add or build
```

## 示例2：使用内置的参数记录

* 使用`parsed_argument_t`来获知参数出现的次数以及对应的参数的数量。而不是完全使用回调函数。

```c
#include "args.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

args_context_t* ctx;

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
    argparse_add_parameter(ctx, "read", 'r', "Read", 0, 0, 0, NULL);
    argparse_add_parameter(ctx, "write", 'w', "Write", 0, 0, 0, NULL);
    argparse_add_parameter(ctx, "binary", 'b', "Binary", 0, 0, 0, NULL);
    argparse_add_parameter(ctx, "save", 's', "ave, or save to another file(s)", 0, 100, 0, NULL);
    argparse_set_parameter_name(ctx, "FILES...");
    argparse_add_parameter(ctx, "help", 'h', "Show help message", 0, 0, 0, show_help);
    argparse_add_parameter(ctx, "verbose", 'v', "Use this flag to set verbose level", 0, 0, 0, NULL);
    argparse_add_parameter(ctx, "version", 0, "Version", 0, 0, 0, NULL);
    argparse_add_parameter(ctx, NULL, 'E', "Set encoding", 1, 1, 0, NULL);
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
    argparse_parse_result_deinit(r);
}

int main(int argc, const char** argv) {
//    __acane__argparse_unit_test();
    argument_parse(argc, argv);
    deinit_args_context(ctx);
}
```

* 样例输出

```
$ ./test_argparse -vv --verb --save 1.txt 4.txt -- 5.txt -brw 6.txt 7.txt --save 1.txt -rr
opening file #0:  5.txt
opening file #1:  6.txt
opening file #2:  7.txt
count of --read, -r:    3
count of --read, -r:    3
count of --write, -w:   1
verbose level:          3
count of --save, -s:    2
   -- Parameter count:  3
   -- [0]:    1.txt
   -- [1]:    4.txt
   -- [2]:    1.txt
```
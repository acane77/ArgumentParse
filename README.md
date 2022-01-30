# 命令行参数解析

## 功能

* Unix风格的命令行基本参数解析
    * 长格式的解析(e.g., `--flag`)支持缩略使用(e.g., `--f`, `--fl`, `--fla`)
    * 短格式的解析(e.g., `-f`)
    * 带参数的和可选参数的解析
    * 位置参数的解析
    * 实现了去二义性参数(即 `--`)
* 实现上下文相关的命令参数组（类似`git add [OPTIONS]`和`git commit [OPTIONS]`的区别）
* 自动生成帮助信息（待实现）
* 使用事件队列执行命令规定的参数（待实现）

## 使用方法

* 将include目录中的头文件加入项目文件中。
* 编译src目录中的args.c或直接使用源码依赖。

## 示例

```c
#include "args.h"
#include <stdlib.h>
#include <stdio.h>

args_context_t* ctx;

// 不同的参数的处理函数
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
    // 定义命令行参数
    argparse_add_parameter(ctx, "read", 'r', "Read", 0, 0, cb_process_read);
    argparse_add_parameter(ctx, "write", 'w', "Write", 0, 0, cb_process_write);
    argparse_add_parameter(ctx, "binary", 'b', "Binary", 0, 0, cb_process_binary);
    argparse_add_parameter(ctx, "save", 's', "ave, or save to another file(s)", 0, 100, cb_process_save);
    argparse_add_parameter(ctx, "help", 'h', "Show help message", 0, 0, show_help);
    argparse_add_parameter(ctx, "verbose", 'v', "Use this flag to set verbose level", 0, 0, cb_process_verbose);
    argparse_add_parameter(ctx, "version", 0, "Version", 0, 0, cb_process_version);
    argparse_add_parameter(ctx, NULL, 'E', "Set encoding", 1, 1, cb_process_encoding);
    // 设置回调
    argparse_set_error_handle(ctx, cb_error_report);
    argparse_set_positional_arg_process(ctx, cb_process_positional);
    argparse_set_positional_args(ctx, 1, 100);
    // 启用去二义性的标志 (--)
    argparse_enable_remove_ambiguous(ctx);
    // 解析参数
    if (!parse_args(ctx, argc, argv)) {
        printf("usage: test [OPTIONS...] FILES...\n");
        printf("type `%s --help` for more information\n", argv[0]);
        deinit_args_context(ctx);
        exit(1);
    }
    deinit_args_context(ctx);
}

int main(int argc, const char** argv) {
    argument_parse(argc, argv);
    if (verbose_level != 0) {
        printf("set verbose level finally to %d\n", verbose_level);
    }
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
usage: test [OPTIONS...] FILES...

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
```
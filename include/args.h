/*! \file args.h */

/**
MIT License

Copyright (c) 2021 Acane (Zhixun Liu)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 * */

#ifndef _ACANE_ARGS_C_
#define _ACANE_ARGS_C_

#include <stdio.h>

struct args_context;
typedef struct args_context args_context_t;

#ifdef __cplusplus
extern "C" {
#endif

#define PARAMETER_NO_ARGS             0
#define PARAMETER_ARGS_COUNT_NO_LIMIT 32767
#define PARAMETER_NO_SHORT_TERM       0
#define PARAMETER_NO_LONG_TERM        NULL

/// Create and initialize an argument context
/// \return pointer to context
args_context_t* init_args_context();

/// Set count of positional arguments
/// \param ctx  pointer to context
/// \param minc min count of positional arguments
/// \param maxc max count of positional arguments
/// \return OK or FAIL
int argparse_set_positional_args(args_context_t* ctx, int minc, int maxc);

/// Set name of positional names (one by one)
/// First call sets the first positional arg, second call sets the second one, so does more.
/// \param ctx    pointer to context
/// \param name   name of positional args
/// \param description  description of this arguement, if no description, pass NULL instead
/// \return
int argparse_set_positional_arg_name(args_context_t* ctx, const char* name, const char* description);

/// De-initialize context
/// \param ctx  pointer to context
void deinit_args_context(args_context_t* ctx);

/// Add parameter meta information
/// \param ctx         pointer to context
/// \param long_term   long term of the argument (e.g., --flag)
/// \param short_term  short term of the argument (e.g., -f)
/// \param description description
/// \param minc        min argument count
/// \param maxc        min argument count
/// \param required     if the parameter is quired
/// \param process     callback to process function
/// \return OK or FAIL
int argparse_add_parameter(args_context_t* ctx, const char* long_term, char short_term,
                   const char* description, int minc, int maxc, int required,
                   void (*process)(int parac, const char** parav));

/// Add parameter meta information (thread safe)
/// \param ctx         pointer to context
/// \param long_term   long term of the argument (e.g., --flag)
/// \param short_term  short term of the argument (e.g., -f)
/// \param description description
/// \param minc        min argument count
/// \param maxc        min argument count
/// \param required    if the parameter is quired
/// \param arg_name    name of argument
/// \param process     callback to process function
/// \return OK or FAIL
int argparse_add_parameter_with_args(args_context_t* ctx, const char* long_term, char short_term,
                   const char* description, int minc, int maxc, int required, const char* arg_name,
                   void (*process)(int parac, const char** parav));

/// Set arg name for last added parameter (need to ensure thread safe by user)
/// \param ctx       pointer to context
/// \param arg_name  name of argument
/// \return
int argparse_set_parameter_name(args_context_t* ctx, const char* arg_name);

/// Add parameter meta information (with only long term, without parameter)
/// \param ctx         pointer to context
/// \param long_term   long term of the argument (e.g., --flag)
/// \param description description
/// \param required    if the parameter is required
/// \param process     callback to process function
/// \return OK or FAIL
int argparse_add_parameter_long_term(args_context_t* ctx, const char* long_term,
                   const char* description, int required,
                   void (*process)(int parac, const char** parav));

/// Add parameter meta information (with only long term)
/// \param ctx         pointer to context
/// \param long_term   long term of the argument (e.g., --flag)
/// \param description description
/// \param minc        min argument count
/// \param maxc        min argument count
/// \param required    if the parameter is required
/// \param process     callback to process function
/// \return OK or FAIL
int argparse_add_parameter_long_term_with_args(args_context_t* ctx, const char* long_term,
                    const char* description, int minc, int maxc, int required,
                    void (*process)(int parac, const char** parav));

/// Add parameter meta information (with only short term)
/// \param ctx         pointer to context
/// \param short_term  short term of the argument (e.g., -f)
/// \param description description
/// \param required    if the parameter is required
/// \param process     callback to process function
/// \return OK or FAIL
int argparse_add_parameter_short_term(args_context_t* ctx, char short_term,
                    const char* description, int required,
                    void (*process)(int parac, const char** parav));

/// Add parameter meta information (with only short term)
/// \param ctx         pointer to context
/// \param short_term  short term of the argument (e.g., -f)
/// \param description description
/// \param minc        min argument count
/// \param maxc        min argument count
/// \param required    if the parameter is required
/// \param process     callback to process function (parac: count of args, parav: args for the flags)
/// \return OK or FAIL
int argparse_add_parameter_short_term_with_args(args_context_t* ctx, char short_term,
                    const char* description, int minc, int maxc, int required,
                    void (*process)(int parac, const char** parav));

/// Add directive meta information
/// \param ctx          pointer to context
/// \param long_term    long term of the argument (e.g., --flag)
/// \param short_term   short term of the argument (e.g., -f)
/// \param description  description
/// \param required     if the parameter is required
/// \param process      callback to process function (this function returns 1 if is processed by directive)
/// \return OK or FAIL
int argparse_add_parameter_directive(args_context_t* ctx, const char* long_term, char short_term,
                                     const char* description,  int required, void (*process)(int parac, const char** parav));

/// Enable remove ambiguous (--) flags
/// \param ctx   pointer to context
void argparse_enable_remove_ambiguous(args_context_t* ctx);

/// Set callback to handle error messages
/// \param ctx   pointer to context
/// \param hnd   function pointer (__msg: error message)
/// \return OK or FAIL
int argparse_set_error_handle(args_context_t* ctx, int (*hnd)(const char* __msg));

/// Set callback to handle positional args
/// \param ctx       pointer to context
/// \param process   callback to process function (index: index of global positional arg, the arg)
/// \return OK or FAIL
int argparse_set_positional_arg_process(args_context_t* ctx, void (*process)(int index, const char* arg));

/// Set callback to handle directive positional args
/// \param ctx       pointer to context
/// \param process   callback to process function (this function returns 1 if is processed by directive)
/// \return
int argparse_set_directive_positional_arg_process(args_context_t* ctx, int (*process)(int index, int argc, const char** argv));

/// Parse arguments
/// \param ctx   pointer to context
/// \param argc  argc of main()
/// \param argv  argv of main()
/// \return OK or FAIL
int parse_args(args_context_t* ctx, int argc, const char** argv);

/// Set help message display width
///
///                           |<---          width       --->|
/// -h, --help                Show help message
/// \param ctx   pointer to context
/// \param width width of help message
/// \return OK or FAIL
int argparse_print_set_help_msg_width(args_context_t* ctx, int width);

/// Set leading spaces for help message items
///
/// |<--- leading spaces --->|
/// -h, --help                Show help message
/// \param ctx   pointer to context
/// \param width leading spaces of help message item
/// \return
int argparse_print_set_help_msg_leading_spaces(args_context_t* ctx, int width);

/// Sort parameters
/// \param ctx   pointer to context
/// \return OK or FAIL
int argparse_sort_parameters(args_context_t* ctx);

/// Print help message for registered parameters
/// \param ctx   pointer to context
/// \return OK or FAIL
int argparse_print_help(args_context_t* ctx);

/// Print help message for registered positional parameters
/// \param ctx   pointer to context
/// \return OK or FAIL
int argparse_print_help_for_positional(args_context_t* ctx);

/// Print usage message for registered parameters
/// \param ctx           pointer to context
/// \param program_name  program name shown after 'usage:'
/// \return
int argparse_print_usage(args_context_t* ctx, const char* program_name);

/// Print help and usage message for registered parameters
/// \param ctx           pointer to context
/// \param program_name  program name shown after 'usage:'
/// \param title_for_position title for positional args section, pass NULL to use default value (i.e., Positional Arguments)
/// \param title_for_args title for args section, pass NULL to use default value (i.e., Arguments)
/// \return
int argparse_print_help_usage(args_context_t* ctx, const char* program_name, const char* title_for_position, const char* title_for_args) ;

/// Set file to output help and usage
/// \param ctx   pointer to context
/// \param file  pointer to FILE object (e.g., stdout, stderr)
/// \return
int argparse_set_print_file(args_context_t* ctx, FILE* file);

/// Unit test
/// \return OK or FAIL
int __acane__argparse_unit_test();

#ifdef __cplusplus
}
#endif

#endif //_ACANE_ARGS_C_
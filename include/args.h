#ifndef _ACANE_ARGS_C_
#define _ACANE_ARGS_C_

struct args_context;
typedef struct args_context args_context_t;

#ifdef __cplusplus
extern "C" {
#endif

#define PARAMETER_NO_ARGS             0
#define PARAMETER_ARGS_COUNT_NO_LIMIT (1 << 16 - 1)
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
/// \param process     callback to process function
/// \return OK or FAIL
int argparse_add_parameter(args_context_t* ctx, const char* long_term, char short_term,
                   const char* description, int minc, int maxc,
                   void (*process)(int parac, const char** parav));

/// Add parameter meta information
/// \param ctx         pointer to context
/// \param long_term   long term of the argument (e.g., --flag)
/// \param short_term  short term of the argument (e.g., -f)
/// \param description description
/// \param minc        min argument count
/// \param maxc        min argument count
/// \param process     callback to process function
/// \return OK or FAIL
int argparse_add_parameter_with_args(args_context_t* ctx, const char* long_term, char short_term,
                   const char* description, int minc, int maxc,
                   void (*process)(int parac, const char** parav));

/// Add parameter meta information (with only long term, without parameter)
/// \param ctx         pointer to context
/// \param long_term   long term of the argument (e.g., --flag)
/// \param description description
/// \param process     callback to process function
/// \return OK or FAIL
int argparse_add_parameter_long_term(args_context_t* ctx, const char* long_term,
                   const char* description,
                   void (*process)(int parac, const char** parav));

/// Add parameter meta information (with only long term)
/// \param ctx         pointer to context
/// \param long_term   long term of the argument (e.g., --flag)
/// \param description description
/// \param minc        min argument count
/// \param maxc        min argument count
/// \param process     callback to process function
/// \return OK or FAIL
int argparse_add_parameter_long_term_with_args(args_context_t* ctx, const char* long_term,
                    const char* description, int minc, int maxc,
                    void (*process)(int parac, const char** parav));

/// Add parameter meta information (with only short term)
/// \param ctx         pointer to context
/// \param short_term  short term of the argument (e.g., -f)
/// \param description description
/// \param process     callback to process function
/// \return OK or FAIL
int argparse_add_parameter_short_term(args_context_t* ctx, char short_term,
                    const char* description,
                    void (*process)(int parac, const char** parav));

/// Add parameter meta information (with only short term)
/// \param ctx         pointer to context
/// \param short_term  short term of the argument (e.g., -f)
/// \param description description
/// \param minc        min argument count
/// \param maxc        min argument count
/// \param process     callback to process function (parac: count of args, parav: args for the flags)
/// \return OK or FAIL
int argparse_add_parameter_short_term_with_args(args_context_t* ctx, char short_term,
                    const char* description, int minc, int maxc,
                    void (*process)(int parac, const char** parav));

/// Add directive meta information
/// \param ctx          pointer to context
/// \param long_term    long term of the argument (e.g., --flag)
/// \param short_term   short term of the argument (e.g., -f)
/// \param description  description
/// \param process      callback to process function (this function returns 1 if is processed by directive)
/// \return OK or FAIL
int argparse_add_parameter_directive(args_context_t* ctx, const char* long_term, char short_term,
                                     const char* description, void (*process)(int parac, const char** parav));

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

int argparse_print_set_help_msg_width(args_context_t* ctx, int width);

int argparse_print_help(args_context_t* ctx);

/// Unit test
/// \return OK or FAIL
int __acane__argparse_unit_test();

#ifdef __cplusplus
}
#endif

#endif //_ACANE_ARGS_C_
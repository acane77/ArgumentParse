#include "args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define OK     1
#define FAIL   0
#define DEBUG
#ifdef DEBUG
#define LOG(fmt, ...) printf("[%s:%d][ INFO  ] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#define LOG(fmt, ...)
#endif // DEBUG
#define LOGE(fmt, ...) printf("[%s:%d][ ERROR ] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);

/* Node */
struct valarray;
typedef struct valarray valarray_t;

#define ACANE_SIGN 0x77061584

/* Arguments info */
typedef struct arg_info {
	const char* long_term;
	const char* description;
	int         short_term;
	int         min_parameter_count;
	int         max_parameter_count;
    int         directive_flag;   // treat all flags after into parameter of this flag
	void        (*process)(int parac, const char** parav); // no free parav!

    int         required;
    int         _requirement_satisfied_sign;
} arg_info_t;

/* graph nodes */
typedef struct arg_ctx_node {
	char        ch;     
	valarray_t* children;
	arg_info_t* arg_info;
	int         _arg_sign;
} ctx_node_t;

/* args graph-based metadata */
typedef struct arg_ctx_graph {
	ctx_node_t* head;
} ctx_graph_t;

typedef void* val_array_element_t;
#define VALARRAY_CAPACITY_PER_INCREASE 10

/* Variable length array for nodes */
struct valarray {
	val_array_element_t* data;
	size_t   capacity;
	size_t   size;
};

int valarray_extend_capacity(valarray_t* arr) {
	assert(arr && "arr is not initialized");
	val_array_element_t* _old = arr->data;
    val_array_element_t* _new = (val_array_element_t*)realloc((void*)_old, sizeof(val_array_element_t) * (arr->capacity + VALARRAY_CAPACITY_PER_INCREASE));
	if (!_new) {
		LOGE("allocate memory for capacity %lu failed", arr->capacity + 10);
		return FAIL;
	}
	arr->capacity += VALARRAY_CAPACITY_PER_INCREASE;
	arr->data = _new;
	return OK;
}

int valarray_init(valarray_t** arr) {
	*arr = (valarray_t*)malloc(sizeof(valarray_t));
	if (!*arr) {
		LOGE("allocate memory failed");
		return FAIL;
	}
	(*arr)->capacity = 0;
	(*arr)->size = 0;
	(*arr)->data = NULL;
	return valarray_extend_capacity(*arr);
}

int valarray_push_back(valarray_t* arr, val_array_element_t el) {
	if (arr->size + 1 > arr->capacity) {
		int ret = valarray_extend_capacity(arr);
		if (ret != OK) {
			return ret;
		}
	}
	arr->data[arr->size++] = el;
	return OK;
}

val_array_element_t* valarray_get(valarray_t* arr, size_t __i) {
	assert(arr && "arr is not initialized");
	assert(arr->data && "arr->data is not initialized");
	if (arr->size <= __i) {
		LOGE("index out of range, with [size=%zu, i=%zu]", arr->size, __i);
		return NULL;
	}
	return &arr->data[__i];
}

size_t valarray_index_of(valarray_t* arr, val_array_element_t* x, int (*cmp)(val_array_element_t* __lhs, val_array_element_t* __rhs)) {
	for (int i = 0; i < arr->size; ++i)
		if (!cmp(&arr->data[i], x))
			return i;
	return -1;
}

size_t valarray_el_index_of(valarray_t* arr, int ch) {
	for (int i = 0; i < arr->size; ++i)
		if (((ctx_node_t*)(arr->data[i]))->ch == ch)
			return i;
	return -1;
}

// ==============================================================================

ctx_node_t* ctx_node_init(int ch) {
	LOG("construct node with [ch=%c (%d)]", ch, ch);
	ctx_node_t* __n = (ctx_node_t*)malloc(sizeof(ctx_node_t));
	if (!__n)
		return NULL;
	__n->ch = ch;
	int __r = valarray_init(&__n->children);
	if (__r != OK) {
		LOG("init children list failed");
		free(__n);
		return NULL;
	}
    __n->arg_info = NULL;
	return __n;
}

ctx_node_t* ctx_graph_add_nodes(ctx_graph_t* __g, const char* str) {
	ctx_node_t* node = __g->head;
	for (; *str; ++str) {
		int index = valarray_el_index_of(node->children, *str);
		if (index >= 0) {
			assert(index < node->children->size);
			node = *valarray_get(node->children, index);
		}
		else {
			ctx_node_t* new_node = ctx_node_init(*str);
			valarray_push_back(node->children, new_node);
			node = new_node;
		}
	}
	// returns the final node
	return node;
}

ctx_graph_t* ctx_graph_init() {
	ctx_graph_t* __g = (ctx_graph_t*)malloc(sizeof(ctx_graph_t));
	if (!__g) {
		return NULL;
	}
	__g->head = ctx_node_init(0);
	return __g;
}

void ctx_graph_node_free(ctx_node_t* __n) {
    if (!__n)
        return;
    for (int i=0; i<__n->children->size; i++) {
        ctx_graph_node_free(*valarray_get(__n->children, i));
    }
    free(__n);
}

void ctx_graph_free(ctx_graph_t* __g) {
    if (__g) {
        ctx_graph_node_free(__g->head);
        free(__g);
    }
}

// =================================================================================

#define _DEFAULT_HELP_LINE_WIDTH  (50)

struct args_context {
	ctx_graph_t* ctx_graph;
	int (*error_handle)(const char* __msg);
	int current_addi_arg_count;
	arg_info_t* current_arg;
    int remove_ambiguous;

    // process positional args
    void (*process_positional)(int index, const char* arg);
    int (*process_directive_positional)(int index, int argc, const char** argv); // returns 1 if processed directive
    int positional_minc;
    int positional_maxc;

    // help message
    int help_line_width;
    int help_leading_spaces;
    FILE* output_file;

    // all arguments
    valarray_t* args;
};

args_context_t* init_args_context() {
	args_context_t* ctx = (args_context_t*)malloc(sizeof(args_context_t));
	if (!ctx) return NULL;
	ctx->ctx_graph = ctx_graph_init();
	ctx->current_addi_arg_count = 0;
	ctx->current_arg = NULL;
    ctx->error_handle = NULL;
    ctx->process_positional = NULL;
    ctx->process_directive_positional = NULL;
    ctx->positional_maxc = 0;
    ctx->positional_minc = 0;
    ctx->remove_ambiguous = 0;
    ctx->help_line_width = _DEFAULT_HELP_LINE_WIDTH;
    ctx->help_leading_spaces = 25;
    ctx->output_file = stdout;
    valarray_init(&ctx->args);
	return ctx;
}

void deinit_args_context(args_context_t* ctx) {
    if (!ctx) return;
	if (ctx->ctx_graph)
        ctx_graph_free(ctx->ctx_graph);
    if (ctx->args && ctx->args->data)
        free(ctx->args->data);
	free(ctx);
}

int argparse_set_positional_args(args_context_t* ctx, int minc, int maxc) {
    if (!ctx)
        return FAIL;
    ctx->positional_minc = minc;
    ctx->positional_maxc = maxc;
    return OK;
}

int regsiter_parameter_on_graph(args_context_t* ctx, const char* param,
		const char* description, int minc, int maxc, int required,
		void (*process)(int parac, const char** parav), int is_long_term, int is_directive,
        arg_info_t** arginfo) {
	ctx_node_t* final_node = ctx_graph_add_nodes(ctx->ctx_graph, param);
	if (!final_node) {
		LOGE("add node for --%s failed", param);
		return FAIL;
	}
	if (final_node->_arg_sign == ACANE_SIGN) {
		LOGE("parameter %s  already registered", param);
		return FAIL;
	}
	final_node->_arg_sign = ACANE_SIGN;
    if (arginfo && *arginfo)
        // already as a corresponding arg_info
        final_node->arg_info = *arginfo;
    else
        final_node->arg_info = (arg_info_t*) malloc(sizeof(arg_info_t));
	if (is_long_term)
		final_node->arg_info->long_term = param;
	else
		final_node->arg_info->short_term = param[0];
    if (arginfo && *arginfo) {
        LOG("arginfo already registered, skip");
        return OK;
    }
    final_node->arg_info->description = description;
	final_node->arg_info->max_parameter_count = maxc;
	final_node->arg_info->min_parameter_count = minc;
	final_node->arg_info->process = process;
    final_node->arg_info->directive_flag = is_directive;
    final_node->arg_info->required = required;
    if (arginfo)
        *arginfo = final_node->arg_info;
	return OK;
}

int add_parameter_with_args_(args_context_t* ctx, const char* long_term, char short_term,
                            const char* description, int minc, int maxc, int required,
                            void (*process)(int parac, const char** parav),
                            int is_directive) {
    if (!ctx) return FAIL;
    if (!long_term && !short_term) {
        LOGE("at least one of long_term and short_term should be provided");
        return FAIL;
    }
    arg_info_t* arginfo = NULL;
    // register short term
    if (short_term) {
        char __short_term[2] = { 0, 0 };
        __short_term[0] = short_term;
        int ret = regsiter_parameter_on_graph(ctx, __short_term, description, minc, maxc, required, process, 0, is_directive, &arginfo);
        if (ret != OK)
            return ret;
    }
    // register long term
    if (long_term) {
        int ret = regsiter_parameter_on_graph(ctx, long_term, description, minc, maxc, required, process, 1, is_directive, &arginfo);
        if (ret != OK)
            return ret;
    }
    // register parameter on context
    if (arginfo) {
        if (short_term) arginfo->short_term = short_term;
        valarray_push_back(ctx->args, (void *) arginfo);
        LOG("add arg_info to args [args.size=%zu]", ctx->args->size);
    }
    return OK;
}

int add_parameter_with_args(args_context_t* ctx, const char* long_term, char short_term,
				  const char* description, int minc, int maxc, int required,
				  void (*process)(int parac, const char** parav)) {
	return add_parameter_with_args_(ctx, long_term, short_term, description, minc, maxc, required, process, 0);
}

int argparse_add_parameter_directive(args_context_t* ctx, const char* long_term, char short_term,
                                     const char* description,  int required, void (*process)(int parac, const char** parav)) {
    return add_parameter_with_args_(ctx, long_term, short_term, description, 0, PARAMETER_ARGS_COUNT_NO_LIMIT, required,
                                    process, 1);
}

int argparse_add_parameter(args_context_t* ctx, const char* long_term, char short_term,
	const char* description, int minc, int maxc, int required,
	void (*process)(int parac, const char** parav)) {
	return add_parameter_with_args(ctx, long_term, short_term,
		description, minc, maxc, required, process);
}

int argparse_add_parameter_long_term(args_context_t* ctx, const char* long_term,
	const char* description, int required,
	void (*process)(int parac, const char** parav)) {
	return add_parameter_with_args(ctx, long_term, PARAMETER_NO_SHORT_TERM,
		description, PARAMETER_NO_ARGS, PARAMETER_NO_ARGS, required, process);
}

int argparse_add_parameter_long_term_with_args(args_context_t* ctx, const char* long_term,
	const char* description, int minc, int maxc, int required,
	void (*process)(int parac, const char** parav)) {
	return add_parameter_with_args(ctx, long_term, PARAMETER_NO_SHORT_TERM,
		description, minc, maxc, required, process);
}

int argparse_add_parameter_short_term(args_context_t* ctx, char short_term,
	const char* description, int required,
	void (*process)(int parac, const char** parav)) {
	return add_parameter_with_args(ctx, PARAMETER_NO_LONG_TERM, short_term,
		description, PARAMETER_NO_ARGS, PARAMETER_NO_ARGS, required, process);
}

int argparse_add_parameter_short_term_with_args(args_context_t* ctx, char short_term,
	const char* description, int minc, int maxc, int required,
	void (*process)(int parac, const char** parav)) {
	return add_parameter_with_args(ctx, PARAMETER_NO_LONG_TERM, short_term,
		description, minc, maxc, required, process);
}

int argparse_set_error_handle(args_context_t* ctx, int (*hnd)(const char* __msg)) {
	if (!ctx) return FAIL;
	ctx->error_handle = hnd;
    return OK;
}

#define PARSEARG_REPORT_ERROR(msg, ...)   do { \
    char error_msg_buf[256];                   \
    sprintf(error_msg_buf, msg, ##__VA_ARGS__);\
    if (ctx->error_handle) {\
        ctx->error_handle(error_msg_buf);                                \
        return FAIL;\
    }\
} while (0)

arg_info_t* get_parameter_from_graph(args_context_t* ctx, const char* arg) {
    const char * __a = arg;
    if (!ctx) return NULL;
    ctx_node_t* node = ctx->ctx_graph->head;
    for (; *arg; ++arg) {
        int index = valarray_el_index_of(node->children, *arg);
        if (index >= 0) {
            assert(index < node->children->size);
            node = *valarray_get(node->children, index);
        }
        else {
            // no such parameter
            return NULL;
        }
    }
    // If this flag can become an end
    if (node->arg_info) {
        return node->arg_info;
    }
    // returns the final node
    // if the node is not final node
    if (node->children->size// if has children node
        && arg[1] // if is not short term
        ) {
        // try to find the final node
        while (1) {
            if (node->children->size == 0) {
                // reach the final node
                break;
            }
            else if (node->children->size == 1) {
                // walk to the next node
                node = *valarray_get(node->children, 0);
            }
            else {
                // has one more children, report error for ambiguous flags
                PARSEARG_REPORT_ERROR("--%s is ambiguous", __a);
                return NULL;
            }
        }
    }

    if (node->_arg_sign == ACANE_SIGN) {
        node->arg_info->_requirement_satisfied_sign = ACANE_SIGN;
        return node->arg_info;
    }
    // no arg bound to this node
    return NULL;
}

const char* arg_info_to_string(arg_info_t* arg) {
    static char short_term[2] = {0, 0};
    if (arg->long_term)
        return arg->long_term;
    if (arg->short_term) {
        short_term[0] = arg->short_term;
        return short_term;
    }
    return NULL;
}



void process_if_no_args(args_context_t* ctx) {
    // process if no need args
    if (ctx->current_arg->max_parameter_count == 0) {
        LOG("   ctx->current_arg->max_parameter_count=%d", ctx->current_arg->max_parameter_count);
        LOG("do process for flag (no args):  %s", arg_info_to_string(ctx->current_arg));
        if (ctx->current_arg->process)
            ctx->current_arg->process(0, NULL);
        ctx->current_arg = NULL;
    }
}

#define CHECK_ADDITIONAL_ARGS() do { \
    /* Check if last argument need additional argument */ \
    if (ctx->current_arg    /* has arg parsed */ \
        && ctx->current_arg->max_parameter_count != 0  /* has additional arg */ \
        && ctx->current_addi_arg_count < ctx->current_arg->min_parameter_count /* not all args present */ \
    ) {\
        PARSEARG_REPORT_ERROR("at least %d additional arguments should provided for --%s",\
                              ctx->current_arg->min_parameter_count, arg_info_to_string(ctx->current_arg));\
    }                                    \
} while (0)

#define GET_PARAMETER_FROM_GRAPH_AND_CHECK(_arg) do {\
    arg_info_t* argi = get_parameter_from_graph(ctx, _arg);\
    if (!argi) {\
        PARSEARG_REPORT_ERROR("unknown option --%s", _arg);\
    } \
    ctx->current_arg = argi;\
    /* process if no need args */\
    process_if_no_args(ctx);                         \
    /* process directive, return directly */\
    /* for example,   git add [-A "xxxxxx"]  */\
    if (ctx->current_arg && ctx->current_arg->directive_flag) {\
        ctx->current_arg->process(argc - __last_arg_idx, argv + __last_arg_idx);\
        return OK;\
    }                                                     \
} while (0)

int parse_args(args_context_t* ctx, int argc, const char** argv) {
    int __last_arg_idx = 0;
    int __global_positonal_argc = 0;
    if (argc <= 1)
        goto final_check;
	for (int i = 1; i <= argc; i++) {
		const char* arg = argv[i];
        if (!arg) continue;
        LOG("--> %s", arg);
		// if is flag
		if (arg[0] == '-') {
            LOG("   * is a flag (- or --)");

            // try process optional with no args
            if (ctx->current_arg && ctx->current_arg->min_parameter_count == 0 && ctx->current_arg->max_parameter_count) {
                LOG("do process for flag:  %s", arg_info_to_string(ctx->current_arg));
                if (ctx->current_arg->process)
                    ctx->current_arg->process(0, NULL);
                ctx->current_arg = NULL;
            }

            // record current index
            __last_arg_idx = i;

			// if is --flag flag
			if (arg[1] == '-') {
                CHECK_ADDITIONAL_ARGS();
			    const char* long_term = arg + 2; // ignore --
                // the flag is -- and enable remove_ambiguous
                if (!*long_term && ctx->remove_ambiguous) {
                    ctx->current_arg = NULL;
//                    // if last parameter requires args
//                    if (ctx->current_arg && ctx->current_arg->min_parameter_count > 0) {
//                        LOG("do process for flag at -- X:  %s", arg_info_to_string(ctx->current_arg));
//                        if (ctx->current_arg->process)
//                            ctx->current_arg->process(i - __last_arg_idx, argv + __last_arg_idx + 1);
//                    }
                    continue;
                }
                LOG("process --%s", long_term);
                GET_PARAMETER_FROM_GRAPH_AND_CHECK(long_term);
			}
			// if is -f flag
			else {
                // process combined multiply parameters: -abcd
                while (*(++arg)) {
                    // check addtional args for every parameter
                    CHECK_ADDITIONAL_ARGS();

                    char __s[2] = {0, 0};  __s[0] = *arg;
                    LOG("process -%s", __s);
                    GET_PARAMETER_FROM_GRAPH_AND_CHECK(__s);

                }
			}

		}
        // if is parameters
        else {
            LOG("   * is a pos arg");
            // if is global positional args
            // i.e.,  no current args present
            if (!ctx->current_arg) {
                LOG("global positional arg: %s", arg);
                if (ctx->positional_maxc < __global_positonal_argc + 1) {
                    PARSEARG_REPORT_ERROR("unknown positional arg: %s", arg);
                }
                if (ctx->process_directive_positional) {
                    // process as `git commit [-m "sadsadsa"]`
                    if (ctx->process_directive_positional(__global_positonal_argc, argc - i, argv + i)) {
                        return OK;
                    }
                }
                if (ctx->process_positional)
                    ctx->process_positional(__global_positonal_argc, arg);
                __global_positonal_argc++;
            }
            // if is args for certain parameters
            else {
                LOG("positional arg for [%s]: %s", arg_info_to_string(ctx->current_arg), arg);

                // If current parameter has args, and no args anymore, do process
                if ((i + 1 < argc && argv[i+1][0] == '-'   // next arg is a flag
                    && ctx->current_arg && ctx->current_arg->max_parameter_count != 0)   // this parameter requires args
                    || (i + 1) >= argc      // no more args
                    || (ctx->current_arg && ctx->current_arg->max_parameter_count <= i - __last_arg_idx) // reach max count
                ) {
                    LOG("do process for flag:  %s", arg_info_to_string(ctx->current_arg));
                    if (ctx->current_arg->process)
                        ctx->current_arg->process(i - __last_arg_idx, argv + __last_arg_idx + 1);
                    ctx->current_arg = NULL;
                }
            }
        }
	}
final_check:
    if (ctx->current_arg && ctx->current_arg->min_parameter_count > 0) {
        CHECK_ADDITIONAL_ARGS();
    }
    // check required positional arguments
    if (ctx->positional_minc > __global_positonal_argc) {
        PARSEARG_REPORT_ERROR("%d positional args provided, expected at least %d positional args",
                              __global_positonal_argc, ctx->positional_minc);
    }
    // check required arguments
    for (int i=0; i<ctx->args->size; i++) {
        arg_info_t* __a = (arg_info_t*)ctx->args->data[i];
        if (__a->required && __a->_requirement_satisfied_sign != ACANE_SIGN) {
            PARSEARG_REPORT_ERROR("missing required arg: --%s", arg_info_to_string(__a));
        }
    }
    return OK;
}

int argparse_set_positional_arg_process(args_context_t* ctx, void (*process)(int index, const char* arg)) {
    ctx->process_positional = process;
    return OK;
}

int argparse_set_directive_positional_arg_process(args_context_t* ctx, int (*process)(int index, int argc, const char** argv)) {
    ctx->process_directive_positional = process;
    return OK;
}

void argparse_enable_remove_ambiguous(args_context_t* ctx) {
    ctx->remove_ambiguous = 1;
}

int argparse_print_set_help_msg_width(args_context_t* ctx, int width) {
    if (!ctx) return FAIL;
    ctx->help_line_width = width;
    return OK;
}

size_t strlen_wd(const char* str) {
    int ch, nextch;
    const char* str_begin = str;
    while (1) {
        ch = *(str++);
        if (!ch)      // end of string
            break;
        nextch = *str;
        if (ch == ' ' && nextch != ' ') // [space][non-space]
            break;
    }
    return str-str_begin-1;
}

void print_line_wrap(args_context_t* ctx, const char* str, int leading_spaces) {
    int lwc=0; // line word count
    int lcc=0; // line character count
    int lccmax = ctx->help_line_width;
    char* buf = malloc(strlen(str) + 1);
    char* pbuf = buf;
    strcpy(buf, str);
    int flag = 1;
    while (flag) {
        size_t len = strlen_wd(str);
        pbuf[len] = 0;
        // LOG("current word: len=%zu [%s]", len, pbuf);
        // try print
        if (lcc+len > lccmax) {
            if (lwc > 0) {
                fprintf(ctx->output_file, "\n");
                for (int i=0; i<leading_spaces; i++)
                    fprintf(ctx->output_file, " ");
                lcc = 0;
            }
        }
        fprintf(ctx->output_file, "%s ", pbuf);
        lcc = lcc+len+1;
        lwc++;
        // skip this word
        pbuf += len + 1;  str += len + 1;
        flag = *(str-1);
    }
}

int print_help_print_addi_parameters_name(args_context_t* ctx, arg_info_t* __a, const char* str) {
    if (str == NULL)
        str = __a->max_parameter_count > 1 ? "ARG..." : "ARGS";
    //size_t len = strlen(str);
    // no parameter, print nothing
    if (__a->max_parameter_count == 0) {
        return 0;
    }
    // optional parameter
    if (__a->min_parameter_count == 0) {
        return fprintf(ctx->output_file, " [%s]", str);
    }
    // non-optional parameter
    else {
        return fprintf(ctx->output_file, " %s", str);
    }
    return 0;
}

int print_help_paramater_info(args_context_t* ctx, arg_info_t* arginfo) {
    assert(ctx);
    int _width = fprintf(ctx->output_file, "  ");
    if (arginfo->short_term) {
        _width += fprintf(ctx->output_file, "-%c", arginfo->short_term);
        if (!arginfo->long_term && arginfo->min_parameter_count > 0) {
            _width += print_help_print_addi_parameters_name(ctx, arginfo, NULL);
        }
    }
    if (arginfo->long_term) {
        if (arginfo->short_term) {
            _width += printf(", ");
        }
        else {
            _width += printf("    ");
        }
        _width += fprintf(ctx->output_file, "--%s", arginfo->long_term);
        if (arginfo->max_parameter_count > 0) {
            _width += print_help_print_addi_parameters_name(ctx, arginfo, NULL);
        }
    }
    return _width;
}

int argparse_print_help(args_context_t* ctx) {
    if (!ctx) return FAIL;
    assert(ctx->args);
    for (int i=0; i<ctx->args->size; i++) {
        arg_info_t* __a = (arg_info_t *) ctx->args->data[i];
        int width = print_help_paramater_info(ctx, __a);
        //LOG("  short: %c   long: %s,  minc: %d, maxc: %d\n", __a->short_term, __a->long_term, __a->min_parameter_count, __a->max_parameter_count);
        //LOG("    print count=%d", width);
        if (ctx->help_leading_spaces - 1 <= width) {
            fprintf(ctx->output_file, "\n");
            width = 0;
        }
        int remain = ctx->help_leading_spaces - width;
        for (int i=0; i<remain; i++)
            fprintf(ctx->output_file, " ");
        if (__a->description) {
            print_line_wrap(ctx, __a->description, ctx->help_leading_spaces);
        }
        fprintf(ctx->output_file, "\n");
    }
    return OK;
}

// =================================================================================

void __acane__unit_test_cb_process(int parac, const char** parav) {
    for (int i=0; i<parac; i++) {
        LOG("     [%d]  %s", i, parav[i]);
    }
    if (parac == 0) {
        LOG("     No args");
    }
}

int __acane__unit_test_cb_error_report(const char* __msg) {
    LOG("   %s", __msg);
    return 0;
}

void __acane_unit_test_cb_process_positional(int index, const char* arg) {
    LOG("   GLOBAL %d:  %s", index, arg);
}

void __acane__unit_test__full_test() {
    const char* argv[] = {
            "/Users/acane/__acane__argparse_unit_test",
            "-O", "__acane__argparse_unit_test", "-Ohhhh",
            "--files", "file1", "file2", "file3",
            "--license", "-h", "--fil", "file3", "file4", "file3", "file4",
            "--requi", "-l"
            //"--li",
    };
    int argc = sizeof(argv) / sizeof(const char*) - 1;

    args_context_t* ctx = init_args_context();
    argparse_add_parameter(ctx, "optional", 'O', "optional", 0, 1, 0, __acane__unit_test_cb_process);
    argparse_add_parameter(ctx, "files", 'f', "specify files", 1, 2, 0, __acane__unit_test_cb_process);
    argparse_add_parameter(ctx, "list", 'L', "list files", 0, 0, 0, __acane__unit_test_cb_process);
    argparse_add_parameter(ctx, "license", 0, "Show licenses", 0, 0, 0, __acane__unit_test_cb_process);
    argparse_add_parameter(ctx, "required", 'R', "Required", 0, 0, 1, __acane__unit_test_cb_process);
    argparse_add_parameter(ctx, "help", 'h', "Show help info", 0, 0, 0, __acane__unit_test_cb_process);
    argparse_add_parameter(ctx, NULL, 'l', "lll", 0, 0, 0, __acane__unit_test_cb_process);
    argparse_set_error_handle(ctx, __acane__unit_test_cb_error_report);
    argparse_set_positional_arg_process(ctx, __acane_unit_test_cb_process_positional);
    argparse_set_positional_args(ctx, 0, 3);

    argparse_print_help(ctx);
    parse_args(ctx, argc, argv);
    deinit_args_context(ctx);
}

void __acane__test_args() {
	args_context_t* ctx = init_args_context();
    argparse_add_parameter(ctx, "list", 'L', "list files", 0, 0, 0, NULL);
    argparse_add_parameter(ctx, "license", 0, "Show licenses", 0, 0, 0, NULL);
    argparse_add_parameter(ctx, "help", 'h', "Show help info", 0, 0, 0, NULL);
    argparse_add_parameter(ctx, NULL, 'l', "lll", 0, 0, 0, NULL);
	deinit_args_context(ctx);
}

void __acane__test_graph() {
	ctx_graph_t* graph = ctx_graph_init();
	//ctx_graph_add_nodes(graph, "help");
	ctx_graph_add_nodes(graph, "list");
	ctx_graph_add_nodes(graph, "licence");
	ctx_graph_add_nodes(graph, "license");
}

void __acane__test_print_wrap() {
    args_context_t * ctx = init_args_context();
    print_line_wrap(ctx, "The reason that they're distinct types is to (try to) prevent, or at least detect, errors in their use. If you declare an object of type int*, you're saying that you intend for it to point to an int object (if it's not a null pointer). Storing the address of a float object in your int* object is almost certainly a mistake. Enforcing type safety allows such mistakes to be detected as early as possible (when your compiler prints a warning rather than when your program crashes during a demo for an important client).",
                    ctx->help_leading_spaces);
    deinit_args_context(ctx);
}

void __acane__test_print() {
    args_context_t * ctx = init_args_context();
    argparse_add_parameter(ctx, "files", 0, "This is a test. This is a test. This is a test. This is a test. This is a test.", 1, 2, 1, __acane__unit_test_cb_process);
    argparse_add_parameter(ctx, "readable_items", 'R', "Load a readable item.", 1, 2, 0, __acane__unit_test_cb_process);
    argparse_add_parameter(ctx, "name", 'n', "This is a test. This is a test. This is a test. This is a test. This is a test.", 0, 1, 0, __acane__unit_test_cb_process);
    argparse_add_parameter(ctx, NULL, 'h', "This is a test. This is a test. This is a test. This is a test. This is a test.", 0, 0, 0, __acane__unit_test_cb_process);
    argparse_print_help(ctx);
    deinit_args_context(ctx);
}

int __acane__argparse_unit_test() {
    __acane__unit_test__full_test();
    //__acane__test_print();
    return 0;
}
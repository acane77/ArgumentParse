#include "args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define OK     1
#define FAIL   0
//#define DEBUG
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
	void        (*process)(int parac, const char** parav); // no free parav!
} arg_info_t;

/* graph nodes */
typedef struct arg_ctx_node {
	char        ch;     
	valarray_t* children;
	arg_info_t  arg_info;
	int         _arg_sign;
} ctx_node_t;

/* args graph-based metadata */
typedef struct arg_ctx_graph {
	ctx_node_t* head;
} ctx_graph_t;

typedef ctx_node_t* val_array_element_t;
#define VALARRAY_CAPACITY_PER_INCREASE 10

/* Variable length array for nodes */
struct valarray {
	val_array_element_t* nodes;
	size_t   capacity;
	size_t   size;
};

int valarray_extend_capacity(valarray_t* arr) {
	assert(arr && "arr is not initialized");
	val_array_element_t* _old = arr->nodes;
    val_array_element_t* _new = (val_array_element_t*)realloc((void*)_old, sizeof(val_array_element_t) * (arr->capacity + VALARRAY_CAPACITY_PER_INCREASE));
	if (!_new) {
		LOGE("allocate memory for capacity %lu failed", arr->capacity + 10);
		return FAIL;
	}
	arr->capacity += VALARRAY_CAPACITY_PER_INCREASE;
	arr->nodes = _new;
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
	(*arr)->nodes = NULL;
	return valarray_extend_capacity(*arr);
}

int valarray_push_back(valarray_t* arr, val_array_element_t el) {
	if (arr->size + 1 > arr->capacity) {
		int ret = valarray_extend_capacity(arr);
		if (ret != OK) {
			return ret;
		}
	}
	arr->nodes[arr->size++] = el;
	return OK;
}

val_array_element_t* valarray_get(valarray_t* arr, size_t __i) {
	assert(arr && "arr is not initialized");
	assert(arr->nodes && "arr is not initialized");
	if (arr->size <= __i) {
		LOGE("index out of range, with [size=%zu, i=%zu]", arr->size, __i);
		return NULL;
	}
	return &arr->nodes[__i];
}

int __valarray_default_el_cmp_fn(val_array_element_t* __lhs, val_array_element_t* __rhs) {
	return (*__lhs)->ch - (*__rhs)->ch;
}

size_t valarray_index_of(valarray_t* arr, val_array_element_t* x, int (*cmp)(val_array_element_t* __lhs, val_array_element_t* __rhs)) {
	for (int i = 0; i < arr->size; ++i)
		if (!cmp(&arr->nodes[i], x))
			return i;
	return -1;
}

size_t valarray_el_index_of(valarray_t* arr, int ch) {
	for (int i = 0; i < arr->size; ++i)
		if (arr->nodes[i]->ch == ch)
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

struct args_context {
	ctx_graph_t* ctx_graph;
	int (*error_handle)(const char* __msg);
	int current_addi_arg_count;
	arg_info_t* current_arg;
    int remove_ambiguous;

    // process positional args
    void (*process_positional)(int index, const char* arg);
    int positional_minc;
    int positional_maxc;
};

args_context_t* init_args_context() {
	args_context_t* ctx = (args_context_t*)malloc(sizeof(args_context_t));
	if (!ctx) return NULL;
	ctx->ctx_graph = ctx_graph_init();
	ctx->current_addi_arg_count = 0;
	ctx->current_arg = NULL;
    ctx->error_handle = NULL;
    ctx->process_positional = NULL;
    ctx->positional_maxc = 0;
    ctx->positional_minc = 0;
    ctx->remove_ambiguous = 0;
	return ctx;
}

void deinit_args_context(args_context_t* ctx) {
	if (ctx->ctx_graph)
        ctx_graph_free(ctx->ctx_graph);
	if (ctx)
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
		const char* description, int minc, int maxc,
		void (*process)(int parac, const char** parav), int is_long_term) {
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
	final_node->arg_info.description = description;
	if (is_long_term)
		final_node->arg_info.long_term = param;
	else
		final_node->arg_info.short_term = param[0];
	final_node->arg_info.max_parameter_count = maxc;
	final_node->arg_info.min_parameter_count = minc;
	final_node->arg_info.process = process;
	return OK;
}

int add_parameter_with_args(args_context_t* ctx, const char* long_term, char short_term,
				  const char* description, int minc, int maxc,
				  void (*process)(int parac, const char** parav)) {
	if (!ctx) return FAIL;
	if (!long_term && !short_term) {
		LOGE("at least one of long_term and short_term should be provided");
		return FAIL;
	}
	// register short term
	if (short_term) {
		char __short_term[2] = { 0, 0 };
		__short_term[0] = short_term;
		int ret = regsiter_parameter_on_graph(ctx, __short_term, description, minc, maxc, process, 0);
		if (ret != OK)
			return ret;
	}
	// register long term
	if (long_term) {
		int ret = regsiter_parameter_on_graph(ctx, long_term, description, minc, maxc, process, 1);
		if (ret != OK)
			return ret;
	}
	return OK;
}

int argparse_add_parameter(args_context_t* ctx, const char* long_term, char short_term,
	const char* description, int minc, int maxc,
	void (*process)(int parac, const char** parav)) {
	return add_parameter_with_args(ctx, long_term, short_term,
		description, minc, maxc, process);
}

int argparse_add_parameter_long_term(args_context_t* ctx, const char* long_term,
	const char* description,
	void (*process)(int parac, const char** parav)) {
	return add_parameter_with_args(ctx, long_term, PARAMETER_NO_SHORT_TERM,
		description, PARAMETER_NO_ARGS, PARAMETER_NO_ARGS, process);
}

int argparse_add_parameter_long_term_with_args(args_context_t* ctx, const char* long_term,
	const char* description, int minc, int maxc,
	void (*process)(int parac, const char** parav)) {
	return add_parameter_with_args(ctx, long_term, PARAMETER_NO_SHORT_TERM,
		description, minc, maxc, process);
}

int argparse_add_parameter_short_term(args_context_t* ctx, char short_term,
	const char* description,
	void (*process)(int parac, const char** parav)) {
	return add_parameter_with_args(ctx, PARAMETER_NO_LONG_TERM, short_term,
		description, PARAMETER_NO_ARGS, PARAMETER_NO_ARGS, process);
}

int argparse_add_parameter_short_term_with_args(args_context_t* ctx, char short_term,
	const char* description, int minc, int maxc,
	void (*process)(int parac, const char** parav)) {
	return add_parameter_with_args(ctx, PARAMETER_NO_LONG_TERM, short_term,
		description, minc, maxc, process);
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

    if (node->_arg_sign == ACANE_SIGN) // ????
        return &node->arg_info;
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
    process_if_no_args(ctx);      \
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
                ctx->process_positional(__global_positonal_argc++, arg);
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
    if (ctx->positional_minc > __global_positonal_argc) {
        PARSEARG_REPORT_ERROR("%d positional args provided, expected at least %d positional args",
                              __global_positonal_argc, ctx->positional_minc);
    }
    return OK;
}

int argparse_set_positional_arg_process(args_context_t* ctx, void (*process)(int index, const char* arg)) {
    ctx->process_positional = process;
    return OK;
}

void argparse_enable_remove_ambiguous(args_context_t* ctx) {
    ctx->remove_ambiguous = 1;
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
            "--license", "-lh", "--fil", "file3", "file4", "file3", "file4",
            "--li",
    };
    int argc = sizeof(argv) / sizeof(const char*) - 1;

    args_context_t* ctx = init_args_context();
    argparse_add_parameter(ctx, "optional", 'O', "optional", 0, 1, __acane__unit_test_cb_process);
    argparse_add_parameter(ctx, "files", 'f', "specify files", 1, 2, __acane__unit_test_cb_process);
    argparse_add_parameter(ctx, "list", 'L', "list files", 0, 0, __acane__unit_test_cb_process);
    argparse_add_parameter(ctx, "license", 0, "Show licenses", 0, 0, __acane__unit_test_cb_process);
    argparse_add_parameter(ctx, "help", 'h', "Show help info", 0, 0, __acane__unit_test_cb_process);
    argparse_add_parameter(ctx, NULL, 'l', "lll", 0, 0, __acane__unit_test_cb_process);
    argparse_set_error_handle(ctx, __acane__unit_test_cb_error_report);
    argparse_set_positional_arg_process(ctx, __acane_unit_test_cb_process_positional);
    argparse_set_positional_args(ctx, 0, 3);
    parse_args(ctx, argc, argv);
    deinit_args_context(ctx);
}

void __acane__test_args() {
	args_context_t* ctx = init_args_context();
    argparse_add_parameter(ctx, "list", 'L', "list files", 0, 0, NULL);
    argparse_add_parameter(ctx, "license", 0, "Show licenses", 0, 0, NULL);
    argparse_add_parameter(ctx, "help", 'h', "Show help info", 0, 0, NULL);
    argparse_add_parameter(ctx, NULL, 'l', "lll", 0, 0, NULL);
	deinit_args_context(ctx);
}

void __acane__test_graph() {
	ctx_graph_t* graph = ctx_graph_init();
	//ctx_graph_add_nodes(graph, "help");
	ctx_graph_add_nodes(graph, "list");
	ctx_graph_add_nodes(graph, "licence");
	ctx_graph_add_nodes(graph, "license");
}

void __acane__test_valarray() {
	ctx_node_t n1 = { 'a', NULL };
	ctx_node_t n2 = { 'b', NULL };
	valarray_t* arr;
	valarray_init(&arr);
	valarray_push_back(arr, &n1);
	valarray_push_back(arr, &n1);
	valarray_push_back(arr, &n1);
	valarray_push_back(arr, &n1);
	valarray_push_back(arr, &n1);
	valarray_push_back(arr, &n2);
	valarray_push_back(arr, &n2);
	valarray_push_back(arr, &n2);
	valarray_push_back(arr, &n2);
	valarray_push_back(arr, &n1);
	valarray_push_back(arr, &n1);
	valarray_push_back(arr, &n1);
	valarray_push_back(arr, &n1);
	valarray_push_back(arr, &n1);
	valarray_push_back(arr, &n2);
	valarray_push_back(arr, &n2);
	valarray_push_back(arr, &n2);
	valarray_push_back(arr, &n2);

	for (int i = 0; i < arr->size; i++) {
		ctx_node_t** n = valarray_get(arr, i);
		LOG("  arr[%d]=%c (%d)", i, (*n)->ch, (*n)->ch);
	}
}

int __acane__argparse_unit_test() {
    __acane__unit_test__full_test();
    return 0;
}
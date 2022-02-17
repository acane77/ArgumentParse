/*! \file args.c */

#include "args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define OK     1
#define FAIL   0
//ma#define DEBUG
#ifdef DEBUG
#define LOG(fmt, ...) printf("[%s:%d][ INFO  ] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#define LOG(fmt, ...)
#endif // DEBUG
#define LOGE(fmt, ...) printf("[%s:%d][ ERROR ] " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);

#define __ACANE_IN_OUT
#define __ACANE_OUT

#define FLAG_LEADING_PARAMETER  (1 << 0)
#define _ACANE_HAS_FLAG(_arg_ptr, _flag) ((_arg_ptr)->flag & _flag)

/* Node */
struct valarray;
typedef struct valarray valarray_t;

#define ACANE_SIGN 0x77061584

struct parse_result_item;

/* Arguments info */
typedef struct arg_info {
    const char* long_term;
    const char* description;
    int         short_term;
    int         min_parameter_count;
    int         max_parameter_count;
    int         directive_flag;   // treat all flags after into parameter of this flag
    void        (*process)(args_context_t* ctx, int parac, const char** parav); // no free parav!
    int         required;
    const char* arg_name;
    int         flag;

    // env
    int         _requirement_satisfied_sign;
    int         _arg_name_sign;
    struct parse_result_item* result_item;
} arg_info_t;

typedef struct parse_result_item {
    arg_info_t* arginfo;
    int count;
    valarray_t* args; // type: const char*
} parse_result_item_t;

struct parse_result {
    valarray_t* args;  // type: parse_result_item_t
    int keep_this_obj; // keep this obj, do not auto free
};

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
/////

void valarray_element_swap(val_array_element_t* a, val_array_element_t* b) {
    val_array_element_t t = *a;
    *a = *b;
    *b = t;
}

// quick sort
int valarray_quick_sort_partition(valarray_t* arr, int s, int e, int (*cmp)(val_array_element_t a, val_array_element_t b)) {
    int i=s;
    val_array_element_t x=arr->data[s];
    for (int j=s; j<=e; j++)
        if (cmp(arr->data[j], x))
            valarray_element_swap(&arr->data[++i], &arr->data[j]);
    valarray_element_swap(&arr->data[i], &arr->data[s]);
    return i;
}

void valarray_quick_sort(valarray_t* arr, int s, int e, int (*cmp)(val_array_element_t a, val_array_element_t b)) {
    if (s >= e)
        return;
    int m = valarray_quick_sort_partition(arr, s, e, cmp);
    valarray_quick_sort(arr, s, m-1, cmp);
    valarray_quick_sort(arr, m+1, e, cmp);
}

void valarray_sort(valarray_t* arr, int (*cmp)(val_array_element_t a, val_array_element_t b)) {
    assert(arr && cmp);
    valarray_quick_sort(arr, 0, arr->size-1, cmp);
}

// foreach
void valarray_foreach(valarray_t* arr, void (*fn)(val_array_element_t* el)) {
    for (int i=0; i<arr->size; i++) {
        fn(arr->data[i]);
    }
}

// free
void valarray_deinit(valarray_t* arr) {
    if (!arr) return;
    if (!arr->data) return;
    free(arr->data);
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
    valarray_t* positional_args;
    valarray_t* positional_args_description;

    // env
    arg_info_t* current_arg;
    parse_result_t* last_result;
};

int argparse_default_error_handle(const char* __msg) {
    fprintf(stderr, "error: %s\n", __msg);
    return 0;
}

args_context_t* init_args_context() {
    args_context_t* ctx = (args_context_t*)malloc(sizeof(args_context_t));
    if (!ctx) return NULL;
    ctx->ctx_graph = ctx_graph_init();
    ctx->current_addi_arg_count = 0;
    ctx->current_arg = NULL;
    ctx->error_handle = argparse_default_error_handle;
    ctx->process_positional = NULL;
    ctx->process_directive_positional = NULL;
    ctx->positional_maxc = 0;
    ctx->positional_minc = 0;
    ctx->remove_ambiguous = 0;
    ctx->help_line_width = _DEFAULT_HELP_LINE_WIDTH;
    ctx->help_leading_spaces = 25;
    ctx->output_file = stdout;
    valarray_init(&ctx->args);
    valarray_init(&ctx->positional_args);
    valarray_init(&ctx->positional_args_description);
    ctx->last_result = NULL;
    return ctx;
}

void deinit_args_context(args_context_t* ctx) {
    if (!ctx) return;
    // reset env to free some fields if neededg
    argparse_reset_env(ctx);
    // deinit graph
    if (ctx->ctx_graph)
        ctx_graph_free(ctx->ctx_graph);
    // deinit args
    if (ctx->args && ctx->args->data)
        free(ctx->args->data);
    if (ctx->args)
        free(ctx->args);
    // deinit positional args
    if (ctx->positional_args && ctx->positional_args->data)
        free(ctx->positional_args->data);
    if (ctx->positional_args)
        free(ctx->positional_args);
    if (ctx->positional_args_description && ctx->positional_args_description->data)
        free(ctx->positional_args_description->data);
    if (ctx->positional_args_description)
        free(ctx->positional_args_description);
    // free context
    free(ctx);
}

int argparse_set_positional_args(args_context_t* ctx, int minc, int maxc) {
    if (!ctx)
        return FAIL;
    ctx->positional_minc = minc;
    ctx->positional_maxc = maxc;
    return OK;
}

int argparse_set_positional_arg_name(args_context_t* ctx, const char* name, const char* description) {
    if (!ctx)
        return FAIL;
    valarray_push_back(ctx->positional_args, (void*) name);
    valarray_push_back(ctx->positional_args_description, (void*) description);
    return OK;
}

int regsiter_parameter_on_graph(args_context_t *ctx, const char *param, const char *description, int minc, int maxc,
                                int required, void (*process)(args_context_t *, int, const char **), int is_long_term,
                                int is_directive, arg_info_t **arginfo, int flag) {
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
    final_node->arg_info->arg_name = NULL;
    final_node->arg_info->flag = flag;
    if (arginfo)
        *arginfo = final_node->arg_info;
    return OK;
}

int
add_parameter_with_args_(args_context_t *ctx, const char *long_term, char short_term, const char *description, int minc,
                         int maxc, int required, void (*process)(args_context_t *, int, const char **),
                         int is_directive, int flag) {
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
        int ret = regsiter_parameter_on_graph(ctx, __short_term, description, minc, maxc, required, process, 0,
                                              is_directive, &arginfo, flag);
        if (ret != OK)
            return ret;
    }
    // register long term
    if (long_term) {
        int ret = regsiter_parameter_on_graph(ctx, long_term, description, minc, maxc, required, process, 1,
                                              is_directive, &arginfo, flag);
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

int argparse_add_parameter_with_args(args_context_t* ctx, const char* long_term, char short_term,
                                     const char* description, int minc, int maxc, int required, const char* arg_name,
                                     void (*process)(args_context_t* ctx, int parac, const char** parav)) {
    if (!ctx) return FAIL;
    static volatile int _locked = 0;
    while (_locked);
    _locked = 1;
    int ret = add_parameter_with_args_(ctx, long_term, short_term, description, minc, maxc, required, process, 0, 0);
    if (!ret) return FAIL;
    argparse_set_parameter_name(ctx, arg_name);
    _locked = 0;
    return OK;
}

int argparse_set_parameter_name(args_context_t* ctx, const char* arg_name) {
    if (!ctx) return FAIL;
    arg_info_t* _a = ctx->args->data[ctx->args->size-1];
    _a->_arg_name_sign = ACANE_SIGN;
    _a->arg_name = arg_name;
    return OK;
}

int add_parameter_with_args(args_context_t* ctx, const char* long_term, char short_term,
                  const char* description, int minc, int maxc, int required,
                  void (*process)(args_context_t* ctx, int parac, const char** parav)) {
    return add_parameter_with_args_(ctx, long_term, short_term, description, minc, maxc, required, process, 0, 0);
}

int argparse_add_parameter_directive(args_context_t* ctx, const char* long_term, char short_term,
                                     const char* description,  int required, void (*process)(args_context_t* ctx, int parac, const char** parav)) {
    return add_parameter_with_args_(ctx, long_term, short_term, description, 0, PARAMETER_ARGS_COUNT_NO_LIMIT, required,
                                    process, 1, 0);
}

int argpaese_add_short_leading_parameter(args_context_t* ctx, char short_term, const char* description, int required,
                                         void (*process)(args_context_t* ctx, int parac, const char** parav)) {
    return add_parameter_with_args_(ctx, NULL, short_term, description, 1, 1, required,
                                    process, 0, FLAG_LEADING_PARAMETER);
}


int argparse_add_parameter(args_context_t* ctx, const char* long_term, char short_term,
    const char* description, int minc, int maxc, int required,
    void (*process)(args_context_t* ctx, int parac, const char** parav)) {
    return add_parameter_with_args(ctx, long_term, short_term,
        description, minc, maxc, required, process);
}

int argparse_add_parameter_long_term(args_context_t* ctx, const char* long_term,
    const char* description, int required,
    void (*process)(args_context_t* ctx, int parac, const char** parav)) {
    return add_parameter_with_args(ctx, long_term, PARAMETER_NO_SHORT_TERM,
        description, PARAMETER_NO_ARGS, PARAMETER_NO_ARGS, required, process);
}

int argparse_add_parameter_long_term_with_args(args_context_t* ctx, const char* long_term,
    const char* description, int minc, int maxc, int required,
    void (*process)(args_context_t* ctx, int parac, const char** parav)) {
    return add_parameter_with_args(ctx, long_term, PARAMETER_NO_SHORT_TERM,
        description, minc, maxc, required, process);
}

int argparse_add_parameter_short_term(args_context_t* ctx, char short_term,
    const char* description, int required,
    void (*process)(args_context_t* ctx, int parac, const char** parav)) {
    return add_parameter_with_args(ctx, PARAMETER_NO_LONG_TERM, short_term,
        description, PARAMETER_NO_ARGS, PARAMETER_NO_ARGS, required, process);
}

int argparse_add_parameter_short_term_with_args(args_context_t* ctx, char short_term,
    const char* description, int minc, int maxc, int required,
    void (*process)(args_context_t* ctx, int parac, const char** parav)) {
    return add_parameter_with_args(ctx, PARAMETER_NO_LONG_TERM, short_term,
        description, minc, maxc, required, process);
}

void argparse_default_help_callback_(args_context_t* ctx, int parac, const char** parav) {
    argparse_print_help_usage(ctx, "test", NULL, NULL);
    exit(0);
}

int argparse_add_help_parameter(args_context_t* ctx, const char* program_name, const char* description) {
    if (!ctx) return FAIL;
    if (!description) description = "Print this help message and exit";
    argparse_add_parameter(ctx, "help", 'h', description, 0, 0, 0, argparse_default_help_callback_);
    return OK;
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
    for (; *arg && *arg != '='; ++arg) {
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
        goto return_an_arg_info;
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
        goto return_an_arg_info;
    }
    // no arg bound to this node
    return NULL;

return_an_arg_info:
    node->arg_info->_requirement_satisfied_sign = ACANE_SIGN;
    node->arg_info->result_item->count++;
    return node->arg_info;
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


parse_result_item_t* argparse_parse_result_item_init() {
    parse_result_item_t* _r = (parse_result_item_t*)malloc(sizeof(parse_result_item_t));
    if (!_r) return NULL;
    valarray_init(&_r->args);
    _r->count = 0;
    return _r;
}

void argparse_parse_result_item_deinit(parse_result_item_t* _r) {
    if (!_r) return;
    valarray_deinit(_r->args);
    free(_r);
}


parse_result_t* argparse_parse_result_init(args_context_t* ctx) {
    parse_result_t* _r = (parse_result_t*) malloc(sizeof(parse_result_t*));
    if (!_r) return NULL;
    _r->keep_this_obj = 0;
    valarray_init(&_r->args);
    // assign current result to arg_info object
    for (int i=0; i<ctx->args->size; i++) {
        arg_info_t* _a = ctx->args->data[i];
        parse_result_item_t* ri = argparse_parse_result_item_init();
        _a->result_item = ri;
        ri->arginfo = _a;
        valarray_push_back(_r->args, (void*) ri);
    }
    assert(_r->args->size == ctx->args->size);
    return _r;
}

void argparse_parse_result_deinit(parse_result_t* _r) {
    if (!_r) return;
    for (int i=0; i<_r->args->size; i++) {
        argparse_parse_result_item_deinit((parse_result_item_t*)_r->args->data[i]);
    }
    valarray_deinit(_r->args);
    free(_r);
}

parse_result_t* argparse_get_last_parse_result(args_context_t* ctx) {
    if (!ctx) return NULL;
    if (!ctx->last_result) return NULL;
    ctx->last_result->keep_this_obj = 1; // transfer memory control to callee
    return ctx->last_result;
}

int argparse_get_parsed_arg(parse_result_t* _r, const char* argname, parsed_argument_t* _out_a) {
    if (!_r)   return FAIL;
    if (!argname || !*argname) return FAIL;
    parse_result_item_t* item;
    for (int i=0; i<_r->args->size; i++) {
        item = (parse_result_item_t*)_r->args->data[i];
        // is short term
        if (!argname[1] && item->arginfo->short_term) {
            int short_term = *argname;
            if (short_term == item->arginfo->short_term)
                goto return_parsed_arg;
        }
        // is long term
        else if (argname[1] && item->arginfo->long_term) {
            if (!strcmp(argname, item->arginfo->long_term))
                goto return_parsed_arg;
        }
    }
    LOG(" no arg info for --%s", argname);
    return FAIL;

return_parsed_arg:
    LOG(" got arginfo --%s", argname);
    assert(item);
    _out_a->count = item->count;
    _out_a->parac = item->args->size;
    _out_a->parav = (const char**) item->args->data;
    return OK;
}

int argparse_count(parse_result_t* _r, const char* argname) {
    parsed_argument_t _a;
    int ret = argparse_get_parsed_arg(_r, argname, &_a);
    if (ret != OK) return 0;
    return _a.count;
}

void process_if_no_args(args_context_t* ctx) {
    // process if no need args
    if (ctx->current_arg->max_parameter_count == 0) {
        LOG("   ctx->current_arg->max_parameter_count=%d", ctx->current_arg->max_parameter_count);
        LOG("do process for flag (no args):  %s", arg_info_to_string(ctx->current_arg));
        if (ctx->current_arg->process)
            ctx->current_arg->process(ctx, 0, NULL);
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
    if (ctx->current_arg && ctx->current_arg->directive_flag) { \
        LOG("  -- is a directive flag");                                             \
        if (ctx->current_arg->process)               \
            ctx->current_arg->process(ctx, argc - __last_arg_idx, argv + __last_arg_idx);\
        return OK;\
    }                                                     \
} while (0)

int argparse_reset_env(args_context_t* ctx) {
    if (!ctx) return FAIL;
    // reset env vars in ctx
    ctx->current_arg = NULL;
    // reset env vars in args
    for (int i=0; i<ctx->args->size; i++) {
        arg_info_t* _a = ctx->args->data[i];
        assert(_a);
        _a->_requirement_satisfied_sign = 0;
        _a->_arg_name_sign = 0;
        _a->result_item = NULL;
    }
    // free result if needed
    if (ctx->last_result && !ctx->last_result->keep_this_obj) {
        argparse_parse_result_deinit(ctx->last_result);
        ctx->last_result = NULL;
    }
    return OK;
}

int parse_args(args_context_t* ctx, int argc, const char** argv) {
    int __last_arg_idx = 0;
    int __global_positonal_argc = 0;
    argparse_reset_env(ctx);
    // initialize record of this time of parse
    ctx->last_result = argparse_parse_result_init(ctx);
    assert(ctx->last_result);

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
                    ctx->current_arg->process(ctx, 0, NULL);
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
                while (*(++arg)) {
                    // if is --name=value
                    if (*arg == '=') {
                        goto process_inl_arg;
                    }
                }
            }
            // if is -f flag
            else {
                // process combined multiply parameters: -abcd
                while (*(++arg)) {
                    // check addtional args for every parameter
                    CHECK_ADDITIONAL_ARGS();

                    // if is -n=value
                    if (*arg == '=') {
                        goto process_inl_arg;
                    }

                    char __s[2] = {0, 0};  __s[0] = *arg;
                    LOG("process -%s", __s);
                    GET_PARAMETER_FROM_GRAPH_AND_CHECK(__s);
                    // check if is leading flag, e.g., -Dvariable=value
                    LOG(" current flag: %d", ctx->current_arg->flag);
                    if (ctx->current_arg && _ACANE_HAS_FLAG(ctx->current_arg, FLAG_LEADING_PARAMETER)) {
                        if (*(arg+1)) // if not an empty argg
                            goto process_inl_arg;
                    }
                }
            }

            volatile const int false_cond = 0;
            if (false_cond) {
process_inl_arg:
                arg++; // eat equal sign
                // process inline positional arg xxx=value
                LOG("inline positional arg for [%s]: %s", arg_info_to_string(ctx->current_arg), arg);

                // Add this parameter (arg) to current_arg
                valarray_push_back(ctx->current_arg->result_item->args, (void*)arg);
                LOG("Added parameter to [%s] [arglist.size=%zu]: %s", arg_info_to_string(ctx->current_arg),
                    ctx->current_arg->result_item->args->size, arg);

                // add this to ensure only one parameter will be obtained
                ctx->current_arg = NULL;
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

                // Add this parameter (argv[i]) to current_arg
                valarray_push_back(ctx->current_arg->result_item->args, (void*)argv[i]);
                LOG("Added parameter to [%s] [arglist.size=%zu]: %s", arg_info_to_string(ctx->current_arg),
                    ctx->current_arg->result_item->args->size, argv[i]);

                // If current parameter has args, and no args anymore, do process
                if ((i + 1 < argc && argv[i+1][0] == '-'   // next arg is a flag
                    && ctx->current_arg && ctx->current_arg->max_parameter_count != 0)   // this parameter requires args
                    || (i + 1) >= argc      // no more args
                    || (ctx->current_arg && ctx->current_arg->max_parameter_count <= i - __last_arg_idx) // reach max count
                ) {
                    LOG("do process for flag:  %s", arg_info_to_string(ctx->current_arg));
                    if (ctx->current_arg->process)
                        ctx->current_arg->process(ctx, i - __last_arg_idx, argv + __last_arg_idx + 1);
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

int argparse_print_set_help_msg_leading_spaces(args_context_t* ctx, int width) {
    if (!ctx) return FAIL;
    ctx->help_leading_spaces = width;
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

int help_print_addi_parameters_name_to_str(arg_info_t* __a, const char* str, char* buf) {
    if (str == NULL)
        str = __a->max_parameter_count > 1 ? "ARG..." : "ARGS";
    //size_t len = strlen(str);
    // no parameter, print nothing
    if (__a->max_parameter_count == 0) {
        return 0;
    }
    // optional parameter
    if (__a->min_parameter_count == 0) {
        return sprintf(buf, " [%s]", str);
    }
    // non-optional parameter
    else {
        return sprintf(buf, " %s", str);
    }
    return 0;
}

#define PRINT_HELP_PARAMETER_(str) do { \
    _width += help_print_addi_parameters_name_to_str(arginfo, str, buf + _width);\
} while (0)

int print_help_paramater_info(args_context_t* ctx, arg_info_t* arginfo, char** _out_buf) {
    assert(ctx);
    static char buf[1024];
    int _width = sprintf(buf, "  ");
    if (arginfo->short_term) {
        _width += sprintf(buf + _width, "-%c", arginfo->short_term);
        if (!arginfo->long_term && arginfo->min_parameter_count > 0) {
            PRINT_HELP_PARAMETER_(arginfo->arg_name);
        }
    }
    if (arginfo->long_term) {
        if (arginfo->short_term) {
            _width += sprintf(buf + _width, ", ");
        }
        else {
            _width += sprintf(buf + _width, "    ");
        }
        _width += sprintf(buf + _width, "--%s", arginfo->long_term);
        if (arginfo->max_parameter_count > 0) {
            PRINT_HELP_PARAMETER_(arginfo->arg_name);
        }
    }
    *_out_buf = buf;
    return _width;
}

int argparse_print_help(args_context_t* ctx) {
    if (!ctx) return FAIL;
    assert(ctx->args);
    for (int i=0; i<ctx->args->size; i++) {
        arg_info_t* __a = (arg_info_t *) ctx->args->data[i];
        char* buf = NULL;
        int width = print_help_paramater_info(ctx, __a, &buf);
        fprintf(ctx->output_file, "%s", buf);
        //LOG("  short: %c   long: %s,  minc: %d, maxc: %d\n", __a->short_term, __a->long_term, __a->min_parameter_count, __a->max_parameter_count);
        //LOG("    print count=%d", width);
        if (ctx->help_leading_spaces <= width) {
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

int argparse_print_help_for_positional(args_context_t* ctx) {
    if (!ctx) return FAIL;
    assert(ctx->positional_args->size == ctx->positional_args_description->size);
    for (int i=0; i<ctx->positional_args->size; i++) {
        if (ctx->positional_args_description->data[i]) {
            const char* name = ctx->positional_args->data[i];
            const char* description = ctx->positional_args_description->data[i];
            int w = fprintf(ctx->output_file, "  %s", name);
            if (w > ctx->help_leading_spaces) {
                w = 0;
                fprintf(ctx->output_file, "\n");
            }
            for (int j=w; j<ctx->help_leading_spaces; j++)
                fputc(' ', ctx->output_file);
            print_line_wrap(ctx, description, ctx->help_leading_spaces);
            fputc('\n', ctx->output_file);
        }
    }
    return OK;
}

int argparse_compare_arginfo_fn(val_array_element_t a, val_array_element_t b) {
    arg_info_t* __a = (arg_info_t*)a;
    arg_info_t* __b = (arg_info_t*)b;
    return strcmp(arg_info_to_string(__a), arg_info_to_string(__b)) < 0;
}

int argparse_sort_parameters(args_context_t* ctx) {
    if (!ctx) return FAIL;
    valarray_sort(ctx->args, argparse_compare_arginfo_fn);
    return OK;
}

#define MAX_USAGE_LEADING_SPACE 25
#define FOREACH_ARG_START \
for (int i=0; i<ctx->args->size; i++) {\
    arg_info_t* __a = ctx->args->data[i];
#define FOREACH_ARG_END    }
#define PRINT_USAGE_(fmt, ...) _width += fprintf(ctx->output_file, fmt, ##__VA_ARGS__)
#define PRINT_USAGE_LEADING_SPACE_() do {\
    for (int __i=0; __i<leading_spaces; __i++) {\
        fprintf(ctx->output_file, " ");\
    }\
} while (0)
int argparse_print_usage(args_context_t* ctx, const char* program_name) {
    int pnlen = strlen(program_name) + strlen("usage: ") + 1;
    int leading_spaces = pnlen < MAX_USAGE_LEADING_SPACE ? pnlen : MAX_USAGE_LEADING_SPACE;
    LOG("LEADING SPACES=%d", leading_spaces);
    int max_width = (ctx->help_leading_spaces + ctx->help_line_width - leading_spaces);
    int _width = 0;
    // print program name
    PRINT_USAGE_("usage: %s ", program_name);
    if (_width > leading_spaces) {
        _width -= leading_spaces;
    }
    // restore width to 0 (real initial width)
    _width = 0;

    static char buf[1024];   // parameter in string

    // print short required usage without parameter
    int print_l = 0, print_e = 1;
    int _c = 0;
    FOREACH_ARG_START
        if (__a->required && __a->short_term && __a->max_parameter_count == 0) {
            if (!print_l) {
                PRINT_USAGE_("-");
                print_l = 1;
            }
            int w = sprintf(buf, "%c", __a->short_term);
            if (w + _width > max_width) {
                PRINT_USAGE_("\n");
                PRINT_USAGE_LEADING_SPACE_();
                _width = 0;
                print_l = 0;
            }
            _width += fprintf(ctx->output_file, "%s", buf);
            _c++;
        }
    FOREACH_ARG_END
    // print short optional usage without parameter
    print_l = 0;
    if (_c)   PRINT_USAGE_(" ");
    _c = 0;
    FOREACH_ARG_START
        if (!__a->required && __a->short_term && __a->max_parameter_count == 0) {
            int w = sprintf(buf, "%c", __a->short_term);
            if (w + _width > max_width) {
                PRINT_USAGE_("]\n");
                PRINT_USAGE_LEADING_SPACE_();
                _width = 0;
                print_e = 1;
            }
            if (!print_l) {
                PRINT_USAGE_("[-");
                print_l = 1;
                print_e = 0;
            }
            _width += fprintf(ctx->output_file, "%s", buf);
        }
    FOREACH_ARG_END
    if (!print_e) PRINT_USAGE_("] ");
    // print long required usage, or short usage with args
    FOREACH_ARG_START
        if ((__a->required && !__a->short_term) || (__a->short_term && __a->required && __a->max_parameter_count > 0)) {
            int w = (__a->required && !__a->short_term) ?
                    sprintf(buf, "--%s%s", __a->long_term, __a->max_parameter_count > 0 ? "" : " ") :
                    sprintf(buf, "-%c", __a->short_term);
            // with arg
            if (__a->max_parameter_count > 0) {
                // optional parameter
                w += help_print_addi_parameters_name_to_str(__a, __a->arg_name, buf + w);
                w += sprintf(buf + w, " ");
            }
            if (w + _width > max_width) {
                _width = 0;
                fprintf(ctx->output_file, "\n");
                PRINT_USAGE_LEADING_SPACE_();
            }
            PRINT_USAGE_("%s", buf);
        }
    FOREACH_ARG_END
    // print long optional usage, or short usage with args
    FOREACH_ARG_START
        if ((!__a->required && !__a->short_term) || (__a->short_term && !__a->required && __a->max_parameter_count > 0)) {
            int w = (!__a->required && !__a->short_term) ?
                     sprintf(buf, "[--%s", __a->long_term) :
                     sprintf(buf, "[-%c", __a->short_term);
            // with arg
            if (__a->max_parameter_count > 0) {
                // optional parameter
                w += help_print_addi_parameters_name_to_str(__a, __a->arg_name, buf + w);
            }
            w += sprintf(buf + w, "] ");
            if (w + _width > max_width) {
                _width = 0;
                fprintf(ctx->output_file, "\n");
                PRINT_USAGE_LEADING_SPACE_();
            }
            PRINT_USAGE_("%s", buf);

        }
    FOREACH_ARG_END

    /// print required positional args
    for (int i=0; i<ctx->positional_minc; i++) {
        int w = i < ctx->positional_args->size ?
                sprintf(buf, "%s ",  (char*)ctx->positional_args->data[i]) :
                sprintf(buf, "ARG%d ", i+1);
        if (w + _width > max_width) {
            _width = 0;
            fprintf(ctx->output_file, "\n");
            PRINT_USAGE_LEADING_SPACE_();
        }
        PRINT_USAGE_("%s", buf);
    }

    /// print optional positional args
    for (int i=ctx->positional_minc; i<ctx->positional_maxc; i++) {
        if (i >= ctx->positional_args->size) {
            PRINT_USAGE_("...");
            break;
        }
        int w = sprintf(buf, "[%s] ",  (char*)ctx->positional_args->data[i]);
        if (w + _width > max_width) {
            _width = 0;
            fprintf(ctx->output_file, "\n");
            PRINT_USAGE_LEADING_SPACE_();
        }
        PRINT_USAGE_("%s", buf);
    }

    PRINT_USAGE_("\n");
    return OK;
}

int argparse_print_help_usage(args_context_t* ctx, const char* program_name, const char* title_for_position, const char* title_for_args) {
    if (!ctx) return FAIL;
    argparse_print_usage(ctx, program_name);
    fprintf(ctx->output_file, "\n");

    // try print help for positional arguments
    int positional_count = 0;
    for (int i=0; i<ctx->positional_args_description->size; i++)
        if (((arg_info_t*)ctx->positional_args_description->data[i])->description)
            positional_count++;
    if (positional_count) {
        fprintf(ctx->output_file, "%s:\n", title_for_position ? title_for_position : "Positional Arguments");
        argparse_print_help_for_positional(ctx);
        fprintf(ctx->output_file, "\n");
    }

    // print help
    fprintf(ctx->output_file, "%s:\n", title_for_args ? title_for_args : "Arguments");
    argparse_print_help(ctx);
    return OK;
}

int argparse_set_print_file(args_context_t* ctx, FILE* file) {
    if (!ctx) return FAIL;
    ctx->output_file = file;
    return OK;
}

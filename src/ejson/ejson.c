#include <stdio.h>
#include <string.h>

#include "ejson.h"

#if DEBUG > 3
#include <stdio.h>

static char printBuf[48];

#define DBG(write_cb, FORMAT, ...)           \
    {                                        \
        snprintf(printBuf, sizeof(printBuf), \
                 FORMAT, ##__VA_ARGS__);     \
        if (write_cb)                        \
            write_cb(printBuf);              \
    }

#define DBGLN(write_cb, FORMAT, ...)            \
    {                                           \
        snprintf(printBuf, sizeof(printBuf),    \
                 FORMAT "\r\n", ##__VA_ARGS__); \
        if (write_cb)                           \
            write_cb(printBuf);                 \
    }
#else
#define DBG(write_cb, FORMAT, ...)
#define DBGLN(write_cb, FORMAT, ...)
#endif

#if DEBUG > 2
#define INFO(write_cb, FORMAT, ...)          \
    {                                        \
        snprintf(printBuf, sizeof(printBuf), \
                 FORMAT, ##__VA_ARGS__);     \
        if (write_cb)                        \
            write_cb(printBuf);              \
    }

#define INFOLN(write_cb, FORMAT, ...)           \
    {                                           \
        snprintf(printBuf, sizeof(printBuf),    \
                 FORMAT "\r\n", ##__VA_ARGS__); \
        if (write_cb)                           \
            write_cb(printBuf);                 \
    }
#else
#define INFO(write_cb, FORMAT, ...)
#define INFOLN(write_cb, FORMAT, ...)
#endif

#if DEBUG > 1
#define ERROR(write_cb, FORMAT, ...)         \
    {                                        \
        snprintf(printBuf, sizeof(printBuf), \
                 FORMAT, ##__VA_ARGS__);     \
        if (write_cb)                        \
            write_cb(printBuf);              \
    }

#define ERRORLN(write_cb, FORMAT, ...)          \
    {                                           \
        snprintf(printBuf, sizeof(printBuf),    \
                 FORMAT "\r\n", ##__VA_ARGS__); \
        if (write_cb)                           \
            write_cb(printBuf);                 \
    }
#else
#define ERROR(write_cb, FORMAT, ...)
#define ERRORLN(write_cb, FORMAT, ...)
#endif

enum ejson_token_type
{
    EJSON_TOKEN_JSON_UNINIT,
    EJSON_TOKEN_JSON,
    // EJSON_TOKEN_JSON_START,
    // EJSON_TOKEN_JSON_END,
    EJSON_TOKEN_KEYVAL,
    EJSON_TOKEN_KEY,
    // EJSON_TOKEN_KEY_START,
    // EJSON_TOKEN_KEY_END,
    EJSON_TOKEN_SEP,
    // EJSON_TOKEN_SEP_START,
    // EJSON_TOKEN_SEP_END,
    EJSON_TOKEN_VAL,
    // EJSON_TOKEN_VAL_START,
    // EJSON_TOKEN_VAL_END,
    EJSON_TOKEN_ARRAY,
    // EJSON_TOKEN_ARRAY_START,
    // EJSON_TOKEN_ARRAY_END,
    EJSON_TOKEN_NUM,
    // EJSON_TOKEN_NUM_START,
    // EJSON_TOKEN_NUM_END,
    EJSON_TOKEN_STRING,
    // EJSON_TOKEN_STRING_START,
    // EJSON_TOKEN_STRING_END,
    EJSON_TOKEN_BOOLEAN_TRUE,
    EJSON_TOKEN_BOOLEAN_FALSE,

    EJSON_TOKEN_NULL_OBJ,
};

struct ejson_token
{
    enum ejson_token_type type;
    int idx_start;
    int idx_end;
};

struct ejson_spad
{
    uint8_t *spad;
    int spad_used;
    int len;
};

struct ejson_keyval_node
{
    struct ejson_keyval *keyval;
    struct ejson_keyval_node *next;
};

struct ejson_keyval_list
{
    int num_keyval;
    struct ejson_keyval_node *head;
    struct ejson_keyval_node *last_added;
};

enum ejson_obj_type
{
    EJSON_OBJ_TYPE_UNK,
    EJSON_OBJ_TYPE_JSON,
    EJSON_OBJ_TYPE_ARRAY
};

struct ejson_internal_ctx
{
    struct ejson_spad *spad;

    struct ejson_token token_stack[7];
    int stacktop;

    enum ejson_obj_type obj_type;
    struct ejson_val_list list_vals;
    struct ejson_keyval_list list_keyval;

    struct ejson_internal_ctx *parent_ctx;
    struct ejson_keyval *parent_keyval;

    debug_write_cb_t dbg_write;
};

static inline int align(struct ejson_internal_ctx *ctx, int ptr, int align)
{
    int offset;

    offset = (ptr % align);

    if (offset > 0)
    {
        offset = align - offset;
        DBGLN(ctx->dbg_write, "Adding %d bytes to align to %d", offset, align);
    }
    return offset;
}

void *ejson_malloc(struct ejson_internal_ctx *ctx, int size)
{
    void *ptr;

    ctx->spad->spad_used += align(ctx, ctx->spad->spad_used, sizeof(void *));

    ptr = &ctx->spad->spad[ctx->spad->spad_used];
    ctx->spad->spad_used += size;

    DBGLN(ctx->dbg_write, "MALLOC RET: %p", ptr);
    DBGLN(ctx->dbg_write, "SPAD USED: %d", ctx->spad->spad_used);

    return ptr;
}

void ejson_val_list_init(struct ejson_val_list *list)
{
    list->head = NULL;
    list->last_added = NULL;
    list->num_vals = 0;
}

void ejson_val_list_add(struct ejson_internal_ctx *ctx, struct ejson_val_list *list, struct ejson_val *val)
{
    struct ejson_val_node *new = (struct ejson_val_node *)ejson_malloc(ctx, sizeof(struct ejson_val_node));

    new->val = val;
    new->next = NULL;

    if (list->num_vals == 0)
    {
        list->head = new;
    }
    else
    {
        list->last_added->next = new;
    }

    list->last_added = new;
    list->num_vals++;
}

void ejson_keyval_list_init(struct ejson_internal_ctx *ctx)
{
    ctx->list_keyval.num_keyval = 0;
    ctx->list_keyval.head = NULL;
    ctx->list_keyval.last_added = NULL;
}

void ejson_keyval_list_add(struct ejson_internal_ctx *ctx, struct ejson_keyval *keyval)
{
    struct ejson_keyval_node *new = (struct ejson_keyval_node *)ejson_malloc(ctx, sizeof(struct ejson_keyval_node));

    new->keyval = keyval;
    new->next = NULL;

    if (ctx->list_keyval.num_keyval == 0)
    {
        ctx->list_keyval.head = new;
    }
    else
    {
        ctx->list_keyval.last_added->next = new;
    }

    ctx->list_keyval.last_added = new;
    ctx->list_keyval.num_keyval++;
}

static int ejson_init_internal_ctx(struct ejson_internal_ctx *ctx, struct ejson_spad *spad, struct ejson_internal_ctx *parent_ctx, struct ejson_keyval *parent_keyval, debug_write_cb_t dbg_write)
{
    ctx->spad = spad;
    ctx->parent_ctx = parent_ctx;
    ctx->parent_keyval = parent_keyval;

    ctx->stacktop = -1;
    memset(ctx->token_stack, 0, sizeof(struct ejson_token) * 6);

    ejson_keyval_list_init(ctx);

    ctx->dbg_write = dbg_write;

    return 0;
}

int ejson_init_ctx(struct ejson_ctx *uctx, uint8_t *spadbuf, int len_spadbuf, debug_write_cb_t dbg_write)
{
    struct ejson_internal_ctx *ctx;
    struct ejson_spad *_spad;

    if (uctx == NULL)
        return -1;

    ctx = (struct ejson_internal_ctx *)uctx;

    _spad = (struct ejson_spad *)spadbuf;
    _spad->spad = (uint8_t *)(spadbuf + sizeof(struct ejson_spad));
    _spad->spad_used = 0;
    _spad->len = len_spadbuf - sizeof(struct ejson_spad);

    ejson_init_internal_ctx(ctx, _spad, NULL, NULL, dbg_write);

    DBGLN(dbg_write, "sizeof(ejson_internal_ctx): %lld", sizeof(struct ejson_internal_ctx));
    DBGLN(dbg_write, "sizeof(ejson_ctx): %lld", sizeof(struct ejson_ctx));
    DBGLN(dbg_write, "sizeof(ejson_spad): %lld", sizeof(struct ejson_spad));
    DBGLN(dbg_write, "sizeof(ejson_token): %lld", sizeof(struct ejson_token));
    DBGLN(dbg_write, "sizeof(ejson_val): %lld", sizeof(struct ejson_val));

    return 0;
}

int ejson_parse_int(char *buf, int len, int *val)
{
    int ret;
    int parsed;

    ret = sscanf(buf, "%d", &parsed);
    if (ret != 1)
        return -20;

    *val = parsed;

    return 0;
}

int ejson_parse_float(char *buf, int len, float *val)
{

    int ret;
    float parsed;

    ret = sscanf(buf, "%f", &parsed);
    if (ret != 1)
        return -20;

    *val = parsed;

    return 0;
}

static inline struct ejson_token *ejson_push_token(struct ejson_internal_ctx *ctx, enum ejson_token_type type, int start_idx)
{
    struct ejson_token *token_top;

    ctx->stacktop++;

    token_top = &ctx->token_stack[ctx->stacktop];
    token_top->type = type;
    token_top->idx_start = start_idx;

    return token_top;
}

static inline struct ejson_token *ejson_pop_token(struct ejson_internal_ctx *ctx, int end_idx)
{

    struct ejson_token *token_top;

    if (ctx->stacktop < 0)
        return NULL;

    token_top = &ctx->token_stack[ctx->stacktop];
    token_top->idx_end = end_idx;

    ctx->stacktop--;

    if (ctx->stacktop == -1)
        return NULL;

    return &ctx->token_stack[ctx->stacktop];
}

int ejson_loads(char *buf, int len, struct ejson_ctx *uctx)
{
    struct ejson_internal_ctx *ctx = (struct ejson_internal_ctx *)uctx;
    int offset = 0;
    uint8_t temp;
    uint8_t *temp_str;
    int temp_str_len = 0;
    int ret;
    struct ejson_token *token_top = NULL;
    int isEscaped = 0;

    /* Set number of found keyvals to 0 */
    ctx->spad->spad_used = 0;

    ejson_init_internal_ctx(ctx, ctx->spad, NULL, NULL, ctx->dbg_write);

    struct ejson_keyval *curKeyVal = NULL;
    struct ejson_val *curVal = NULL;
    struct ejson_internal_ctx *parJsonCtx = NULL;
    struct ejson_internal_ctx *curJsonCtx = ctx;

    while ((offset < len) && buf[offset] != '\0')
    {
        temp = buf[offset++];
        if (temp == ' ' || temp == '\r' || temp == '\n' || temp == '\t')
            continue;

        else if (temp == '{')
        {
            token_top = ejson_push_token(curJsonCtx, EJSON_TOKEN_JSON, offset);
            curJsonCtx->obj_type = EJSON_OBJ_TYPE_JSON;
            break;
        }
        else if (temp == '[')
        {
            curJsonCtx->obj_type = EJSON_OBJ_TYPE_ARRAY;
            ejson_val_list_init(&curJsonCtx->list_vals);

            token_top = ejson_push_token(curJsonCtx, EJSON_TOKEN_ARRAY, offset);
            token_top = ejson_push_token(curJsonCtx, EJSON_TOKEN_VAL, offset);

            break;
        }

        ERRORLN(curJsonCtx->dbg_write, "Invalid token @ %d, expected '{'", offset);
        return -30;
    }

    if (token_top == NULL)
    {
        INFO(curJsonCtx->dbg_write, "Empty string or No JSON found\r\n");
        return -70;
    }

    while ((offset < len) && buf[offset] != '\0')
    {
        temp = buf[offset];

        switch (token_top->type)
        {
        case EJSON_TOKEN_JSON_UNINIT:

            DBG(curJsonCtx->dbg_write, "Invalid internal state @ %d, expected '{'", offset);
            break;
        case EJSON_TOKEN_JSON:
            if (temp == ' ' || temp == '\r' || temp == '\n' || temp == '\t')
                break;
            else if (temp == '"')
            {
                DBG(curJsonCtx->dbg_write, "Key start found @ %d\r\n", offset);

                token_top = ejson_push_token(curJsonCtx, EJSON_TOKEN_KEYVAL, offset);
                token_top = ejson_push_token(curJsonCtx, EJSON_TOKEN_KEY, offset);

                /* Allocate JSON keyval element */
                curKeyVal = (struct ejson_keyval *)ejson_malloc(curJsonCtx, sizeof(struct ejson_keyval));
                curKeyVal->val = NULL;

                /* Add key val to list */
                ejson_keyval_list_add(curJsonCtx, curKeyVal);

                /* Init keyval */
                curKeyVal->key = ejson_malloc(curJsonCtx, 1);

                curKeyVal->key[0] = '\0';
                curKeyVal->key_len = 0;

                break;
            }
            else if (temp == '}')
            {
                if (curJsonCtx->parent_ctx == NULL)
                {
                    /* Pased JSON successfully */
                    INFO(curJsonCtx->dbg_write, "End of root JSON\r\n");
                    INFO(curJsonCtx->dbg_write, "Total scratchpad used: %d Bytes\r\n", ctx->spad->spad_used);
                    return 0;
                }
                else
                {
                    curKeyVal = curJsonCtx->parent_keyval;
                    curJsonCtx = curJsonCtx->parent_ctx;

                    token_top = &curJsonCtx->token_stack[curJsonCtx->stacktop];
                }

                break;
            }

            /* Malformed json */
            ERRORLN(curJsonCtx->dbg_write, "Invalid token @ %d, expected '\"' or '}'\r\n", offset)
            return -30;

            break;
        case EJSON_TOKEN_KEY:
            if (isEscaped == 1)
            {
                isEscaped = 0;
                break;
            }
            if (temp == '\\')
            {
                isEscaped = 1;
                break;
            }

            if (temp == '"')
            {
                DBG(curJsonCtx->dbg_write, "Key end found @ %d\r\n", offset);

                /* Add END of string to Key */
                curKeyVal->key[curKeyVal->key_len] = '\0';
                curJsonCtx->spad->spad_used++;

                token_top = ejson_pop_token(curJsonCtx, offset);

                token_top = ejson_push_token(curJsonCtx, EJSON_TOKEN_SEP, offset + 1);
                break;
            }

            /* Add character to Key */
            curKeyVal->key[curKeyVal->key_len++] = temp;
            curJsonCtx->spad->spad_used++;
            break;

        case EJSON_TOKEN_SEP:
            if (temp == ' ' || temp == '\r' || temp == '\n' || temp == '\t')
                break;

            if (temp == ':')
            {
                token_top = ejson_pop_token(curJsonCtx, offset);
                token_top = ejson_push_token(curJsonCtx, EJSON_TOKEN_VAL, offset);
                break;
            }

            /* Malformed json */
            ERRORLN(curJsonCtx->dbg_write, "Invalid token @ %d, expected ':'\r\n", offset)
            return -30;

            break;

        case EJSON_TOKEN_VAL:
            if (temp == ' ' || temp == '\r' || temp == '\n' || temp == '\t')
                break;

            if (temp == '"')
            {

                DBG(curJsonCtx->dbg_write, "String start found @ %d\r\n", offset);
                token_top = ejson_push_token(curJsonCtx, EJSON_TOKEN_STRING, offset);

                curVal = (struct ejson_val *)ejson_malloc(ctx, sizeof(struct ejson_val));
                curVal->type = EJSON_VAL_TYPE_STRING;

                /* Add value to list */
                switch (curJsonCtx->obj_type)
                {
                case EJSON_OBJ_TYPE_JSON:
                    curKeyVal->val = curVal;
                    break;
                case EJSON_OBJ_TYPE_ARRAY:
                    ejson_val_list_add(curJsonCtx, &curJsonCtx->list_vals, curVal);
                    break;
                default:
                    break;
                }

                curVal->val.str_val.len = 0;
                curVal->val.str_val.str = ejson_malloc(ctx, 1);

                curVal->val.str_val.str[0] = '\0';

                break;
            }
            else if ((temp >= '0' && temp <= '9') || temp == '+' || temp == '-')
            {
                DBG(curJsonCtx->dbg_write, "NUM start found @ %d\r\n", offset);
                token_top = ejson_push_token(curJsonCtx, EJSON_TOKEN_NUM, offset);

                curVal = (struct ejson_val *)ejson_malloc(ctx, sizeof(struct ejson_val));
                curVal->type = EJSON_VAL_TYPE_INT;

                /* Add value to list */
                switch (curJsonCtx->obj_type)
                {
                case EJSON_OBJ_TYPE_JSON:
                    curKeyVal->val = curVal;
                    break;
                case EJSON_OBJ_TYPE_ARRAY:
                    ejson_val_list_add(curJsonCtx, &curJsonCtx->list_vals, curVal);
                    break;
                default:
                    break;
                }

                temp_str = &curJsonCtx->spad->spad[curJsonCtx->spad->spad_used];
                temp_str[0] = temp;
                temp_str_len = 1;
                break;
            }
            else if (temp == 't' || temp == 'T')
            {
                DBG(curJsonCtx->dbg_write, "Boolean 'true' start found @ %d\r\n", offset);

                token_top = ejson_push_token(curJsonCtx, EJSON_TOKEN_BOOLEAN_TRUE, offset);

                curVal = (struct ejson_val *)ejson_malloc(ctx, sizeof(struct ejson_val));
                curVal->type = ESJON_VAL_TYPE_BOOLEAN;

                /* Add value to list */
                switch (curJsonCtx->obj_type)
                {
                case EJSON_OBJ_TYPE_JSON:
                    curKeyVal->val = curVal;
                    break;
                case EJSON_OBJ_TYPE_ARRAY:
                    ejson_val_list_add(curJsonCtx, &curJsonCtx->list_vals, curVal);
                    break;
                default:
                    break;
                }

                temp_str = &curJsonCtx->spad->spad[curJsonCtx->spad->spad_used];

                /* Convert capital character to lower case */
                if (temp >= 'A' && temp <= 'Z')
                    temp = temp - 'A' + 'a';

                temp_str[0] = temp;
                temp_str_len = 1;
                break;
            }
            else if (temp == 'f' || temp == 'F')
            {
                DBG(curJsonCtx->dbg_write, "Boolean 'false' start found @ %d\r\n", offset);

                token_top = ejson_push_token(curJsonCtx, EJSON_TOKEN_BOOLEAN_FALSE, offset);

                curVal = (struct ejson_val *)ejson_malloc(ctx, sizeof(struct ejson_val));
                curVal->type = ESJON_VAL_TYPE_BOOLEAN;

                /* Add value to list */
                switch (curJsonCtx->obj_type)
                {
                case EJSON_OBJ_TYPE_JSON:
                    curKeyVal->val = curVal;
                    break;
                case EJSON_OBJ_TYPE_ARRAY:
                    ejson_val_list_add(curJsonCtx, &curJsonCtx->list_vals, curVal);
                    break;
                default:
                    break;
                }

                temp_str = &curJsonCtx->spad->spad[curJsonCtx->spad->spad_used];

                /* Convert capital character to lower case */
                if (temp >= 'A' && temp <= 'Z')
                    temp = temp - 'A' + 'a';

                temp_str[0] = temp;
                temp_str_len = 1;
                break;
            }
            else if (temp == 'n' || temp == 'N')
            {
                DBG(curJsonCtx->dbg_write, "'null' start found @ %d\r\n", offset);

                token_top = ejson_push_token(curJsonCtx, EJSON_TOKEN_NULL_OBJ, offset);

                curVal = (struct ejson_val *)ejson_malloc(ctx, sizeof(struct ejson_val));
                curVal->type = EJSON_VAL_TYPE_NULL_OBJ;

                /* Add value to list */
                switch (curJsonCtx->obj_type)
                {
                case EJSON_OBJ_TYPE_JSON:
                    curKeyVal->val = curVal;
                    break;
                case EJSON_OBJ_TYPE_ARRAY:
                    ejson_val_list_add(curJsonCtx, &curJsonCtx->list_vals, curVal);
                    break;
                default:
                    break;
                }

                temp_str = &curJsonCtx->spad->spad[curJsonCtx->spad->spad_used];

                /* Convert capital character to lower case */
                if (temp >= 'A' && temp <= 'Z')
                    temp = temp - 'A' + 'a';

                temp_str[0] = temp;
                temp_str_len = 1;
                break;
            }
            else if (temp == '[')
            {
                DBG(curJsonCtx->dbg_write, "ARR start found @ %d\r\n", offset);

                curVal = (struct ejson_val *)ejson_malloc(ctx, sizeof(struct ejson_val));
                curVal->type = EJSON_VAL_TYPE_OBJ;

                /* Add value to list */
                switch (curJsonCtx->obj_type)
                {
                case EJSON_OBJ_TYPE_JSON:
                    curKeyVal->val = curVal;
                    break;
                case EJSON_OBJ_TYPE_ARRAY:
                    ejson_val_list_add(curJsonCtx, &curJsonCtx->list_vals, curVal);
                    break;
                default:
                    break;
                }

                /* Allocate new JSON ctx */
                parJsonCtx = curJsonCtx;
                curJsonCtx = (struct ejson_internal_ctx *)ejson_malloc(ctx, sizeof(struct ejson_ctx));

                ejson_init_internal_ctx(curJsonCtx, parJsonCtx->spad, parJsonCtx, curKeyVal, ctx->dbg_write);
                curJsonCtx->obj_type = EJSON_OBJ_TYPE_ARRAY;

                ejson_val_list_init(&curJsonCtx->list_vals);

                curVal->val.ctx = (struct ejson_ctx *)curJsonCtx;

                token_top = ejson_push_token(curJsonCtx, EJSON_TOKEN_ARRAY, offset);
                token_top = ejson_push_token(curJsonCtx, EJSON_TOKEN_VAL, offset);

                curKeyVal = NULL;
                curVal = NULL;

                break;
            }
            else if (temp == '{')
            {
                DBG(curJsonCtx->dbg_write, "JSON start found @ %d\r\n", offset);

                curVal = (struct ejson_val *)ejson_malloc(ctx, sizeof(struct ejson_val));
                curVal->type = EJSON_VAL_TYPE_OBJ;

                /* Add value to list */
                switch (curJsonCtx->obj_type)
                {
                case EJSON_OBJ_TYPE_JSON:
                    curKeyVal->val = curVal;
                    break;
                case EJSON_OBJ_TYPE_ARRAY:
                    ejson_val_list_add(curJsonCtx, &curJsonCtx->list_vals, curVal);
                    break;
                default:
                    break;
                }

                /* Allocate new JSON ctx */
                parJsonCtx = curJsonCtx;
                curJsonCtx = (struct ejson_internal_ctx *)ejson_malloc(ctx, sizeof(struct ejson_ctx));

                ejson_init_internal_ctx(curJsonCtx, parJsonCtx->spad, parJsonCtx, curKeyVal, ctx->dbg_write);
                curJsonCtx->obj_type = EJSON_OBJ_TYPE_JSON;

                curVal->val.ctx = (struct ejson_ctx *)curJsonCtx;

                token_top = ejson_push_token(curJsonCtx, EJSON_TOKEN_JSON, offset);

                curKeyVal = NULL;
                break;
            }
            else if (temp == ']')
            {
                DBG(curJsonCtx->dbg_write, "ARR end found @ %d\r\n", offset);
                /* POP VAL INSIDE AN ARRAY */
                token_top = ejson_pop_token(curJsonCtx, offset);

                /* POP  AN ARRAY ELEMENT */
                token_top = ejson_pop_token(curJsonCtx, offset);

                if (curJsonCtx->parent_ctx == NULL)
                {
                    /* Pased JSON successfully */
                    INFO(curJsonCtx->dbg_write, "End of root JSON\r\n");
                    INFO(curJsonCtx->dbg_write, "Total scratchpad used: %d Bytes\r\n", ctx->spad->spad_used);
                    return 0;
                }

                curKeyVal = curJsonCtx->parent_keyval;
                curJsonCtx = curJsonCtx->parent_ctx;
                token_top = &curJsonCtx->token_stack[curJsonCtx->stacktop];

                /* POP VAL ELEMENT INSODE KEYVAL */
                token_top = ejson_pop_token(curJsonCtx, offset);

                break;
            }
            else if (temp == '}')
            {
                DBG(curJsonCtx->dbg_write, "JSON end found @ %d\r\n", offset);
                if (curJsonCtx->parent_ctx == NULL)
                {
                    /* Pased JSON successfully */
                    INFO(curJsonCtx->dbg_write, "End of root JSON\r\n");
                    INFO(curJsonCtx->dbg_write, "Total scratchpad used: %d Bytes\r\n", ctx->spad->spad_used);
                    return 0;
                }

                curKeyVal = curJsonCtx->parent_keyval;
                curJsonCtx = curJsonCtx->parent_ctx;
                token_top = &curJsonCtx->token_stack[curJsonCtx->stacktop];

                break;
            }

            /* Malformed json */
            ERRORLN(curJsonCtx->dbg_write, "Invalid token @ %d, expected '\"' or '[' or a number\r\n", offset)
            return -30;

            break;

        case EJSON_TOKEN_ARRAY:

            if (temp == ' ' || temp == '\r' || temp == '\n' || temp == '\t')
                break;
            if (temp != ']' && temp != ',')
            {
                /* Malformed json */
                ERRORLN(curJsonCtx->dbg_write, "Invalid token @ %d, expected ']'\r\n", offset)
                return -30;
            }

            if (temp == ',')
            {
                DBG(curJsonCtx->dbg_write, "Val element end found @ %d\r\n", offset);
                token_top = ejson_push_token(curJsonCtx, EJSON_TOKEN_VAL, offset);
                break;
            }
            else if (temp == ']')
            {
                /* POP ARRAY TOKEN */
                token_top = ejson_pop_token(curJsonCtx, offset);

                if (curJsonCtx->parent_ctx == NULL)
                {
                    /* Pased JSON successfully */
                    INFO(curJsonCtx->dbg_write, "End of root JSON\r\n");
                    INFO(curJsonCtx->dbg_write, "Total scratchpad used: %d Bytes\r\n", ctx->spad->spad_used);
                    return 0;
                }

                curKeyVal = curJsonCtx->parent_keyval;
                curJsonCtx = curJsonCtx->parent_ctx;
                token_top = &curJsonCtx->token_stack[curJsonCtx->stacktop];

                /* POP VAL ELEMENT INSODE KEYVAL */
                token_top = ejson_pop_token(curJsonCtx, offset);

                break;
            }

            break;

        case EJSON_TOKEN_STRING:
            if (isEscaped == 1)
            {
                isEscaped = 0;

                if (temp == 'n')
                    temp = '\n';
                else if (temp == 'r')
                    temp = '\r';
                else if (temp == 't')
                    temp = '\t';
                else if (temp == '\b')
                    temp = '\b';

                /* Add character to Key */
                curVal->val.str_val.str[curVal->val.str_val.len++] = temp;
                ctx->spad->spad_used++;
                break;
            }
            else
            {
                if (temp == '\\')
                {
                    isEscaped = 1;
                    break;
                }
            }

            if (temp == '"')
            {
                DBG(curJsonCtx->dbg_write, "String end found @ %d\r\n", offset);

                curVal->val.str_val.str[curVal->val.str_val.len] = '\0';

                /* POP STRING TOKEN */
                token_top = ejson_pop_token(curJsonCtx, offset);

                /* POP VAL TOKEN */
                token_top = ejson_pop_token(curJsonCtx, offset);
                break;
            }

            /* Add character to Key */
            curVal->val.str_val.str[curVal->val.str_val.len++] = temp;
            ctx->spad->spad_used++;
            break;
        case EJSON_TOKEN_BOOLEAN_TRUE:
            /* Convert capital character to lower case */
            if (temp >= 'A' && temp <= 'Z')
                temp = temp - 'A' + 'a';

            if (!((temp == 'r') || (temp == 'u') || (temp == 'e')))
            {
                DBG(curJsonCtx->dbg_write, "Boolean 'true' end found @ %d\r\n", offset);

                temp_str[temp_str_len] = '\0';

                if (strncmp((char *)temp_str, "true", 4) != 0)
                {
                    ERRORLN(curJsonCtx->dbg_write, "Unable to parse boolean 'true' value @ %d", offset);

                    /* Value ERROR */
                    return -20;
                }

                curVal->val.booleanVal = 1;

                /* POP STRING TOKEN */
                token_top = ejson_pop_token(curJsonCtx, offset);

                /* POP VAL TOKEN */
                token_top = ejson_pop_token(curJsonCtx, offset);

                /* This is done to check if this character is the ','
                 * which should be treated as end of KeyVal pair.
                 */
                offset--;
                break;
            }

            /* Add character to temp buffer */
            temp_str[temp_str_len++] = temp;

            break;
        case EJSON_TOKEN_BOOLEAN_FALSE:
            /* Convert capital character to lower case */
            if (temp >= 'A' && temp <= 'Z')
                temp = temp - 'A' + 'a';

            if (!((temp == 'a') || (temp == 'l') || (temp == 's') || (temp == 'e')))
            {
                DBG(curJsonCtx->dbg_write, "Boolean 'false' end found @ %d\r\n", offset);

                temp_str[temp_str_len] = '\0';

                if (strncmp((char *)temp_str, "false", 5) != 0)
                {
                    ERRORLN(curJsonCtx->dbg_write, "Unable to parse boolean 'false' value @ %d", offset);

                    /* Value ERROR */
                    return -20;
                }

                curVal->val.booleanVal = 0;

                /* POP STRING TOKEN */
                token_top = ejson_pop_token(curJsonCtx, offset);

                /* POP VAL TOKEN */
                token_top = ejson_pop_token(curJsonCtx, offset);

                /* This is done to check if this character is the ','
                 * which should be treated as end of KeyVal pair.
                 */
                offset--;

                break;
            }

            /* Add character to temp buffer */
            temp_str[temp_str_len++] = temp;
            break;
        case EJSON_TOKEN_NULL_OBJ:
            /* Convert capital character to lower case */
            if (temp >= 'A' && temp <= 'Z')
                temp = temp - 'A' + 'a';

            if (!((temp == 'u') || (temp == 'l')))
            {
                DBG(curJsonCtx->dbg_write, "'null' end found @ %d\r\n", offset);

                temp_str[temp_str_len] = '\0';

                if (strncmp((char *)temp_str, "null", 4) != 0)
                {
                    ERRORLN(curJsonCtx->dbg_write, "Unable to parse 'null' value @ %d", offset);

                    /* Value ERROR */
                    return -20;
                }

                /* POP STRING TOKEN */
                token_top = ejson_pop_token(curJsonCtx, offset);

                /* POP VAL TOKEN */
                token_top = ejson_pop_token(curJsonCtx, offset);

                /* This is done to check if this character is the ','
                 * which should be treated as end of KeyVal pair.
                 */
                offset--;

                break;
            }

            /* Add character to temp buffer */
            temp_str[temp_str_len++] = temp;
            break;
        case EJSON_TOKEN_NUM:
            if (!((temp >= '0' && temp <= '9') || temp == '.' || temp == 'E' || temp == 'e' || temp == '-'))
            {
                DBG(curJsonCtx->dbg_write, "Found NUM end @ %d\r\n", offset);
                temp_str[temp_str_len] = '\0';

                if (curVal->type == EJSON_VAL_TYPE_FLOAT)
                    ret = ejson_parse_float((char *)temp_str, temp_str_len, &curVal->val.f);
                else
                    ret = ejson_parse_int((char *)temp_str, temp_str_len, &curVal->val.i);

                if (ret != 0)
                {
                    ERRORLN(curJsonCtx->dbg_write, "Unable to parse numerical value @ %d", offset);

                    /* Value ERROR */
                    return -20;
                }

                /* POP NUM TOKEN */
                token_top = ejson_pop_token(curJsonCtx, offset);

                /* POP VAL TOKEN */
                token_top = ejson_pop_token(curJsonCtx, offset);

                /* This is done to check if this character is the ','
                 * which should be treated as end of KeyVal pair
                 */
                offset--;

                break;
            }
            if (temp == '.' || temp == 'E' || temp == 'e')
            {
                DBG(curJsonCtx->dbg_write, "Changing type of number to float\r\n")
                curVal->type = EJSON_VAL_TYPE_FLOAT;
            }

            temp_str[temp_str_len++] = temp;
            break;

        case EJSON_TOKEN_KEYVAL:
            if (temp == ' ' || temp == '\r' || temp == '\n' || temp == '\t')
                break;

            if (temp != ',' && temp != ']' && temp != '}')
            {
                /* Malformed json */
                ERRORLN(curJsonCtx->dbg_write, "Invalid token @ %d, expected ',' or ']' or '}'\r\n", offset)
                return -30;
            }

            /* Pop keyval */
            token_top = ejson_pop_token(curJsonCtx, offset);

            /* In case of both end of an array or end of a JSON ',' denotes end of val or key val resp. */
            if (temp == ',')
            {
                DBG(curJsonCtx->dbg_write, "Keyval-pair or val element end found @ %d\r\n", offset);

                break;
            }

            if (token_top->type == EJSON_TOKEN_ARRAY)
            {
                if (temp == ']')
                {
                    if (curJsonCtx->parent_ctx == NULL)
                    {
                        /* Pased JSON successfully */
                        INFO(curJsonCtx->dbg_write, "End of root JSON\r\n");
                        INFO(curJsonCtx->dbg_write, "Total scratchpad used: %d Bytes\r\n", ctx->spad->spad_used);
                        return 0;
                    }

                    curKeyVal = curJsonCtx->parent_keyval;
                    curJsonCtx = curJsonCtx->parent_ctx;
                    token_top = &curJsonCtx->token_stack[curJsonCtx->stacktop];

                    /* POP VAL ELEMENT INSODE KEYVAL */
                    token_top = ejson_pop_token(curJsonCtx, offset);
                }
            }
            else
            {
                if (temp == '}')
                {
                    if (curJsonCtx->parent_ctx == NULL)
                    {
                        /* Pased JSON successfully */
                        INFO(curJsonCtx->dbg_write, "End of root JSON\r\n");
                        INFO(curJsonCtx->dbg_write, "Total scratchpad used: %d Bytes\r\n", ctx->spad->spad_used);
                        return 0;
                    }
                    else
                    {
                        curKeyVal = curJsonCtx->parent_keyval;
                        curJsonCtx = curJsonCtx->parent_ctx;

                        token_top = ejson_pop_token(curJsonCtx, offset);
                    }

                    break;
                }
            }
        }

        offset++;
    }

    return -255;
}

static inline int ejson_dump_indent(char *buf, int len, int indentLvl)
{
    int i, j, offset = 0;

    for (j = 0; j < indentLvl; j++)
        for (i = 0; i < 4; i++)
            buf[offset++] = ' ';

    return offset;
}

static inline int ejson_dumps_internal(struct ejson_ctx *uctx, char *buf, int len, int indentLvl, int isPretty);

static inline int ejson_dump_val(struct ejson_val *val, char *buf, int len, int indentLvl, int isPretty)
{
    int offset = 0;

    switch (val->type)
    {
    case EJSON_VAL_TYPE_STRING:
        offset += snprintf(&buf[offset], (len - offset), "\"%s\"", val->val.str_val.str);
        break;
    case EJSON_VAL_TYPE_INT:
        offset += snprintf(&buf[offset], (len - offset), "%d", val->val.i);
        break;
    case ESJON_VAL_TYPE_BOOLEAN:
        offset += snprintf(&buf[offset], (len - offset), "%s", val->val.booleanVal ? "true" : "false");
        break;
    case EJSON_VAL_TYPE_NULL_OBJ:
        offset += snprintf(&buf[offset], (len - offset), "null");
        break;
    case EJSON_VAL_TYPE_FLOAT:
        offset += snprintf(&buf[offset], (len - offset), "%.5f", val->val.f);
        break;
    case EJSON_VAL_TYPE_OBJ:
        offset += ejson_dumps_internal(val->val.ctx, (char *)&buf[offset], (len - offset), indentLvl, isPretty);
        break;
    default:
        break;
    }

    return offset;
}

static int ejson_dumps_internal(struct ejson_ctx *uctx, char *buf, int len, int indentLvl, int isPretty)
{
    struct ejson_keyval_node *node_keyval;
    struct ejson_val_node *node_val;
    struct ejson_internal_ctx *ctx = (struct ejson_internal_ctx *)uctx;
    int k;
    int offset = 0;

    switch (ctx->obj_type)
    {
    case EJSON_OBJ_TYPE_JSON:
        /* Start of the JSON Obj */
        buf[offset++] = '{';

        if (ctx->list_keyval.num_keyval > 0)
        {
            if (isPretty)
            {
                buf[offset++] = '\r';
                buf[offset++] = '\n';
            }

            indentLvl++;

            node_keyval = ctx->list_keyval.head;
            for (k = 0; k < (ctx->list_keyval.num_keyval - 1); k++)
            {
                /* Apply Indent */
                if (isPretty)
                    offset += ejson_dump_indent(&buf[offset], (len - offset), indentLvl);

                offset += snprintf(&buf[offset], (len - offset), "\"%s\"", node_keyval->keyval->key);
                if (isPretty)
                {
                    buf[offset++] = ':';
                    buf[offset++] = ' ';
                }
                else
                    buf[offset++] = ':';
                offset += ejson_dump_val(node_keyval->keyval->val, &buf[offset], (len - offset), indentLvl, isPretty);
                // offset += snprintf(&buf[offset], (len - offset), ",\r\n");
                buf[offset++] = ',';
                if (isPretty)
                {
                    buf[offset++] = '\r';
                    buf[offset++] = '\n';
                }

                node_keyval = node_keyval->next;
            }

            /* Apply Indent */
            if (isPretty)
                offset += ejson_dump_indent(&buf[offset], (len - offset), indentLvl);

            offset += snprintf(&buf[offset], (len - offset), "\"%s\"", node_keyval->keyval->key);
            if (isPretty)
            {
                buf[offset++] = ':';
                buf[offset++] = ' ';
            }
            else
                buf[offset++] = ':';
            offset += ejson_dump_val(node_keyval->keyval->val, &buf[offset], (len - offset), indentLvl, isPretty);
            // offset += snprintf(&buf[offset], (len - offset), "\r\n");
            if (isPretty)
            {
                buf[offset++] = '\r';
                buf[offset++] = '\n';
            }
        }

        /* End of the JSON Obj */
        indentLvl--;
        if (isPretty)
            offset += ejson_dump_indent(&buf[offset], (len - offset), indentLvl);

        buf[offset++] = '}';

        break;
    case EJSON_OBJ_TYPE_ARRAY:
        /* Start of the JSON ARRAY */
        buf[offset++] = '[';

        if (ctx->list_vals.num_vals > 0)
        {
            node_val = ctx->list_vals.head;
            indentLvl++;

            if (node_val->val->type != EJSON_VAL_TYPE_OBJ && ctx->list_vals.num_vals > 1)
            {
                buf[offset++] = '\r';
                buf[offset++] = '\n';
            }
            for (k = 0; k < (ctx->list_vals.num_vals - 1); k++)
            {
                if (isPretty)
                {
                    if (node_val->val->type == EJSON_VAL_TYPE_OBJ)
                    {
                        if (ctx->list_vals.num_vals > 1)
                        {
                            if (k > 0)
                                offset += ejson_dump_indent(&buf[offset], (len - offset), indentLvl);
                            offset += ejson_dump_val(node_val->val, &buf[offset], (len - offset), indentLvl, isPretty);
                        }
                        else
                        {
                            if (k > 0)
                                offset += ejson_dump_indent(&buf[offset], (len - offset), indentLvl);
                            offset += ejson_dump_val(node_val->val, &buf[offset], (len - offset), indentLvl - 1, isPretty);
                        }
                    }
                    else
                    {
                        offset += ejson_dump_indent(&buf[offset], (len - offset), indentLvl);
                        offset += ejson_dump_val(node_val->val, &buf[offset], (len - offset), indentLvl, isPretty);
                    }
                }
                else
                {
                    offset += ejson_dump_val(node_val->val, &buf[offset], (len - offset), indentLvl, isPretty);
                }
                buf[offset++] = ',';
                if (isPretty)
                {
                    buf[offset++] = '\r';
                    buf[offset++] = '\n';
                }
                node_val = node_val->next;
            }

            /* Print last element */
            if (isPretty)
            {
                // if (node_val->val->type != EJSON_VAL_TYPE_OBJ)
                //     offset += ejson_dump_indent(&buf[offset], (len - offset), indentLvl);

                if (node_val->val->type == EJSON_VAL_TYPE_OBJ)
                {
                    if (ctx->list_vals.num_vals > 1)
                    {
                        if (k > 0)
                            offset += ejson_dump_indent(&buf[offset], (len - offset), indentLvl);
                        offset += ejson_dump_val(node_val->val, &buf[offset], (len - offset), indentLvl, isPretty);
                    }
                    else
                    {
                        if (k > 0)
                            offset += ejson_dump_indent(&buf[offset], (len - offset), indentLvl);
                        offset += ejson_dump_val(node_val->val, &buf[offset], (len - offset), indentLvl - 1, isPretty);
                    }
                }
                else
                {
                    if (ctx->list_vals.num_vals > 1)
                        offset += ejson_dump_indent(&buf[offset], (len - offset), indentLvl);
                    offset += ejson_dump_val(node_val->val, &buf[offset], (len - offset), indentLvl, isPretty);
                }
            }
            else
            {
                offset += ejson_dump_val(node_val->val, &buf[offset], (len - offset), indentLvl, isPretty);
            }

            if (isPretty)
            {
                if (ctx->parent_ctx != NULL)
                {

                    if (node_val->val->type != EJSON_VAL_TYPE_OBJ)
                    {
                        if (ctx->list_vals.num_vals > 1)
                        {
                            indentLvl--;
                            buf[offset++] = '\r';
                            buf[offset++] = '\n';

                            offset += ejson_dump_indent(&buf[offset], (len - offset), indentLvl);
                        }
                    }
                    else
                    {
                        // indentLvl--;

                        // offset += ejson_dump_indent(&buf[offset], (len - offset), indentLvl);
                    }
                }
                else
                {
                    buf[offset++] = '\r';
                    buf[offset++] = '\n';
                }
            }
        }

        /* End of the JSON ARRAY */
        buf[offset++] = ']';
        break;
    default:
        break;
    }

    return offset;
}

int ejson_dumps(struct ejson_ctx *uctx, char *buf, int len, int isPretty)
{
    int ret;
    ret = ejson_dumps_internal(uctx, buf, len, 0, isPretty);
    buf[ret] = '\0';

    return ret;
}

struct ejson_val *ejson_create_str_val(struct ejson_ctx *uctx, const char *buf, int len)
{
    struct ejson_internal_ctx *ctx = (struct ejson_internal_ctx *)uctx;

    /* Create new value */
    struct ejson_val *newVal = (struct ejson_val *)ejson_malloc(ctx, sizeof(struct ejson_val));
    newVal->type = EJSON_VAL_TYPE_STRING;

    /* Create buffer for string */
    newVal->val.str_val.str = ejson_malloc(ctx, (len + 1));

    /* Copy string value and set length */
    strncpy(newVal->val.str_val.str, buf, len);
    newVal->val.str_val.str[len] = '\0';
    newVal->val.str_val.len = len;

    return newVal;
}

struct ejson_val *ejson_create_int_val(struct ejson_ctx *uctx, int val)
{
    struct ejson_internal_ctx *ctx = (struct ejson_internal_ctx *)uctx;

    /* Create new value */
    struct ejson_val *newVal = (struct ejson_val *)ejson_malloc(ctx, sizeof(struct ejson_val));
    newVal->type = EJSON_VAL_TYPE_INT;

    /* Set value */
    newVal->val.i = val;

    return newVal;
}

struct ejson_val *ejson_create_float_val(struct ejson_ctx *uctx, float val)
{
    struct ejson_internal_ctx *ctx = (struct ejson_internal_ctx *)uctx;

    /* Create new value */
    struct ejson_val *newVal = (struct ejson_val *)ejson_malloc(ctx, sizeof(struct ejson_val));
    newVal->type = EJSON_VAL_TYPE_FLOAT;

    /* Set value */
    newVal->val.f = val;

    return newVal;
}

struct ejson_val *ejson_create_empty_json_val(struct ejson_ctx *uctx)
{
    struct ejson_internal_ctx *ctx = (struct ejson_internal_ctx *)uctx;

    /* Create new value */
    struct ejson_val *newVal = (struct ejson_val *)ejson_malloc(ctx, sizeof(struct ejson_val));
    newVal->type = EJSON_VAL_TYPE_OBJ;

    /* Create and initialize JSON ctx */
    struct ejson_internal_ctx *ctxNew = (struct ejson_internal_ctx *)ejson_malloc(ctx, sizeof(struct ejson_ctx));
    ejson_init_internal_ctx(ctxNew, ctx->spad, ctx, NULL, ctx->dbg_write);
    ctxNew->obj_type = EJSON_OBJ_TYPE_JSON;

    /* Set JSON ctx to value */
    newVal->val.ctx = (struct ejson_ctx *)ctxNew;

    return newVal;
}

struct ejson_keyval *ejson_create_keyval(struct ejson_ctx *uctx, const char *key, int key_len, int forceArray)
{
    struct ejson_internal_ctx *ctx = (struct ejson_internal_ctx *)uctx;
    struct ejson_keyval *newKeyVal = NULL;

    /* Allocate JSON keyval element */
    newKeyVal = (struct ejson_keyval *)ejson_malloc(ctx, sizeof(struct ejson_keyval));

    /* Add key val to list */
    ejson_keyval_list_add(ctx, newKeyVal);

    /* Init and set key */
    newKeyVal->key = ejson_malloc(ctx, (key_len + 1));

    /* Copy key */
    strncpy(newKeyVal->key, key, key_len);
    newKeyVal->key[key_len] = '\0';
    newKeyVal->key_len = key_len;

    newKeyVal->val = NULL;

    return newKeyVal;
}

int ejson_keyval_add_val(struct ejson_ctx *uctx, struct ejson_keyval *keyval, struct ejson_val *val)
{
    keyval->val = val;

    /* If value is of type JSON set the parent keyval */
    if (val->type == EJSON_VAL_TYPE_OBJ)
        ((struct ejson_internal_ctx *)val->val.ctx)->parent_keyval = keyval;

    return 0;
}

struct ejson_keyval *ejson_get_keyval(struct ejson_ctx *uctx, const char *key, int keylen)
{

    struct ejson_keyval_node *node;
    struct ejson_keyval *keyval = NULL;
    struct ejson_internal_ctx *ctx = (struct ejson_internal_ctx *)uctx;
    int i;

    if (ctx->obj_type != EJSON_OBJ_TYPE_JSON)
    {
        ERRORLN(ctx->dbg_write, "Object is not a JSON object");

        return NULL;
    }

    node = ctx->list_keyval.head;

    for (i = 0; i < ctx->list_keyval.num_keyval; i++)
    {
        /* Check if node is null */
        if (node == NULL)
            break;

        /* Get keyval */

        if (node->keyval->key_len == keylen)
            if (strncmp(key, node->keyval->key, node->keyval->key_len) == 0)
            {
                keyval = node->keyval;
                break;
            }

        node = node->next;
    }

    return keyval;
}

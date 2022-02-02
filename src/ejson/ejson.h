﻿#ifndef _EJSON_H_
#define _EJSON_H_

#include <stdint.h>

#ifndef DEBUG
#define DEBUG 4
#endif

struct ejson_ctx
{
    uint8_t res[6 * sizeof(void*) + sizeof(int) * 3 * 7 + 4 * sizeof(int)];
};

enum ejson_val_type
{
    EJSON_VAL_TYPE_UNK,
    EJSON_VAL_TYPE_STRING,
    ESJON_VAL_TYPE_BOOLEAN,
    EJSON_VAL_TYPE_INT,
    EJSON_VAL_TYPE_FLOAT,
    EJSON_VAL_TYPE_JSON,
    EJSON_VAL_TYPE_NULL_OBJ
};

struct ejson_val
{
    enum ejson_val_type type;
    union
    {
        uint8_t booleanVal;
        struct _1
        {
            int len;
            char *str;
        } str_val;

        float f;
        int i;

        struct ejson_ctx *ctx;
    } val;
};

struct ejson_val_node
{
    struct ejson_val *val;
    struct ejson_val_node *next;
};

struct ejson_val_list
{
    int num_vals;
    struct ejson_val_node *head;
    struct ejson_val_node *last_added;
};


struct ejson_keyval
{
    char *key;
    int key_len;
    int force_array;
    struct ejson_val_list *list_val;
};

typedef void (*debug_write_cb_t)(char *);

int ejson_init_ctx(struct ejson_ctx *uctx, uint8_t *spadbuf, int len_spadbuf, debug_write_cb_t dbg_write);

int ejson_loads(char *buf, int len, struct ejson_ctx *uctx);

int ejson_dumps(struct ejson_ctx *uctx, char *buf, int len, int isPretty);

void ejson_print_info(struct ejson_ctx *uctx, int indentLvl);

struct ejson_val * ejson_create_str_val(struct ejson_ctx *uctx, const char *buf, int len);
struct ejson_val * ejson_create_int_val(struct ejson_ctx *uctx, int val);
struct ejson_val * ejson_create_float_val(struct ejson_ctx *uctx, float val);
struct ejson_val * ejson_create_empty_json_val(struct ejson_ctx *uctx);

struct ejson_keyval * ejson_create_keyval(struct ejson_ctx *uctx, const char *key, int key_len, int forceArray);
int ejson_keyval_add_val(struct ejson_ctx *uctx, struct ejson_keyval *keyval, struct ejson_val *val);

struct ejson_keyval *ejson_get_keyval(struct ejson_ctx *uctx, const char *key, int keylen);

#endif /* _EJSON_H_ */
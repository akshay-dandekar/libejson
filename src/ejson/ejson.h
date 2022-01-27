#ifndef _EJSON_H_
#define _EJSON_H_

#include <stdint.h>

#define DEBUG 4

enum ejson_val_type
{
    EJSON_VAL_TYPE_STRING,
    EJSON_VAL_TYPE_INT,
    EJSON_VAL_TYPE_FLOAT,
    EJSON_VAL_TYPE_ARRAY,
    EJSON_VAL_TYPE_JSON
};

union ejson_val
{
    struct _1
    {
        int len;
        char *str;
    } str_val;

    struct _2
    {
        int len;
        union ejson_val *arr;
    } arr;

    float f;
    int i;
};

struct ejson_keyval
{
    char *key;
    int key_len;
    union ejson_val *val;
    int val_nbr;
    enum ejson_val_type type;
};

struct ejson_keyval_node
{
    struct ejson_keyval *keyval;
    struct ejson_keyval_node *next;
};

struct ejson_ctx
{
    int len_spad;
    uint8_t *spad;
    int spad_used;

    struct _4
    {
        int num_keyval;
        struct ejson_keyval_node *head;
        struct ejson_keyval_node *last_added;
    } list_keyval;
};

int ejson_init_ctx(struct ejson_ctx *ctx, uint8_t *spad, int len_spad);

int ejson_loads(char *buf, int len, struct ejson_ctx *ctx);

int ejson_dumps(struct ejson_ctx *ctx, char *buf, int len);

#endif /* _EJSON_H_ */
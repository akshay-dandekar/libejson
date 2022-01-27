#include <stdio.h>
#include <string.h>

#include "ejson\ejson.h"

int main(int argc, char *argv[])
{
    struct ejson_ctx ctx;
    int ret, i;
    uint8_t spad[1024];
    struct ejson_keyval_node *node;
    const char str_json[] = " {  \t\r\n\"key1\"   : \t \r\r\n\"val1\", \t\r\n\"key2\": 1.23456 \r\n\r\n,\"key3\":100,\t\r\n\"key4\"   : \t \r\r\n\"val4 akshay dandekar\"}    ";

    /* Initialize JSON context */
    ret = ejson_init_ctx(&ctx, spad, sizeof(spad));
    if (ret != 0)
    {
        fprintf(stderr, "Unable to initialize JSON context\r\n");
        return -1;
    }

    /* Parse/load JSON string */
    ret = ejson_loads((char *)str_json, strlen(str_json), &ctx);
    if (ret != 0)
    {
        fprintf(stderr, "Unable to parse JSON\r\n");
        return -1;
    }

    printf("Number of keyval pairs found: %d\r\n", ctx.list_keyval.num_keyval);

    node = ctx.list_keyval.head;
    for (i = 0; i < ctx.list_keyval.num_keyval; i++)
    {

        printf("-----------\r\n");
        printf("Key %d: %s\r\n", i, node->keyval->key);
        printf("Val type: %d\r\n", node->keyval->type);
        switch (node->keyval->type)
        {
        case EJSON_VAL_TYPE_STRING:
            printf("Val %d: %s\r\n", i, node->keyval->val->str_val.str);
            break;
        case EJSON_VAL_TYPE_INT:
            printf("Val %d: %d\r\n", i, node->keyval->val->i);
            break;
        case EJSON_VAL_TYPE_FLOAT:
            printf("Val %d: %.4f\r\n", i, node->keyval->val->f);
            break;
        default:
            break;
        }
        printf("-----------\r\n\r\n");

        node = node->next;
    }

    return 0;
}
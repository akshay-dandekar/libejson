#include <stdio.h>
#include <string.h>

#include "ejson\ejson.h"

static void print_buf(char *str)
{
    printf(str);
}

int main(int argc, char *argv[])
{
    struct ejson_ctx ctx;
    int ret;
    uint8_t spad[2048];
    char strJson[2048];
    struct ejson_keyval *keyval, *keyvalLatLong;
    struct ejson_val *val, *valLatLong;
    struct ejson_ctx *ctxLatLong;

    const char devid[] = "dev1234";

    /* Initialize JSON context */
    ret = ejson_init_ctx(&ctx, spad, sizeof(spad), print_buf);
    if (ret != 0)
    {
        fprintf(stderr, "Unable to initialize JSON context\r\n");
        return -1;
    }

    printf("Context init complete\r\n");

    /* Create a keyval token for device id */
    keyval = ejson_create_keyval(&ctx, "did", 3, 0);

    /* Create and assign device id to device id token */
    val = ejson_create_str_val(&ctx, devid, (sizeof(devid) - 1));
    ejson_keyval_add_val(&ctx, keyval, val);

    /* Create a keyval token for session id */
    keyval = ejson_create_keyval(&ctx, "sid", 3, 0);

    /* Create and assign session id to session id token */
    val = ejson_create_int_val(&ctx, 123);
    ejson_keyval_add_val(&ctx, keyval, val);

    /* Create a keyval token for latllong data */
    keyval = ejson_create_keyval(&ctx, "l", 1, 1);

    /* Create JSON value token */
    val = ejson_create_empty_json_val(&ctx);
    ejson_keyval_add_val(&ctx, keyval, val);

    ctxLatLong = val->val.ctx;

    /* Create a keyval token for lat */
    keyvalLatLong = ejson_create_keyval(ctxLatLong, "lat", 3, 0);

    /* Create and assign lat to lat token */
    valLatLong = ejson_create_float_val(ctxLatLong, 18.2323);
    ejson_keyval_add_val(ctxLatLong, keyvalLatLong, valLatLong);

    /* Create a keyval token for long */
    keyvalLatLong = ejson_create_keyval(ctxLatLong, "lat", 3, 0);

    /* Create and assign long to long token */
    valLatLong = ejson_create_float_val(ctxLatLong, 73.8999);
    ejson_keyval_add_val(ctxLatLong, keyvalLatLong, valLatLong);


    /* Create a keyval token for batt voltage */
    keyval = ejson_create_keyval(&ctx, "bv", 2, 0);

    /* Create and assign batt voltage to batt voltage token */
    val = ejson_create_float_val(&ctx, 60.23);
    ejson_keyval_add_val(&ctx, keyval, val);

    /* Create a keyval token for batt current */
    keyval = ejson_create_keyval(&ctx, "ba", 2, 0);

    /* Create and assign batt current to batt current token */
    val = ejson_create_float_val(&ctx, 10.313);
    ejson_keyval_add_val(&ctx, keyval, val);

    /* Create a keyval token for batt temperature */
    keyval = ejson_create_keyval(&ctx, "bt", 2, 0);

    /* Create and assign batt temperature to batt temperature token */
    val = ejson_create_int_val(&ctx, 26);
    ejson_keyval_add_val(&ctx, keyval, val);

    

    ret = ejson_dumps(&ctx, strJson, sizeof(strJson), 1);

    printf("Length of JSON string: %d\r\n", ret);
    printf("%s", strJson);

    return 0;
}
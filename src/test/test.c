#include <stdio.h>
#include <string.h>

#include "ejson\ejson.h"

static void print_buf(char *str)
{
    printf(str);
}

static void *align(void *ptr, int align)
{
    unsigned long long val = (unsigned long long)ptr;
    int offset;

    offset = (val % align);

    if (offset > 0)
    {
        val += 4 - offset;
    }
    return (void *)val;
}

int main(int argc, char *argv[])
{
    struct ejson_ctx ctx;
    int ret;
    uint8_t spad[2048];
    char strJson[2048];
    // const char str_json[] = " {  \t\r\n\"key1\"   : \t \r\r\n\"val1\\\\escaped\", \t\r\n\"key2\": 1.23456 \r\n\r\n,\"key3\":100,\t\r\n\"key4\"   : \t \r\r\n\"val4 \\\"akshay dandekar\\\"\"}    ";
    // const char str_json[] = "{\"key1\":\"val1\",\"key2\":{\"nestedkey1\":\"nestedval1\",\"nestedkey2\":+1e-3}}";
    // const char str_json[] = "{\"key1\":[\"hello\",\"world\",34.5454,3333],\"key2\":[{\"nestkey1\":\"nestval1\", \"nestkey3\":\"nestval3\"},{\"nestkey2\":\"nestval2\"},1.234,23],\"key3\":{\"nestedkey4\":\"nestedval4\"},\"key4\":[1.234]}";
    // const char str_json[] = "{\"key1\":{\"key2\":\"val2\"}}";
    const char str_json[] = "{\"key1\":\"val1\",\"key2\":\"val2\",\"emptyarray\":[true,false,null],\"boolkey1\":true,\"boolkey2\":false,\"nullkey\":nulL}";

    uint8_t *ptr = (uint8_t *)0x123FFUL;
    printf("Orig ptr: %p", ptr);
    ptr = align((void *)ptr, 4);
    printf("Aligned ptr: %p", ptr);

    /* Initialize JSON context */
    ret = ejson_init_ctx(&ctx, spad, sizeof(spad), print_buf);
    if (ret != 0)
    {
        fprintf(stderr, "Unable to initialize JSON context\r\n");
        return -1;
    }

    /* Parse/load JSON string */
    ret = ejson_loads((char *)str_json, strlen(str_json), &ctx);
    if (ret != 0)
    {
        fprintf(stderr, "Unable to parse JSON: %d\r\n", ret);
        return -1;
    }

    // ejson_print_info(&ctx, 0);

    ret = ejson_dumps(&ctx, strJson, sizeof(strJson), 1);

    printf("Length of JSON string: %d\r\n", ret);
    printf("%s\r\n", strJson);

    struct ejson_keyval *kv = ejson_get_keyval(&ctx, "boolkey1", 8);
    if (kv == NULL)
    {
        printf("\"boolkey1\" key not found\r\n");
        return -1;
    }

    printf("boolkey1 is %s\r\n", kv->list_val->last_added->val->val.booleanVal ? "true" : "false");

    return 0;
}
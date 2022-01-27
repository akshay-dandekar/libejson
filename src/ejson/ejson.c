#include <string.h>

#include "ejson.h"

#if DEBUG > 3
#include <stdio.h>

#define DBG(FORMAT, ...)               \
    {                                  \
        printf(FORMAT, ##__VA_ARGS__); \
    }
#else
#define DBG(FORMAT, ...) ;
#endif

void ejson_keyval_list_init(struct ejson_ctx *ctx)
{
    ctx->list_keyval.num_keyval = 0;
    ctx->list_keyval.head = NULL;
    ctx->list_keyval.last_added = NULL;
}

void ejson_keyval_list_add(struct ejson_ctx *ctx, struct ejson_keyval *keyval)
{
    struct ejson_keyval_node *new = (struct ejson_keyval_node *)&ctx->spad[ctx->spad_used];
    ctx->spad_used += sizeof(struct ejson_keyval_node);

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

int ejson_init_ctx(struct ejson_ctx *ctx, uint8_t *spad, int len_spad)
{
    if (ctx == NULL)
        return -1;

    ctx->spad = spad;
    ctx->len_spad = len_spad;
    ctx->spad_used = 0;

    ejson_keyval_list_init(ctx);

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

enum ejson_val_parser_state
{
    EJSON_VAL_PARSER_FIND_TYPE,
    EJSON_VAL_PARSER_FOUND_TYPE,
    EJSON_VAL_PARSER_FIND_STR_START,
    EJSON_VAL_PARSER_FOUND_STR_START,
    EJSON_VAL_PARSER_FIND_STR_END,
    EJSON_VAL_PARSER_FOUND_STR_END,
    EJSON_VAL_PARSER_FIND_ARRAY_START,
    EJSON_VAL_PARSER_FOUND_ARRAY_START,
    EJSON_VAL_PARSER_FIND_ARRAY_END,
    EJSON_VAL_PARSER_FOUND_ARRAY_END,
    EJSON_VAL_PARSER_FIND_NUM_START,
    EJSON_VAL_PARSER_FOUND_NUM_START,
    EJSON_VAL_PARSER_FIND_NUM_END,
    EJSON_VAL_PARSER_FOUND_NUM_END,

};

int ejson_parse_array(int lvl, uint8_t *buf, int len, struct ejson_keyval *keyval, uint8_t *spad, int spad_len, int *spad_used)
{

}

int ejson_parse_val(int lvl, uint8_t *buf, int len, struct ejson_keyval *keyval, uint8_t *spad, int spad_len, int *spad_used)
{
    char temp;
    int offset = 0;
    int offset_val;
    int val_start = 0, val_len;
    int ret;
    int _spad_usage = 0;

    enum ejson_val_parser_state state = EJSON_VAL_PARSER_FIND_TYPE;

    if (lvl > 2)
        return -30;

    while ((offset < len) && *buf != '\0')
    {
        temp = *buf;

        switch (state)
        {
        case EJSON_VAL_PARSER_FIND_TYPE:
            if (temp == ' ' || temp == '\r' || temp == '\n' || temp == '\t')
                break;

            else if (temp == '"')
            {
                keyval->type = EJSON_VAL_TYPE_STRING;
                offset_val = 0;

                keyval->val = (union ejson_val *)&spad[_spad_usage];
                _spad_usage += sizeof(union ejson_val);

                keyval->val->str_val.str = &spad[_spad_usage];

                state = EJSON_VAL_PARSER_FIND_STR_START;
                state = EJSON_VAL_PARSER_FOUND_STR_START;
                state = EJSON_VAL_PARSER_FIND_STR_END;
                break;
            }
            else if ((temp >= '0' && temp <= '9') || temp == '+' || temp == '-')
            {
                DBG("Found NUM start @ %d\r\n", offset);

                keyval->type = EJSON_VAL_TYPE_INT;
                val_start = offset;
                offset_val = 0;

                keyval->val = (union ejson_val *)&spad[_spad_usage];
                _spad_usage += sizeof(union ejson_val);

                keyval->val->str_val.str = &spad[_spad_usage];

                keyval->val->str_val.str[offset_val++] = temp;
                _spad_usage++;

                state = EJSON_VAL_PARSER_FIND_NUM_START;
                state = EJSON_VAL_PARSER_FOUND_NUM_START;
                state = EJSON_VAL_PARSER_FIND_NUM_END;

                break;
            }
            else if (temp == '[')
            {

                keyval->type = EJSON_VAL_TYPE_ARRAY;
                offset_val = 0;

                keyval->val = (union ejson_val *)&spad[_spad_usage];
                _spad_usage += sizeof(union ejson_val);

                keyval->val->str_val.str = &spad[_spad_usage];

                state = EJSON_VAL_PARSER_FIND_STR_START;
                state = EJSON_VAL_PARSER_FOUND_STR_START;
                state = EJSON_VAL_PARSER_FIND_STR_END;
                break;
            }
            break;

        case EJSON_VAL_PARSER_FIND_STR_END:
            if (temp == '"')
            {
                DBG("Found str end @ %d\r\n", offset);
                keyval->val->str_val.len = offset_val;
                keyval->val->str_val.str[offset_val] = '\0';
                _spad_usage++;

                *spad_used = _spad_usage;

                return 0;
            }

            keyval->val->str_val.str[offset_val++] = temp;
            _spad_usage++;
            break;

        case EJSON_VAL_PARSER_FIND_NUM_END:
            if (!((temp >= '0' && temp <= '9') || temp == '.'))
            {
                keyval->val->str_val.str[offset_val] = '\0';
                DBG("Found NUM end @ %d\r\n", offset);

                if (keyval->type == EJSON_VAL_TYPE_FLOAT)
                    ret = ejson_parse_float(keyval->val->str_val.str, offset_val, &keyval->val->f);
                else
                    ret = ejson_parse_int(keyval->val->str_val.str, offset_val, &keyval->val->i);

                *spad_used = _spad_usage - offset_val;

                return ret;
            }
            if (temp == '.')
            {
                DBG("Changing type of number to float\r\n")
                keyval->type = EJSON_VAL_TYPE_FLOAT;
            }

            keyval->val->str_val.str[offset_val++] = temp;
            _spad_usage++;

        default:
            break;
        }

        buf++;
        offset++;
    }

    if (state == EJSON_VAL_PARSER_FIND_NUM_END)
    {
        keyval->val->str_val.str[offset_val] = '\0';
        DBG("Found NUM end @ %d\r\n", offset);

        if (keyval->type == EJSON_VAL_TYPE_FLOAT)
            ret = ejson_parse_float(keyval->val->str_val.str, offset_val, &keyval->val->f);
        else
            ret = ejson_parse_int(keyval->val->str_val.str, offset_val, &keyval->val->i);

        *spad_used = _spad_usage - offset_val;

        return ret;
    }

    return -50;
}

enum ejson_parser_state
{
    EJSON_PARSER_STATE_FIND_JSON_START,
    EJSON_PARSER_STATE_FOUND_JSON_START,
    EJSON_PARSER_STATE_FIND_JSON_END,
    EJSON_PARSER_STATE_FOUND_JSON_END,
    EJSON_PARSER_STATE_FIND_KEY_START,
    EJSON_PARSER_STATE_FOUND_KEY_START,
    EJSON_PARSER_STATE_FIND_KEY_END,
    EJSON_PARSER_STATE_FOUND_KEY_END,
    EJSON_PARSER_STATE_FIND_SEP,
    EJSON_PARSER_STATE_FOUND_SEP,
    EJSON_PARSER_STATE_FIND_VAL_START,
    EJSON_PARSER_STATE_FOUND_VAL_START,
    EJSON_PARSER_STATE_FIND_VAL_END,
    EJSON_PARSER_STATE_FOUND_VAL_END,
    EJSON_PARSER_STATE_FIND_COMMA,
    EJSON_PARSER_STATE_FOUND_COMMA,
};

int ejson_loads(char *buf, int len, struct ejson_ctx *ctx)
{
    enum ejson_parser_state state = EJSON_PARSER_STATE_FIND_JSON_START;
    int offset = 0;
    int offset_key = 0, offset_val_start = 0, val_len = 0;
    uint8_t temp;
    int ret, spad_used = 0;
    struct ejson_keyval *keyval;

    /* Set number of found keyvals to 0 */
    ejson_keyval_list_init(ctx);
    ctx->spad_used = 0;

    while ((offset < len) && buf[offset] != '\0')
    {
        temp = buf[offset++];

        switch (state)
        {
        case EJSON_PARSER_STATE_FIND_JSON_START:
            if (temp == ' ' || temp == '\r' || temp == '\n' || temp == '\t')
                break;

            if (temp == '{')
            {
                DBG("JSON start found\r\n");
                state = EJSON_PARSER_STATE_FOUND_JSON_START;
                state = EJSON_PARSER_STATE_FIND_KEY_START;
            }
            else
                /* Invalid JSON string */
                return -10;
            break;
        case EJSON_PARSER_STATE_FIND_KEY_START:
            if (temp == ' ' || temp == '\r' || temp == '\n' || temp == '\t')
                break;
            if (temp == '"')
            {

                DBG("JSON key start found @ %d\r\n", offset);
                offset_key = 0;

                keyval = (struct ejson_keyval *)&ctx->spad[ctx->spad_used];
                ctx->spad_used += sizeof(struct ejson_keyval);

                keyval->key_len = 0;
                keyval->key = (char *)&ctx->spad[ctx->spad_used++];
                *keyval->key = '\0';

                state = EJSON_PARSER_STATE_FOUND_KEY_START;
                state = EJSON_PARSER_STATE_FIND_KEY_END;
                break;
            }
            if (temp == '}')
            {
                DBG("JSON end found @ %d\r\n", offset);
                DBG("Total spad used: %d\r\n", ctx->spad_used);

                state = EJSON_PARSER_STATE_FOUND_JSON_END;
                return 0;
            }
            break;
        case EJSON_PARSER_STATE_FIND_KEY_END:
            if (temp == '"')
            {

                DBG("JSON key end found @ %d\r\n", offset);
                keyval->key[offset_key] = '\0';
                ctx->spad_used++;

                state = EJSON_PARSER_STATE_FOUND_KEY_END;
                state = EJSON_PARSER_STATE_FIND_SEP;
                break;
            }
            if (offset_key == 63)
                continue;

            keyval->key[offset_key++] = temp;
            ctx->spad_used++;
            break;

        case EJSON_PARSER_STATE_FIND_SEP:
            if (temp == ' ' || temp == '\r' || temp == '\n' || temp == '\t')
                break;

            if (temp != ':')
                return -11;

            DBG("KeyVal SEP found @ %d\r\n", offset);

            offset_val_start = offset;
            val_len = 0;

            state = EJSON_PARSER_STATE_FOUND_SEP;
            state = EJSON_PARSER_STATE_FOUND_VAL_START;

            break;

        case EJSON_PARSER_STATE_FOUND_VAL_START:

            if (temp == '}')
            {
                DBG("JSON val end found @ %d\r\n", offset - 1);

                ret = ejson_parse_val(0, &buf[offset_val_start], val_len, keyval, &ctx->spad[ctx->spad_used], ctx->len_spad - ctx->spad_used, &spad_used);
                if (ret != 0)
                {
                    DBG("Unable to parse value for key %s", keyval->key);
                    return ret;
                }

                ctx->spad_used += spad_used;

                DBG("JSON end found @ %d\r\n", offset);
                DBG("Total spad used: %d\r\n", ctx->spad_used);

                state = EJSON_PARSER_STATE_FOUND_JSON_END;

                ejson_keyval_list_add(ctx, keyval);

                return 0;
            }

            if (temp == ',')
            {
                DBG("JSON val end found @ %d\r\n", offset);

                ret = ejson_parse_val(0, &buf[offset_val_start], val_len, keyval, &ctx->spad[ctx->spad_used], ctx->len_spad - ctx->spad_used, &spad_used);

                if (ret != 0)
                {
                    DBG("Unable to parse value for key %s", keyval->key);
                    return ret;
                }

                ctx->spad_used += spad_used;

                ejson_keyval_list_add(ctx, keyval);

                state = EJSON_PARSER_STATE_FOUND_VAL_END;
                state = EJSON_PARSER_STATE_FIND_COMMA;
                state = EJSON_PARSER_STATE_FOUND_COMMA;
                state = EJSON_PARSER_STATE_FIND_KEY_START;
                break;
            }

            val_len++;

            break;
        default:
            break;
        }
    }

    return -255;
}

int ejson_dumps(struct ejson_ctx *ctx, char *buf, int len)
{
}
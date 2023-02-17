#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define bail_assert(s)          \
    do {                        \
        if (!(s)) goto exit;    \
    } while (0)

#define bail_assert_wm(s, fmt, ...)                 \
    do {                                            \
        if (!(s)) {                                 \
            fprintf(stderr, fmt, ##__VA_ARGS__);    \
            goto exit;                              \
        }                                           \
    } while (0)

typedef int bool;
#define true 1
#define false 0

typedef struct _mfloat {
    union {
        int ivalue;
        float fvalue;
    };
} mfloat;

#define STACK_GROW 1024 * sizeof(void *)
typedef struct _stack
{
    int index;
    size_t size;
    int *data;
} stack, *stackp;

stackp stack_create(void)
{
    stackp sp;

    sp = (stackp)calloc(1, sizeof(stack) + STACK_GROW);
    bail_assert_wm(sp, "memory allocation");
    
    sp->index = -1;
    sp->size = STACK_GROW;
    sp->data = (int *)sp + sizeof(*sp);
    return sp;
    
exit:
    if (sp) {
        free(sp);
    }
    return NULL;
}

int stack_push(stackp sp, int data)
{
    int ret = 1;

    bail_assert_wm(sp, "arg: sp");

    if (sp->index + sizeof(void *) >= sp->size) {
        void *t;

        t = realloc(sp, sp->size + STACK_GROW);
        bail_assert_wm(t, "memory allocation");

        sp = t;
    }

    sp->data[++sp->index] = data;
    ret = 0;

exit:
    return ret;
}

bool stack_pop(stackp sp, int *data)
{
    bail_assert_wm(sp, "arg: sp");
    bail_assert_wm(sp->index > -1, "arg: sp->index > -1");

    *data = sp->data[sp->index--];
    return true;

exit:
    return false;
}

bool stack_peek(stackp sp, int *data)
{
    bail_assert_wm(sp, "arg: sp");
    bail_assert(sp->index > -1);

    *data = sp->data[sp->index];
    return true;

exit:
    return false;
}

int stack_count(stackp sp)
{
    bail_assert_wm(sp, "arg: sp");

    return sp->index + 1;

exit:
    return -1;
}

void stack_free(stackp sp)
{
    bail_assert(sp);

exit:
    if (sp) {
        free(sp);
    }
}

char rpn_bmp[] = 
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x01\x00\x00\x00\x00\x00\x00\x00\xFE\xFF\x11\x10\x00\x10\x08\x11"
    "\x08\x08\x08\x08\x08\x08\x08\x08\x08\x08\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x12\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
#define rpn_is_ok(op) \
    (rpn_bmp[op & 0xFF] != 0x00)
#define rpn_is_op(op) \
    ((rpn_bmp[op & 0xFF] & 0x7F) > 0x08)
#define rpn_is_parsable(c) \
    (rpn_bmp[c & 0xFF] == 0x08)
#define rpn_need_skip(c) \
    (rpn_bmp[c & 0xFF] == 0x01)
#define rpn_is_parent_o(c) \
    (rpn_bmp[c & 0xFF] == (char)(-2))
#define rpn_is_parent_c(c) \
    (rpn_bmp[c & 0xFF] == (char)(-1))
#define rpn_need_push(c) \
    (rpn_is_parent_o(c))

size_t rpn_count_token(const char *infix)
{
    size_t ret = 0;

    bail_assert_wm(infix, "arg: infix");

    for (; *infix; ++infix) {
        if (rpn_is_ok(*infix)) {
            ret++;
        }
    }

exit:
    return ret;
}

const char *rpn_parse_number(const char *infix, mfloat *number)
{
    int i;
    char parse[256] = { '\0' };

    bail_assert_wm(infix, "arg: infix");
    bail_assert_wm(number, "arg: number");

    for (i = 0; *infix; ++infix) {
        if (!rpn_is_parsable(*infix)) {
            break;
        }
        
        parse[i++] = *infix;
    }

    number->fvalue = atof(parse);
    return infix - 1;

exit:
    return NULL;
}

char *rpn_infix2postfix(const char *infix)
{
    stackp sp = NULL;
    mfloat number;
    char *ret = NULL;
    char *postfix = NULL;
    char *ppostfix = NULL;
    const char *pinfix = NULL;

    int op;

    sp = stack_create();
    bail_assert(sp);

    // ATT
    // * 8
    // we are putting integers as floats .2f, so,
    printf("rpn_count_token: %zu\n", rpn_count_token(infix));
    postfix = calloc(1, rpn_count_token(infix) * 8);
    bail_assert_wm(postfix, "memory allocation");

    for (pinfix = infix, ppostfix = postfix; *pinfix; ++pinfix) {
        if (rpn_need_skip(*pinfix)) {
            continue;
        }

        bail_assert_wm(rpn_is_ok(*infix), "parse error. char: %c hex: %X at %d\n", *pinfix, *pinfix, (int)(pinfix - infix));
        
        if (!rpn_is_op(*pinfix)) {
            pinfix = rpn_parse_number(pinfix, &number);
            bail_assert(pinfix);

            ppostfix += sprintf(ppostfix, "%.2f ", number.fvalue);
            continue;
        }

        if (rpn_need_push(*pinfix)) {
            stack_push(sp, *pinfix);
            continue;
        }

        while ( stack_peek(sp, &op) &&
                rpn_bmp[op] >= rpn_bmp[*pinfix]) {
            stack_pop(sp, &op);
            ppostfix += sprintf(ppostfix, "%c ", op);
        }
        stack_push(sp, *pinfix);
    }

    while (stack_count(sp) > 0) {
        stack_pop(sp, &op);
        ppostfix += sprintf(ppostfix, "%c ", op);
    }

    ret = postfix;
    postfix = NULL;

exit:
    if (sp) {
        stack_free(sp);
    }
    if (postfix) {
        free(postfix);
    }
    return ret;
}


mfloat rpn_calculate(char *postfix)
{
    mfloat ret = {
        .fvalue = -1
    };
    stackp sp = NULL;
    char *token = NULL;
    char *rest = postfix;

    int parentheses = 0;
    
    mfloat val0;
    mfloat val1;
    mfloat val;

    sp = stack_create();
    bail_assert(sp);
 
    while ((token = strtok_r(rest, " ", &rest))) {
        if (!rpn_is_op(*token)) {
            mfloat mf;

            mf.fvalue = (float)atof(token);
            stack_push(sp, mf.ivalue);
            continue;
        }
        if (rpn_is_parent_o(*token)) {
            parentheses++;
            continue;
        }
        if (rpn_is_parent_c(*token)) {
            parentheses--;
            continue;
        }
        bail_assert_wm(stack_count(sp) > 1, "calculation error. token %c\n", *token);

        stack_pop(sp, &(val1.ivalue));
        stack_pop(sp, &(val0.ivalue));
        printf("%c %.2f %.2f\n", *token, val0.fvalue, val1.fvalue);
        switch (*token)
        {
        case '+':
            val.fvalue = val0.fvalue + val1.fvalue;
            stack_push(sp, val.ivalue);
            break;
        case '-':
            val.fvalue = val0.fvalue - val1.fvalue;
            stack_push(sp, val.ivalue);
            break;
        case '*':
            val.fvalue = val0.fvalue * val1.fvalue;
            stack_push(sp, val.ivalue);
            break;
        case '/':
            val.fvalue = val0.fvalue / val1.fvalue;
            stack_push(sp, val.ivalue);
            break;
        case '^':
            val.fvalue = pow(val0.fvalue, val1.fvalue);
            stack_push(sp, val.ivalue);
            break;
        
        default:
            break;
        }
    }

    bail_assert_wm(parentheses == 0, "parentheses should be 0\n");
    bail_assert_wm(stack_count(sp) == 1, "calculation error. stack size %d\n", stack_count(sp));
    stack_pop(sp, &ret.ivalue);

exit:
    if (sp) {
        stack_free(sp);
    }
    return ret;
}

int main(int argc, char *argv[])
{
    mfloat result;
    char *postfix;

    bail_assert_wm(argc > 1, "argv error");

    postfix = rpn_infix2postfix(argv[1]);
    printf("postfix: %s\n", postfix);

    result = rpn_calculate(postfix);
    printf("= %.2f\n", result.fvalue);
    
exit:
    return 0;
}

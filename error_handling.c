#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

// Possible Lisp value types
enum
{
    LVAL_NUM,
    LVAL_ERR
};

// Possible Error types
enum
{
    LERR_DIV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM
};

// Represent Lisp value (number or error)
typedef struct
{
    int type; // tell which field is meaningful to access
    union
    {
        long num;
        int err;
    };
} lval;

// Create Lisp number
lval lval_num(long x)
{
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

// Create Lisp error
lval lval_err(int x)
{
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}

// Print a Lisp error
void lerr_print(int err)
{
    switch (err)
    {
    case LERR_DIV_ZERO:
        printf("Error: Division By Zero!");
        break;
    case LERR_BAD_OP:
        printf("Error: Invalid Operator!");
        break;
    case LERR_BAD_NUM:
        printf("Error: Invalid Number!");
        break;
    default:
        break;
    }
}

// Print a Lisp value
void lval_print(lval v)
{
    switch (v.type)
    {
    case LVAL_NUM:
        printf("%li", v.num);
        break;
    case LVAL_ERR:
        lerr_print(v.err);
        break;
    default:
        break;
    }
}

// Print a Lisp value followed by a new line
void lval_println(lval v)
{
    lval_print(v);
    putchar('\n');
}

lval eval_op(lval x, char *op, lval y)
{
    // If either value is error return it
    if (x.type == LVAL_ERR)
    {
        return x;
    }
    if (y.type == LVAL_ERR)
    {
        return y;
    }

    // Do maths on number values
    if (strcmp(op, "+") == 0)
    {
        return lval_num(x.num + y.num);
    }
    if (strcmp(op, "-") == 0)
    {
        return lval_num(x.num - y.num);
    }
    if (strcmp(op, "*") == 0)
    {
        return lval_num(x.num * y.num);
    }
    if (strcmp(op, "/") == 0)
    {
        return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
    }

    // Invalid operator
    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t *t)
{
    /* Example AST: * 10 (+ 1 51)
    regex
    operator|char:1:1 '*'
    expr|number|regex:1:3 '10'
    expr|>
        char:1:6 '('
        operator|char:1:7 '+'
        expr|number|regex:1:9 '1'
        expr|number|regex:1:11 '51'
        char:1:13 ')'
    regex
     */

    // Check for number. Handle conversion error
    if (strstr(t->tag, "number"))
    {
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    // The operator is always the second child
    char *op = t->children[1]->contents;

    // Store the third child
    lval x = eval(t->children[2]);

    // Iterate the remaining children and combining
    int i = 3;
    while (strstr(t->children[i]->tag, "expr"))
    {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

int main(int argc, char **argv)
{
    // Create the parsers
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Operator = mpc_new("operator");
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    // Define the parsing rules
    mpca_lang(MPCA_LANG_DEFAULT,
              "\
    number   : /-?[0-9]+/ ;                             \
    operator : '+' | '-' | '*' | '/' ;                  \
    expr     : <number> | '(' <operator> <expr>+ ')' ;  \
    lispy    : /^/ <operator> <expr>+ /$/ ;             \
    ",
              Number, Operator, Expr, Lispy);

    // Print Version and Exit Instruction
    puts("Lispy version 0.0.0.0.1");
    puts("Press Ctrl+C to exit\n");

    // Infinite loop
    while (1)
    {
        // Output the prompt and get input
        char *input = readline("lispy> ");

        // Add input to history (retrieved with up and down arrows)
        add_history(input);

        // Attempt to parse the input
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r))
        {
            // Success parse: r.output is the AST
            lval result = eval(r.output);
            lval_println(result);
            mpc_ast_delete(r.output);
        }
        else
        {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        // Free retrieved input
        free(input);
    }

    // Undefine and delete the parsers
    mpc_cleanup(4, Number, Operator, Expr, Lispy);
    return 0;
}
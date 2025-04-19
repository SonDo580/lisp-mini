#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

// Macros
#define LASSERT(args, cond, err) \
    if (!(cond))                 \
    {                            \
        lval_del(args);          \
        return lval_err(err);    \
    }

// Possible Lisp value types
enum
{
    LVAL_ERR,   // error
    LVAL_NUM,   // number
    LVAL_SYM,   // symbol
    LVAL_SEXPR, // S-expression
    LVAL_QEXPR  // Q-expression
};

// Represent Lisp value
typedef struct lval
{
    int type;

    long num;
    char *err;
    char *sym;

    // list of lval pointer
    int count;
    struct lval **cell;
} lval;

// Construct pointer to a new Lisp value
lval *lval_num(long x);    // Number
lval *lval_err(char *msg); // Error
lval *lval_sym(char *s);   // Symbol
lval *lval_sexpr();        // S-Expression
lval *lval_qexpr();        // Q-Expression

// Delete a Lisp value
void lval_del(lval *v);

// Construct Lisp value from an AST node
lval *lval_read_num(mpc_ast_t *t); // Number
lval *lval_add(lval *v, lval *x);  // Add element to S-expression or a Q-expression
lval *lval_read(mpc_ast_t *t);

// Printing
void lval_expr_print(lval *v, char open, char close); // Print an S-expression or a Q-expression
void lval_print(lval *v);                             // Print a Lisp value
void lval_println(lval *v);                           // Print a Lisp value followed by a new line

// Evaluation
lval *lval_eval_sexpr(lval *v);         // Evaluate an S-expression
lval *lval_eval(lval *v);               // Evaluate a Lisp value
lval *builtin_op(lval *args, char *op); // Apply the operation on the argument list
lval *builtin(lval *args, char *func);  // Call correct built-in function based on symbol

// Utils
lval *lval_pop(lval *v, int i);  // Pop the element at index i
lval *lval_take(lval *v, int i); // Pop the element at index i and delete v

// List operations
lval *builtin_head(lval *args);
lval *builtin_tail(lval *args);
lval *builtin_list(lval *args);
lval *builtin_eval(lval *args);
lval *builtin_join(lval *args);
lval *lval_join(lval *x, lval *y);

int main(int argc, char **argv)
{
    // Create the parsers
    mpc_parser_t *Number = mpc_new("number");
    mpc_parser_t *Symbol = mpc_new("symbol");
    mpc_parser_t *Sexpr = mpc_new("sexpr"); // S-Expression
    mpc_parser_t *Qexpr = mpc_new("qexpr"); // Q-Expression
    mpc_parser_t *Expr = mpc_new("expr");
    mpc_parser_t *Lispy = mpc_new("lispy");

    // Define the parsing rules
    mpca_lang(MPCA_LANG_DEFAULT,
              "\
    number   : /-?[0-9]+/ ; \
    symbol : '+' | '-' | '*' | '/' \
           | \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\" ; \
    sexpr : '(' <expr>* ')' ;  \
    qexpr : '{' <expr>* '}' ;  \
    expr     : <number> | <symbol> | <sexpr> | <qexpr> ;  \
    lispy    : /^/ <expr>* /$/ ; \
    ",
              Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

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
            lval *x = lval_eval(lval_read(r.output));
            lval_println(x);
            lval_del(x);
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

    // Cleanup the parsers
    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
    return 0;
}

// Construct pointer to a new Number
lval *lval_num(long x)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

// Construct pointer to a new Error
lval *lval_err(char *msg)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(msg) + 1);
    strcpy(v->err, msg);
    return v;
}

// Construct pointer to a new Symbol
lval *lval_sym(char *s)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

// Construct pointer to a new S-Expression
lval *lval_sexpr()
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

// Construct pointer to a new Q-Expression
lval *lval_qexpr()
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

// Delete a Lisp value
void lval_del(lval *v)
{
    switch (v->type)
    {
    case LVAL_NUM:
        break;
    case LVAL_ERR:
        free(v->err);
        break;
    case LVAL_SYM:
        free(v->sym);
        break;
    case LVAL_SEXPR:
    case LVAL_QEXPR:
        for (int i = 0; i < v->count; i++)
        {
            lval_del(v->cell[i]);
        }

        free(v->cell);
        break;
    default:
        break;
    }

    free(v);
}

// Construct Number from an AST node
lval *lval_read_num(mpc_ast_t *t)
{
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

// Add element to S-expression or a Q-expression
lval *lval_add(lval *v, lval *x)
{
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);
    v->cell[v->count - 1] = x;
    return v;
}

// Construct Lisp value from an AST node
lval *lval_read(mpc_ast_t *t)
{
    // Handle number and symbol
    if (strstr(t->tag, "number"))
    {
        return lval_read_num(t);
    }
    if (strstr(t->tag, "symbol"))
    {
        return lval_sym(t->contents);
    }

    lval *x = NULL;

    if (strcmp(t->tag, ">") == 0 || strstr(t->tag, "sexpr"))
    {
        // Handle root and S-expression
        x = lval_sexpr();
    }
    else if (strstr(t->tag, "qexpr"))
    {
        // Handle Q-expression
        x = lval_qexpr();
    }

    // Adding valid expressions from children
    for (int i = 0; i < t->children_num; i++)
    {
        if (strcmp(t->children[i]->contents, "(") == 0 ||
            strcmp(t->children[i]->contents, ")") == 0 ||
            strcmp(t->children[i]->contents, "{") == 0 ||
            strcmp(t->children[i]->contents, "}") == 0 ||
            strcmp(t->children[i]->tag, "regex") == 0)
        {
            continue;
        }

        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

// Print an S-expression or a Q-expression
void lval_expr_print(lval *v, char open, char close)
{
    putchar(open);

    for (int i = 0; i < v->count; i++)
    {
        lval_print(v->cell[i]);

        if (i != (v->count - 1))
        {
            putchar(' ');
        }
    }

    putchar(close);
}

// Print a Lisp value
void lval_print(lval *v)
{
    switch (v->type)
    {
    case LVAL_NUM:
        printf("%li", v->num);
        break;
    case LVAL_ERR:
        printf("Error: %s", v->err);
        break;
    case LVAL_SYM:
        printf("%s", v->sym);
        break;
    case LVAL_SEXPR:
        lval_expr_print(v, '(', ')');
        break;
    case LVAL_QEXPR:
        lval_expr_print(v, '{', '}');
        break;
    default:
        break;
    }
}

// Print a Lisp value followed by a new line
void lval_println(lval *v)
{
    lval_print(v);
    putchar('\n');
}

// Evaluate an S-expression
lval *lval_eval_sexpr(lval *v)
{
    // Empty expression
    if (v->count == 0)
    {
        return v;
    }

    // Evaluate children
    for (int i = 0; i < v->count; i++)
    {
        v->cell[i] = lval_eval(v->cell[i]);

        // Error checking
        if (v->cell[i]->type == LVAL_ERR)
        {
            return lval_take(v, i);
        }
    }

    // Single expression
    if (v->count == 1)
    {
        return lval_take(v, 0);
    }

    // Ensure the first element is Symbol
    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_SYM)
    {
        lval_del(f);
        lval_del(v);
        return lval_err("S-expression must start with a symbol!");
    }

    // Call built-in with operator
    lval *result = builtin(v, f->sym);
    lval_del(f);
    return result;
}

// Evaluate a Lisp value
lval *lval_eval(lval *v)
{
    // Evaluate S-expression
    if (v->type == LVAL_SEXPR)
    {
        return lval_eval_sexpr(v);
    }
    return v; // Other types remain the same
}

// Pop the element at index i from an S-expression
lval *lval_pop(lval *v, int i)
{
    // Find the item at index i
    lval *x = v->cell[i];

    // Shift memory after the item at 'i' backward
    memmove(
        &v->cell[i],                        // destination start
        &v->cell[i + 1],                    // source start
        sizeof(lval *) * (v->count - 1 - i) // number of bytes to move
    );

    // Decrease the item count and shrink the memory used
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);

    // Return the popped element
    return x;
}

// Similar to lval_pop, but also delete the original value
lval *lval_take(lval *v, int i)
{
    lval *x = lval_pop(v, i);
    lval_del(v);
    return x;
}

// Apply the operation on the argument list
lval *builtin_op(lval *args, char *op)
{
    // Ensure all arguments are numbers
    for (int i = 0; i < args->count; i++)
    {
        if (args->cell[i]->type != LVAL_NUM)
        {
            lval_del(args);
            return lval_err("Cannot operate on non-number!");
        }
    }

    // Pop the first argument
    lval *x = lval_pop(args, 0);

    // Perform unary negation
    if (args->count == 0 && strcmp(op, "-") == 0)
    {
        x->num = -x->num;
        lval_del(args);
        return x;
    }

    // While there are arguments remaining
    while (args->count > 0)
    {
        // Pop the next element
        lval *y = lval_pop(args, 0);

        if (strcmp(op, "+") == 0)
        {
            x->num += y->num;
        }
        else if (strcmp(op, "-") == 0)
        {
            x->num -= y->num;
        }
        else if (strcmp(op, "*") == 0)
        {
            x->num *= y->num;
        }
        else if (strcmp(op, "/") == 0)
        {
            if (y->num == 0)
            {
                lval_del(x);
                lval_del(y);
                lval_del(args);
                return lval_err("Division by zero!");
            }

            x->num /= y->num;
        }

        lval_del(y);
    }

    lval_del(args);
    return x;
}

// Takes a Q-Expression and returns a Q-Expression with only the first element
lval *builtin_head(lval *args)
{
    LASSERT(args, args->count == 1, "Function 'head' - too many arguments");
    LASSERT(args, args->cell[0]->type == LVAL_QEXPR, "Function 'head' - incorrect argument type");
    LASSERT(args, args->cell[0]->count != 0, "Function 'head' - receive {}");

    // Pop the first element and delete args
    lval *v = lval_take(args, 0);

    // Delete elements that are not head and return
    while (v->count > 1)
    {
        lval_del(lval_pop(v, 1));
    }
    return v;
}

// Takes a Q-Expression and returns a Q-Expression with the first element removed
lval *builtin_tail(lval *args)
{
    LASSERT(args, args->count == 1, "Function 'tail' - too many arguments");
    LASSERT(args, args->cell[0]->type == LVAL_QEXPR, "Function 'tail' - incorrect argument type");
    LASSERT(args, args->cell[0]->count != 0, "Function 'tail' - receive {}");

    // Pop the first element and delete args
    lval *v = lval_take(args, 0);

    // Delete the first element and return
    lval_del(lval_pop(v, 0));
    return v;
}

// Returns a new Q-Expression containing the arguments
lval *builtin_list(lval *args)
{
    args->type = LVAL_QEXPR;
    return args;
}

// Takes a Q-Expression and evaluates it as if it were a S-Expression
lval *builtin_eval(lval *args)
{
    LASSERT(args, args->count == 1, "Function 'eval' - too many arguments");
    LASSERT(args, args->cell[0]->type == LVAL_QEXPR, "Function 'eval' - incorrect argument type");

    lval *v = lval_take(args, 0);
    v->type = LVAL_SEXPR;
    return lval_eval(v);
}

// Returns a Q-Expression by joining Q-Expressions together
lval *builtin_join(lval *args)
{
    for (int i = 0; i < args->count; i++)
    {
        LASSERT(args, args->cell[i]->type == LVAL_QEXPR, "Function 'join' - incorrect argument type");
    }

    lval *v = lval_pop(args, 0);
    while (args->count > 0)
    {
        v = lval_join(v, lval_pop(args, 0));
    }

    lval_del(args);
    return v;
}

// Helper for builtin_join - join 2 Q-Expressions together
lval *lval_join(lval *x, lval *y)
{
    // Append all elements from y to x
    while (y->count > 0)
    {
        x = lval_add(x, lval_pop(y, 0));
    }

    // Delete the empty y and return x
    lval_del(y);
    return x;
}

// Call correct built-in function based on symbol
lval *builtin(lval *args, char *func)
{
    if (strcmp("list", func) == 0)
    {
        return builtin_list(args);
    }
    if (strcmp("head", func) == 0)
    {
        return builtin_head(args);
    }
    if (strcmp("tail", func) == 0)
    {
        return builtin_tail(args);
    }
    if (strcmp("join", func) == 0)
    {
        return builtin_join(args);
    }
    if (strcmp("eval", func) == 0)
    {
        return builtin_eval(args);
    }
    if (strstr("+-*/", func))
    {
        return builtin_op(args, func);
    }

    lval_del(args);
    return lval_err("Unknown Function!");
}

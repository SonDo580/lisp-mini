#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>
#include <editline/history.h>

#include "mpc.h"

// Macros
#define LASSERT(args, cond, fmt_str, ...)             \
    if (!(cond))                                      \
    {                                                 \
        lval *err = lval_err(fmt_str, ##__VA_ARGS__); \
        lval_del(args);                               \
        return err;                                   \
    }

#define LASSERT_NUM_ARGS(func_name, args, expected_argc)                                  \
    LASSERT(args, args->count == expected_argc,                                           \
            "Function '%s' received incorrect number of arguments. Expected %i. Got %i.", \
            func_name, expected_argc, args->count);

#define LASSERT_ARG_TYPE(func_name, args, index, expected_type)                            \
    LASSERT(args, args->cell[index]->type == expected_type,                                \
            "Function '%s' received incorrect type for argument %i. Expected %s. Got %s.", \
            func_name, index, ltype_name(expected_type), ltype_name(args->cell[index]->type));

#define LASSERT_NOT_EMPTY(func_name, args, index) \
    LASSERT(args, args->cell[index]->count > 0,   \
            "Function '%s' passed {} for argument %i.", func_name, index);

// Forward type declarations
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

// Lisp value types
enum
{
    LVAL_ERR,   // error
    LVAL_NUM,   // number
    LVAL_SYM,   // symbol
    LVAL_SEXPR, // S-expression
    LVAL_QEXPR, // Q-expression
    LVAL_FUN    // function
};

// Function pointer
typedef lval *(*lbuiltin)(lenv *, lval *);

// List value
struct lval
{
    int type;

    long num;
    char *err;
    char *sym;

    // Function
    lbuiltin fun;
    char *fun_name;

    // S-expression and Q-expression
    int count;
    lval **cell;
};

// Environment (map sym -> lval)
struct lenv
{
    int count;
    char **syms;
    lval **vals;
};

// Construct a new Lisp value
lval *lval_num(long x);                    // Number
lval *lval_err(char *fmt_str, ...);        // Error
lval *lval_sym(char *s);                   // Symbol
lval *lval_sexpr();                        // S-Expression
lval *lval_qexpr();                        // Q-Expression
lval *lval_fun(lbuiltin func, char *name); // Function

// Delete a Lisp value
void lval_del(lval *v);

// Environment
lenv *lenv_new();                                          // Create new environment
void lenv_del(lenv *e);                                    // Delete an environment
lval *lenv_get(lenv *e, lval *k);                          // Lookup a value from the environment
void lenv_put(lenv *e, lval *k, lval *v);                  // Put value into the environment
void lenv_add_builtins(lenv *e);                           // Register all built-in functions
void lenv_add_builtin(lenv *e, char *name, lbuiltin func); // Register a built-in function

// Construct Lisp value from an AST node
lval *lval_read_num(mpc_ast_t *t); // Number
lval *lval_add(lval *v, lval *x);  // Add element to S-expression or a Q-expression
lval *lval_read(mpc_ast_t *t);

// Printing
void lval_expr_print(lval *v, char open, char close); // Print an S-expression or a Q-expression
void lval_print(lval *v);                             // Print a Lisp value
void lval_println(lval *v);                           // Print a Lisp value followed by a new line

// Evaluation
lval *lval_eval_sexpr(lenv *e, lval *v);         // Evaluate an S-expression
lval *lval_eval(lenv *e, lval *v);               // Evaluate a Lisp value
lval *builtin_op(lenv *e, lval *args, char *op); // Apply the operation on the argument list

// Utils
lval *lval_pop(lval *v, int i);   // Pop the element at index i
lval *lval_take(lval *v, int i);  // Pop the element at index i and delete v
lval *lval_copy(lval *v);         // Create a copy of v
char *ltype_name(int t);          // Return string representation of a type
int is_no_args_function(lval *v); // Check if v value is a function that accept no arguments

// Built-in math functions
lval *builtin_add(lenv *e, lval *args);
lval *builtin_sub(lenv *e, lval *args);
lval *builtin_mul(lenv *e, lval *args);
lval *builtin_div(lenv *e, lval *args);

// Built-in list functions
lval *builtin_head(lenv *e, lval *args);
lval *builtin_tail(lenv *e, lval *args);
lval *builtin_list(lenv *e, lval *args);
lval *builtin_eval(lenv *e, lval *args);
lval *builtin_join(lenv *e, lval *args);
lval *lval_join(lval *x, lval *y); // helper for builtin_join

// Handle variable definitions
lval *builtin_def(lenv *e, lval *args);

// Extra builtin functions
lval *builtin_env_print(lenv *e, lval *args); // Print all entries in an environment

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
    symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ; \
    sexpr : '(' <expr>* ')' ;  \
    qexpr : '{' <expr>* '}' ;  \
    expr     : <number> | <symbol> | <sexpr> | <qexpr> ;  \
    lispy    : /^/ <expr>* /$/ ; \
    ",
              Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    // Print Version and Exit Instruction
    puts("Lispy version 0.0.0.0.1");
    puts("Press Ctrl+C to exit\n");

    // Create a new environment and register built-in functions
    lenv *e = lenv_new();
    lenv_add_builtins(e);

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
            lval *x = lval_eval(e, lval_read(r.output));
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

// Construct new Number
lval *lval_num(long x)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

// Construct new Error
lval *lval_err(char *fmt_str, ...)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERR;

    // Initialize va_list to read extra arguments after fmt_str
    va_list variadic_args;
    va_start(variadic_args, fmt_str);

    // Allocate buffer for the error string
    const int BUFFER_SIZE = 512;
    v->err = malloc(BUFFER_SIZE);

    // Format the variadic arguments into the buffer
    vsnprintf(v->err, BUFFER_SIZE - 1, fmt_str, variadic_args);

    // Shrink the error buffer to the exact size needed
    v->err = realloc(v->err, strlen(v->err) + 1);

    // Clean up va_list and return the constructed error value
    va_end(variadic_args);
    return v;
}

// Construct new Symbol
lval *lval_sym(char *s)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

// Construct new S-Expression
lval *lval_sexpr()
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

// Construct new Q-Expression
lval *lval_qexpr()
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

// Construct new function
lval *lval_fun(lbuiltin func, char *name)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_FUN;
    v->fun = func;

    v->fun_name = malloc(strlen(name) + 1);
    strcpy(v->fun_name, name);

    return v;
}

// Delete a Lisp value
void lval_del(lval *v)
{
    switch (v->type)
    {
    case LVAL_NUM:
        break;
    case LVAL_FUN:
        free(v->fun_name);
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
    return errno != ERANGE ? lval_num(x) : lval_err("Invalid number: %s", t->contents);
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
    case LVAL_FUN:
        printf("<function '%s'>", v->fun_name);
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
lval *lval_eval_sexpr(lenv *e, lval *v)
{
    // Empty expression
    if (v->count == 0)
    {
        return v;
    }

    // Evaluate children
    for (int i = 0; i < v->count; i++)
    {
        v->cell[i] = lval_eval(e, v->cell[i]);

        // Error checking
        if (v->cell[i]->type == LVAL_ERR)
        {
            return lval_take(v, i);
        }
    }

    //  Single expression
    if (v->count == 1 && !is_no_args_function(v->cell[0]))
    {
        return lval_take(v, 0);
    }

    // Ensure the first element (after evaluation) is a function
    lval *f = lval_pop(v, 0);
    if (f->type != LVAL_FUN)
    {
        lval_del(f);
        lval_del(v);
        return lval_err("S-expression must start with a function!");
    }

    // Call function to get result
    lval *result = f->fun(e, v);
    lval_del(f);
    return result;
}

// Evaluate a Lisp value
lval *lval_eval(lenv *e, lval *v)
{
    // Variable resolution
    if (v->type == LVAL_SYM)
    {
        lval *x = lenv_get(e, v);
        lval_del(v);
        return x;
    }

    // Evaluate S-expression
    if (v->type == LVAL_SEXPR)
    {
        return lval_eval_sexpr(e, v);
    }

    // Other types remain the same
    return v;
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

// Create a copy of v
lval *lval_copy(lval *v)
{
    lval *x = malloc(sizeof(lval));
    x->type = v->type;

    switch (v->type)
    {
    // Copy number directly
    case LVAL_NUM:
        x->num = v->num;
        break;

    // Copy function
    case LVAL_FUN:
        x->fun = v->fun;
        x->fun_name = malloc(strlen(v->fun_name) + 1);
        strcpy(x->fun_name, v->fun_name);
        break;

    // Copy string for error and symbol
    case LVAL_ERR:
        x->err = malloc(strlen(v->err) + 1);
        strcpy(x->err, v->err);
        break;
    case LVAL_SYM:
        x->sym = malloc(strlen(v->sym) + 1);
        strcpy(x->sym, v->sym);
        break;

    // Copy List by copying each sub-expression
    case LVAL_SEXPR:
    case LVAL_QEXPR:
        x->count = v->count;
        x->cell = malloc(sizeof(lval *) * x->count);
        for (int i = 0; i < x->count; i++)
        {
            x->cell[i] = lval_copy(v->cell[i]);
        }
        break;
    }

    return x;
}

// Return string representation of a type
char *ltype_name(int t)
{
    switch (t)
    {
    case LVAL_FUN:
        return "Function";
    case LVAL_NUM:
        return "Number";
    case LVAL_ERR:
        return "Error";
    case LVAL_SYM:
        return "Symbol";
    case LVAL_SEXPR:
        return "S-Expression";
    case LVAL_QEXPR:
        return "Q-Expression";
    default:
        return "Unknown";
    }
}

// Check if v value is a function that accept no arguments
int is_no_args_function(lval *v)
{
    if (v->type != LVAL_FUN)
    {
        return 0;
    }

    return v->fun == builtin_env_print;
}

// Apply the operation on the argument list
lval *builtin_op(lenv *e, lval *args, char *op)
{
    // Ensure all arguments are numbers
    for (int i = 0; i < args->count; i++)
    {
        LASSERT_ARG_TYPE(op, args, i, LVAL_NUM);
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

// Built-in math functions
lval *builtin_add(lenv *e, lval *args)
{
    return builtin_op(e, args, "+");
}
lval *builtin_sub(lenv *e, lval *args)
{
    return builtin_op(e, args, "-");
}
lval *builtin_mul(lenv *e, lval *args)
{
    return builtin_op(e, args, "*");
}
lval *builtin_div(lenv *e, lval *args)
{
    return builtin_op(e, args, "/");
}

// Takes a Q-Expression and returns a Q-Expression with only the first element
lval *builtin_head(lenv *e, lval *args)
{
    const char *func_name = "head";
    LASSERT_NUM_ARGS(func_name, args, 1);
    LASSERT_ARG_TYPE(func_name, args, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY(func_name, args, 0);

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
lval *builtin_tail(lenv *e, lval *args)
{
    const char *func_name = "tail";
    LASSERT_NUM_ARGS(func_name, args, 1);
    LASSERT_ARG_TYPE(func_name, args, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY(func_name, args, 0);

    // Pop the first element and delete args
    lval *v = lval_take(args, 0);

    // Delete the first element and return
    lval_del(lval_pop(v, 0));
    return v;
}

// Returns a new Q-Expression containing the arguments
lval *builtin_list(lenv *e, lval *args)
{
    args->type = LVAL_QEXPR;
    return args;
}

// Takes a Q-Expression and evaluates it as if it were a S-Expression
lval *builtin_eval(lenv *e, lval *args)
{
    const char *func_name = "eval";
    LASSERT_NUM_ARGS(func_name, args, 1);
    LASSERT_ARG_TYPE(func_name, args, 0, LVAL_QEXPR);

    lval *v = lval_take(args, 0);
    v->type = LVAL_SEXPR;
    return lval_eval(e, v);
}

// Returns a Q-Expression by joining Q-Expressions together
lval *builtin_join(lenv *e, lval *args)
{
    const char *func_name = "join";

    for (int i = 0; i < args->count; i++)
    {
        LASSERT_ARG_TYPE(func_name, args, i, LVAL_QEXPR);
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

// Create new environment
lenv *lenv_new()
{
    lenv *e = malloc(sizeof(lenv));
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

// Delete an environment
void lenv_del(lenv *e)
{
    for (int i = 0; i < e->count; i++)
    {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}

// Lookup a value from the environment
lval *lenv_get(lenv *e, lval *k)
{
    for (int i = 0; i < e->count; i++)
    {
        // Return a copy of the value if found
        if (strcmp(e->syms[i], k->sym) == 0)
        {
            return lval_copy(e->vals[i]);
        }
    }

    // Return error if symbol not found
    return lval_err("Unbound symbol '%s", k->sym);
}

// Put value into the environment
void lenv_put(lenv *e, lval *k, lval *v)
{
    for (int i = 0; i < e->count; i++)
    {
        // If existing variable found, delete it and replace by the new one
        if (strcmp(e->syms[i], k->sym) == 0)
        {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    // If no existing entry found allocate space for new entry
    e->count++;
    e->vals = realloc(e->vals, sizeof(lval *) * e->count);
    e->syms = realloc(e->syms, sizeof(char *) * e->count);

    // Add the new entry
    e->vals[e->count - 1] = lval_copy(v);
    e->syms[e->count - 1] = malloc(strlen(k->sym) + 1);
    strcpy(e->syms[e->count - 1], k->sym);
}

// Handle variable definitions
lval *builtin_def(lenv *e, lval *args)
{
    const char *func_name = "def";

    // First argument is a symbol list
    LASSERT_ARG_TYPE(func_name, args, 0, LVAL_QEXPR);
    lval *syms = args->cell[0];
    for (int i = 0; i < syms->count; i++)
    {
        LASSERT(args, syms->cell[i]->type == LVAL_SYM,
                "Function 'def' - cannot define non-symbol. Expected %s. Got %s.",
                ltype_name(LVAL_SYM), ltype_name(syms->cell[i]->type));
    }

    // The remaining arguments is the value list
    LASSERT(args, syms->count == args->count - 1,
            "Function 'def' - number of symbols and values mismatch. ",
            "Number of symbols: %i. Number of values: %i",
            syms->count, args->count - 1);

    // Register the variables
    for (int i = 0; i < syms->count; i++)
    {
        lenv_put(e, syms->cell[i], args->cell[i + 1]);
    }

    // Delete 'args' and return an empty expression on success
    lval_del(args);
    return lval_sexpr();
}

// Print all entries in an environment
lval *builtin_env_print(lenv *e, lval *args)
{
    const char *func_name = "env_print";
    LASSERT_NUM_ARGS(func_name, args, 0);

    for (int i = 0; i < e->count; i++)
    {
        printf("Key: %s - Value: ", e->syms[i]);
        lval_println(e->vals[i]);
    }

    lval_del(args);
    return lval_sexpr();
}

// Register a built-in function
void lenv_add_builtin(lenv *e, char *name, lbuiltin func)
{
    lval *k = lval_sym(name);
    lval *v = lval_fun(func, name);
    lenv_put(e, k, v);
    lval_del(k);
    lval_del(v);
}

// Register all built-in functions
void lenv_add_builtins(lenv *e)
{
    // List functions
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);

    // Math functions
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);

    // Variable definition function
    lenv_add_builtin(e, "def", builtin_def);

    // Extra builtin functions
    lenv_add_builtin(e, "env_print", builtin_env_print);
}
1. Write a regular expression matching strings of all `a` or `b` such as `aababa` or `bbaa`

```js
/^[ab]+$/;
```

2. Write a regular expression matching strings of consecutive `a` and `b` such as `ababab` or `aba`

```js
/^a(b|(ba)*b?)$/;
```

3. Write a regular expression matching `pit`, `pot` and `respite` but not `peat`, `spit`, or `part`

```js
/^(pit|pot|respite)$/
/^(p(i|o)t|r.+)$/
// ...
```

## Current Lispy Grammar

```c
"\
    number   : /-?[0-9]+/ ;                             \
    operator : '+' | '-' | '*' | '/' ;                  \
    expr     : <number> | '(' <operator> <expr>+ ')' ;  \
    lispy    : /^/ <operator> <expr>+ /$/ ;             \
"
```

4. Change the grammar to add a new operator such as `%`

```js
`
operator : '+' | '-' | '*' | '/' | '%' ;
`;
```

5. Change the grammar to recognise operators written in textual format `add`, `sub`, `mul`, `div`

```js
`
operator : '+' | '-' | '*' | '/' | 'add' | 'sub' | 'mul' | 'div';
`;
```

6. Change the grammar to recognize decimal numbers such as `0.01`, `5.21`, or `10.2`

```js
`
number : /-?[0-9]+(.[0-9]+)?/ ;                             
`;
```

7. Change the grammar to make the operators written conventionally, between two expressions.

```js
`
term: <number> | '(' <expr> ')' ;
expr : <term> (<operator> <term>)* ;                         
`;
```

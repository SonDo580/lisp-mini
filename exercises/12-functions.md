## Define a Lisp function that returns the first element from a list.

```clojure
def {first} (\ {x} {eval (head x)})
first {1 2 3} ;; 1
```

## Define a Lisp function that returns the second element from a list.

```clojure
def {second} (\ {x} {eval (head (tail x))})
second {1 2 3} ;; 2
```

## Define a Lisp function that calls a function with two arguments in reverse order.

```clojure
def {hoc} (\ {f1 arg1 arg2} {f1 arg2 arg1})
hoc - 1 2 ;; 1
hoc - 2 1 ;; -1
```

## Define a Lisp function that calls a function with arguments, then passes the result to another function.

```clojure
def {h} (\ {g f & args}
           {g (eval (join {f} args))})
def {g} (\ {x} {* x x}) ;; square
h g * 1 2 ;; 4
```

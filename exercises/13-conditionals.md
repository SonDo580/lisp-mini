## Define a recursive Lisp function that returns the nth item of that list.

```clojure
def {nth} (\ {lst n}
             {if (< n 1)
                 {()}
                 {if (== lst {})
                     {()}
                     {if (== n 1)
                         {eval (head lst)}
                         {nth (tail lst) (- n 1)}}}})
```

## Define a recursive Lisp function that returns 1 if an element is a member of a list, otherwise 0.

```clojure
def {include}
    (\ {lst item}
       {if (== lst {})
           {0}
           {if (== item (eval (head lst)))
               {1}
               {include (tail lst) item}}})
```

## Define a Lisp function that returns the last element of a list.
```clojure
def {last}
    (\ {lst}
       {if (== lst {})
           {()}
           {if (== (tail lst) {})
               {eval (head lst)}
               {last (tail lst)}}})
```

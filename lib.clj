;; This is Lispy code, not Clojure code
;; I use the extension to get syntax highlighting 

;; ===== STANDARD LIBRARY =====

(def {nil} {})
(def {true} 1)
(def {false} 0)

;; Function definitions
(def {fun} 
  (\ {f+args body}
   {def (head f+args) 
    (\ (tail f+args) body)}))

;; Unpack list for function
(fun {unpack f args}
     {eval (join (list f)) args})

;; Pack list for function
(fun {pack f & args}
     {f args})

;; Curried and un-curried calling
(def {curry} unpack)
(def {uncurry} pack)

;; Perform things in sequence
;; Note: the list items are evaluated then the last one is returned
(fun {do & lst}
     {if (== lst nil)
      {nil}
      {last lst}})

;; Open new scope
;; Creates an empty function for code to take place in, and evaluates it.
(fun {let body}
     {((\ {_} body) ())})
;; lispy> let {do (= {x} 100) (x)}
;; 100
;; lispy> x
;; Error: Unbound Symbol 'x'

;; Logical operators
(fun {not x} {- 1 x})
(fun {or x y} {+ x y})
(fun {and x y} {* x y})

;; Apply function to 2 arguments in reversed order
(fun {flip f a b} {f b a})

;; Evaluate the arguments list as an expression
(fun {ghost & args} {eval args})
;; ghost + 2 2 
;; -> 4

;; Compose 2 functions
(fun {comp f g args} {f (g args)})
;; (def {neg-mul} (comp - (unpack *)))
;; (neg-mul {2 8})
;; -> -16

;; First/Second/Third item in list
(fun {first lst} {eval (head lst)})
(fun {second lst} {eval (head (tail lst))})
(fun {third lst} {eval (head (tail (tail lst)))})

;; Find list length
(fun {len lst}
     {if (== lst nil)
      {0}
      {+ 1 (len (tail lst))}})

;; Nth item in list
(fun {nth n lst}
     {if (== n 0)
      {first lst}
      {nth (- n 1) (tail lst)}})

;; Last item in list
(fun {last lst}
     {nth (- (len lst) 1) lst})

;; Take n items from a list
(fun {take n lst}
     {if (== n 0)
      {nil}
      {join (head lst) (take (- n 1) (tail lst))}})

;; Drop n items from a list
(fun {drop n lst}
     {if (== n 0)
      {lst}
      {drop (- n 1) (tail lst)}})

;; Split into 2 lists at n
(fun {split n lst}
     {list (take n lst) (drop n lst)})

;; Check if an element is in list
(fun {elem x lst}
     {if (== lst nil)
      {false}
      {if (== x (first lst))
       {true}
       {elem x (tail lst)}}})

;; elem - use foldl
(fun {elem x lst}
     {foldl 
      (\ {acc item} {or acc (== x item)}) 
      false 
      lst})

;; Apply a function to every element in list
(fun {map f lst}
     {if (== lst nil)
      {nil}
      {join (list (f (first lst)) (map f (tail lst)))}})

;; Apply filter on a list
(fun {filter f lst}
     {if (== lst nil)
      {nil}
      {join 
       (if (f (first lst))
         {head lst}
         {nil}) 
       (filter f (tail lst))}})

;; Fold left (reduce)
(fun {foldl f base lst}
     {if (= lst nil)
      {base}
      {foldl f (f base (first lst)) (tail lst)}})

;; Sum and product of elements in list
(fun {sum lst} {foldl + 0 lst})
(fun {product lst} {foldl * 0 lst})

;; Conditional - select
(def {otherwise} true) ;; like default
(fun {select & cases}
     {if (== cases nil)
      {error "No selection found"}
      {if (first (first cases))
       {second (first cases)}
       {unpack select (tail cases)}}})
;; (fun {month-day-suffix i}
;;      {select 
;;       {(== i 0) "st"}
;;       {(== i 1) "nd"}
;;       {(== i 2) "rd"}
;;       {otherwise "th"}})

;; Conditional - case
(fun {case x & cases}
     {if (== cases nil)
      {error "No cases found"}
      {if (== x (first (first cases)))
       {second (first cases)}
       {unpack case (join (list x) (tail cases))}}})
;; (fun {day-name x}
;;      {case x
;;       {0 "Monday"}
;;       {1 "Tuesday"}
;;       {2 "Wednesday"}
;;       {3 "Thursday"}
;;       {4 "Friday"}
;;       {5 "Saturday"}
;;       {6 "Sunday"}})

;; Fibonacci
(fun {fib n}
     {select
      {(== n 0) 0}
      {(== n 1) 1}
      {otherwise (+ (fib (- n 1))
                    (fib (- n 2)))}})

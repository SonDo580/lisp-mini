**5.1. Write down some more examples of strings the Doge language contains**

```python
adjective -> "wow" | "many" | "so" | "such";
noun -> "lisp" | "language" | "book" | "build" | "c";
phrase -> adjective noun;
doge -> phrase*
```

- wow lisp
- many language so build
- wow c so book such lisp
- ...

**5.2. Describe textually a grammar for decimal numbers such as 0.01 or 52.221**

```python
non_zero_digit -> '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9';
digit -> '0' | non_zero_digit;
integer -> '0' | non_zero_digit digit*;
decimal -> '.' digit+;
number -> integer decimal?;
```

**5.3. Describe textually a grammar for web URLs such as http://www.buildyourownlisp.com**

```python
scheme -> 'http' | 'https' ;
top_level_domain -> 'com' | 'org' | 'edu' | 'gov' | 'io' | 'co' | 'uk' | 'us' | ... ;
subdomain -> 'www' | 'mail' | ... ;
main_domain -> [a-zA-Z0-9]+ ;
domain -> (subdomain '.')? main_domain '.' top_level_domain ;
port -> ':' [0-9]+ ;
path -> ('/' [a-zA-Z0-9-._~%!$&'()*+,;=:@]*)* ;
query -> '?' ([a-zA-Z0-9-._~%!$&'()*+,;=:@]*)? ;
hash -> '#' ([a-zA-Z0-9-._~%!$&'()*+,;=:@]*)? ;
url -> scheme "://" domain port? path query? hash?;
```

**5.4. Describe textually a grammar for simple English sentences such as "the cat sat on the mat"**

```python
article -> 'the' ;
noun -> 'cat' | 'mat' ;
verb -> 'sat' ;
preposition -> 'on' ;
noun_phrase -> (article)? noun;
preposition_phrase -> preposition noun_phrase ;
verb_phrase -> verb (preposition? noun_phrase)? ;
sentence -> noun_phrase verb_phrase ;
```

**5.5. If you are familiar with JSON, textually describe a grammar for it**

```python
space -> [\s]* ;
key -> '"' .+ '"' ;
non_zero_digit -> [1-9] ;
digit -> 0 | non_zero_digit ;
integer -> 0 | non_zero_digit digit* ;
decimal -> '.' digit+
number -> integer decimal? ;
string -> '"' .* '"' ;
null -> 'null' ;
terminal -> number | string | null ;
value -> terminal | array | object ;
array_item -> space value space ;
array -> '[' array_item (',' array_item)* ']' ;
object_entry -> space key space ':' space value space;
object -> '{' object_entry (',' object_entry)* '}' ;
```

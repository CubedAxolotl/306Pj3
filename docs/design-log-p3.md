# Project 3 Design Log
**Author:** <Emmanuel> · **P2 code used:** <my own>
**Tools used:** <Claude for fixing bugs, formatting ugly code, testing.>



## 1. Hash table
I used chaining as I found it most simple to implement and search through. It allows for multiple tools to share the same bucket. For hashing I went with a 64-bit FNV-1a function. The table starts with 8 buckets and a max load factor of 1. When adding to the table, code checks if the table has ecceeded its limit and resizes twice its size. Adding a too should be O(1) but resizing makes it take longer in special cases. 

## 2. Pointer stability
Each tool is stored in the heap and uses unique_ptrs. When the table resizes, the pointers are moved to a separate buckets while the tools remain in the heap. 

## 3. Calculator trace
I divided the calculator into two stacks, one for one for numbers and another for expressions.

The calculator handles the parentheses first and uses operator precedence to perform multiplication before subtraction.
## 4. Errors as values
When a tool fails, ex.  when a file is missing or a calculation divides by zero, the program returns an error message instead of crashing.
## 5. What I Would Design Differently
Similarly to my project 2 concensus the spec is extremely detailed yet vague. I feel a disconnect between what we learned in class and what was given to us. I found the hash table part of this program engaging but tool registry and calculator eval are busy work. Specially calculator eval. 

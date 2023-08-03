File: readme.txt
Author: Mihir Anand
----------------------
Allocater Type: Explicit
1. The two design decisions I made for the explicit heap allocater included the fact that I used a typedef header and that I created a struct
of link pointers. First, instead of using size_t pointers everywhere in my code to represent the headers, by typedef-ing them (as was suggested
in the instructions) I was able to make my code more readable and reduce confusion. Regarding the struct, by creating "link *", it made it a little
bit more easy to built the linked list. One scenario where my code for explicit would show stronger performance is if there is more free blocks on
the heap than allocated, as the linked list makes it much faster to find the right free block to use (compared to implicit which has to go through
the whole heap every time). One scenario where my code for implicit would show weaker performance is if there are more allocated blocks and rarely any
free ones (heap is almost full) as the linked list would no longer be as much of an advantage in terms of efficiency. One attempt I made at optimizing
my explicit heap allocater was by incorporating coalescing so that the linked list is even more efficient when searching through it.


2. Some assumptions I am making is that the client does not realloc or malloc bytes than there are on the entire heap and that the client does not
attempt to free a block that is already free. In addition, I am assuming that the client does not introduce any aspects of the stack memory into these
functions. If the client defies these assumptions, it will lead to incredibly unpredictable behavior and will mostly likely crash the program.



Tell us about your quarter in CS107!
-----------------------------------
This quarter in CS107 has definitely been quite tough! At the same time, I learned a tremendous amount and a lot of the insights that Nick Troccoli
provided about computers actually broadened my understanding of how they function at a very fundamental level. I put a lot of effort into all the
assignments and exams. While some of the bugs and segmentation faults can be incredibly frustrating, I am excited to continue pursuing a Computer
Science degree at Stanford and learn more in computing.



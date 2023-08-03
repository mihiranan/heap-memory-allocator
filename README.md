## Heap Allocation Memory Management – A Review

### Implicit Heap Allocator
An implicit heap allocator maintains free and allocated blocks within the heap without any specific data structure to manage the free blocks. 
Instead, it typically uses header information to traverse the entire heap to find suitable free blocks. 
This approach often leads to the following characteristics:

* **Overhead:** Generally lower overhead per block, since there's no need to store additional information such as next or previous free block pointers.

* **Search Time:** Finding free blocks may be slower, especially if the heap is large, since the entire heap must be traversed to find a suitable block.

* **Coalescing:** If implemented, coalescing (combining adjacent free blocks) can be more time-consuming, as it requires searching for adjacent free blocks during allocation or deallocation.

### Explicit Heap Allocator
An explicit heap allocator maintains a data structure, such as a linked list, to manage the free blocks within the heap. 
This method has some distinct characteristics:

* **Overhead:** Typically, there is a higher overhead for each free block, as pointers to next and/or previous free blocks must be stored.

* **Search Time:** Finding free blocks can be faster, especially if the data structure is optimized for search, such as a balanced binary search tree or sorted linked list.

* **Coalescing:** Coalescing adjacent free blocks can be more efficient, as the data structure can directly provide access to neighboring free blocks.

* **Fragmentation:** Managing free blocks explicitly can allow for strategies that reduce fragmentation, such as best-fit or first-fit algorithms.

---

## Explicit Heap Allocator Reflection

### Design Decisions:
Two major design decisions were taken into account in the creation of this explicit heap allocator:

1. **Use of Typedef Headers**: By utilizing typedef for headers, I improved the readability of the code and reduced potential confusion. This approach allowed me to focus on the logic without   getting tangled in complex pointer representations.

2. **Creation of Link Pointers Struct**: The development of a "link *" struct facilitated the construction of the linked list, making the overall process more manageable and intuitive.

### Performance Considerations
- **Strengths**: This approach particularly shines when there are more free blocks on the heap compared to allocated ones. The linked list structure significantly speeds up the process of finding suitable free blocks, as opposed to traversing the entire heap.
- **Weaknesses**: The method may be less efficient when the heap is nearly full, and there are few free blocks available, diminishing the advantage of the linked list in terms of efficiency.

### Optimization
An attempt at optimization was made by incorporating **coalescing**, making the linked list even more efficient during searches.

### Assumptions
- **No Over-allocation**: The client is expected not to realloc or malloc more bytes than available on the entire heap.
- **Proper Block Handling**: Clients should avoid attempting to free an already free block.
- **Stack Memory Isolation**: The client should not introduce aspects of stack memory into the allocation functions.

### Conclusion
This project allowed me to delve into the intricate workings of memory management and sharpen my understanding of heap allocation. It showcases not only the functionality of an explicit heap allocator but also the thought process and optimizations that can make such a system more efficient and robust.

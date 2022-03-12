Assignment 1 Writeup
=============

My name: Soomin Choi

My POVIS ID: choisium

My student ID (numeric): 20160169

This assignment took me about 7.5 hours to do (including the time on studying, designing, and writing the code).

Program Structure and Design of the StreamReassembler:  

## Data Structure 

- `buffer`: a map to store unassembled segments with its start index. 

- `next_assembled_index`: the next index to be assembled. 

- `_unassembled_bytes`: the total bytes of strings stored in `buffer`. Note 
that `unassembled_bytes` + `_output.buffer_size()` should be less than or equal 
to its capacity. 

- `eof_index`: the last index + 1 of last segment. 


## Logic of `push_substring` 

1. Call `push_unoverlapped_substring` 
   - Find all unoverlapped substrings of segment `data`. 
   - Push all unoverlapped substrings into `buffer`. 

2. Call `resolve_underflow` 
   - Remove the segments that overflows the capacity. 

3. Call `reassemble` 
   - Move in-order segments into bytestream `_output`. 
   - The segments moved into `_output` can be read by user. 

4. Extra chores 
   - Record `eof_index` if `eof` is activated. 
   - Check if all segments are reassembled. If all segments are reassembled, 
    signal to `_output` by calling `end_input`. 

Implementation Challenges: 

- It was hard to design a function to find all nonoverlapped substrings from
`data`. I considered many ways like returning the index pair of nonoverlapped 
strings, but finally I decided to allow temporary overflow of buffer and push 
all nonoverlapped substrings into `buffer`. This allowed me to divide 
functionalities of `push_nonoverlapped_substring` and `resolve_overflow`. 

- I refactored push_substring since it has too many functionalities at first. 
Finally there are three functions called in `push_substring`. 

Remaining Bugs: None. This code passes all tests. 

- Optional: I had unexpected difficulty with: determining the data structures. 
I changed all of my codes twice. 

- Optional: I think you could make this assignment better by: 

- Unify the type of the parameter `index` in method `push_string`. The type in 
header is `uint64_t`, while that in code file is `size_t`. 

- Optional: I was surprised by: - 

- Optional: I'm not sure about: - 
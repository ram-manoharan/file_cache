# file_cache
In memory File cache (Based on LRU eviction)


Design :

   Refer "cache_design.pdf"

Requirements :

   Refer requirements.txt

Compilation:

   do a make.

Execution :

   ./cache <cache_size> <reader_file_list> <writer_file_list> <item_file>

Output:

   new *.out.txt files will be generated against each reader file."item_list.txt" will be updated accordingly.


How to enable Debug?

  Compile object file with "-DDEBUG" (add in CXXFLAGS in Makefile).

How to get performance Benchmark?

 Compile object file with "-DPERFORMANCE_TEST" (add in CXXFLAGS in Makefile).Refer "performance_output.txt" file for sample benchmarking.

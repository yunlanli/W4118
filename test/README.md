## TEST_PROGRAM:
- we assume that the file system is mounted at /mnt.
To run the program, we need to
1. compile the two executables map_pages and inspect_pages
1. run the map_pages executable
1. press ENTER upon prompts to see changes

## RESULT:

[ INFO ] ./inspect_pages 2064
[ RESULT ] pid: 2064, pid->comm: map_pages, zero: 0, total: 385
Press enter to map 5 zero pages...

[ INFO ] ./inspect_pages 2064
[ RESULT ] pid: 2064, pid->comm: map_pages, zero: 0, total: 391
Press enter to transform 5 zero pages to non-zero pages...

[ INFO ] ./inspect_pages 2064
[ RESULT ] pid: 2064, pid->comm: map_pages, zero: 0, total: 391
Press enter to scrub 5 non-zero pages to zero pages...

[ INFO ] ./inspect_pages 2064
[ RESULT ] pid: 2064, pid->comm: map_pages, zero: 0, total: 391
Press enter to unmap 5 zero pages...

[ INFO ] ./inspect_pages 2064
[ RESULT ] pid: 2064, pid->comm: map_pages, zero: 0, total: 386
Press enter to exit...

## EXPECTATION AND ANALYSIS:
1. [385 -> 391] In step one, we attempted to map 5 zero pages using mmap
   with MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED to create zero filled pages.
   We expect to see 5 zero pages, yet see an increase of 6 in total page. 
   - The increase of 6 pages instead of 5 might be due to the fact that we
       are executing some fair amount of code in between the log -> causing
       one more page being created
   - Instead of zero page, we have total page. This means either our zero
       page detection using "is_zero_pfn" is incorrect, or we did not manage
       to understand how to create zero page in user program.
1. [391 -> 391] Since we are scrubbing the zero -> non-zero page, the total
   amount of pages does not change. This is as expected and the result
   conforms with our expectation.
1. [391 -> 391] Firsly, we are transforming between non-zero -> zero page,
   so the total number of pages is still unchanged, this is as expected.
   However, since we didn't manage to get the zero page count working, that
   number is not updated to 5.
1. [391 -> 386] Lastly, we are unmapping 5 pages. So we are expecting a
   reduction of 5 for the total number of pages. This time the result
   conforms with our expectation.



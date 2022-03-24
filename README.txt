This file should contain:

-You Zhou yz3883, Aoxue Wei aw3389, Panyu Gao pg2676
-HW5
-Description for each part

Part1 works successfully. This part is not hard, but I made a lot of mistakes in freeing memory. Also, for spin lock, when there are two conditions to return differnt things, I only unlocked it in one condition, which cause the deadlock in the other condition. But I have solved them all.

Test case - flower_t1: This test checks the kkv_put() function's concurrency between threads. We implement a program called "chameleon" we use multiple threads calling kkv_put() to change color between green and red.

Test case - flower_t2: This test checks that if no entry is found, kkv_get will return "-ENOENT", and different keys does not affect each other.

Part2: We use read write lock in this part. We regards init/destroy as writers and put/get as readers of the hashtable. The only problem is that put/get may strave init/destroy.

Test case - p1_test1: This test checks the kkv_put() function's concurrency between threads. We implement a program called "chameleon" we use multiple threads calling kkv_put() to change color between green and red.

Test case - p1_test2: This test checks that if no entry is found, kkv_get will return "-ENOENT", and different keys does not affect each other.

Test case - p1_test3: This test checks that the size is not 0, and val is not NULL

Test case - p2-misuse-t1: This check the performance when destroy and init are called twice

Test case - p2-misuse-t2: This check the performance when *kkv_put() called while executing kkv_put()

Test case - p2-misise-t3: This call kkv_put() while doing kkv_init()

Part3 works successfully. We just create the SLAB cache by `kmem_cache_create`. and replace `kmalloc()` and `kfree()` to appropriate SLAB allocator calls `kmem_cache_alloc()` and `kmem_cache_free()`.

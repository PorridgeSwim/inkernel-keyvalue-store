This file should contain:

-You Zhou yz3883, Aoxue Wei 3389, Panyu Gao pg2676
-HW5
-	Description for each part

Part1 works successfully. This part is not hard, but I made a lot of mistakes in freeing memory. Also, for spin lock, when there are two conditions to return differnt things, I only unlocked it in one condition, which cause the deadlock in the other condition. But I have solved them all.

Part2: We use read write lock in this part. We regards init/destroy as writers and put/get as readers of the hashtable. The only problem is that put/get may strave init/destroy.

Test case - p1_test1: This test checks the kkv_put() function's concurrency between threads. We implement a program called "chameleon" we use multiple threads calling kkv_put() to change color between green and red.

Test case - p1_test2: This test checks that if no entry is found, kkv_get will return "-ENOENT", and different keys does not affect each other.

Test case - p1_test3: This test checks that the size is not 0, and val is not NULL

Test case - p2-misuse-t1: This check the performance when destroy and init are called twice

Test case - p2-misuse-t2: This check the performance when *kkv_put() called while executing kkv_put()

Test case - p2-misise-t3: This call kkv_put() while doing kkv_init()

The description should indicate whether your solution for the part is working
or not. You may also want to include anything else you would like to
communicate to the grader, such as extra functionality you implemented or how
you tried to fix your non-working code.

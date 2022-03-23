This file should contain:

-You Zhou yz3883, Aoxue Wei aw3389, Panyu Gao pg2676
-HW5
-	Description for each part

Part1 works successfully. This part is not hard, but I made a lot of mistakes in freeing memory. Also, for spin lock, when there are two conditions to return differnt things, I only unlocked it in one condition, which cause the deadlock in the other condition. But I have solved them all.

Test case - flower_t1: This test checks the kkv_put() function's concurrency between threads. We implement a program called "chameleon" we use multiple threads calling kkv_put() to change color between green and red.

Test case - flower_t2: This test checks that if no entry is found, kkv_get will return "-ENOENT", and different keys does not affect each other.


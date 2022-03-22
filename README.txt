This file should contain:

-You Zhou yz3883, Aoxue Wei 3389, Panyu Gao pg2676
-HW5
-	Description for each part

Part1 works successfully. This part is not hard, but I made a lot of mistakes in freeing memory. Also, for spin lock, when there are two conditions to return differnt things, I only unlocked it in one condition, which cause the deadlock in the other condition. But I have solved them all.

Part2: We use read write lock in this part. We regards init/destroy as writers and put/get as readers of the hashtable. The only problem is that put/get may strave init/destroy.

The description should indicate whether your solution for the part is working
or not. You may also want to include anything else you would like to
communicate to the grader, such as extra functionality you implemented or how
you tried to fix your non-working code.

Part3 works successfully.

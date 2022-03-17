/*
 * fridge.c
 *
 * A kernel-level key-value store. Accessed via user-defined
 * system calls. This is the module implementation.
 */

#include <linux/module.h>
#include <linux/printk.h>
#include "fridge_data_structures.h"

#define MODULE_NAME "Fridge"

extern void * kkv_init_ptr;
extern void * kkv_destory_ptr;
extern void * kkv_put_ptr;
extern void * kkv_get_ptr;

long kkv_init(int flags)
{
	static struct kkv_ht_bucket *hashtable = kmalloc(17*sizeof(kkv_ht_bucket));
	struct kkv_ht_bucket *current;
	for (i=0; i<17, i++){
		current = hashtable+i;
		spin_lock_init(current->lock);
		INIT_LIST_HEAD(current->entries);
		current->count = 0;
	}
}

long kkv_destory(int flags)
{
}

long kkv_put(uint32_t key, void __user *val, size_t size, int flags)
{
}

long kkv_get(uint32_t key, void __user *val, size_t size, int flags)
{
}

int fridge_init(void)
{
	pr_info("Installing fridge\n");

	//may need lock here
	//
	kkv_init_ptr = kkv_init;
	kkv_destory_ptr = kkv_destory;
	kkv_put_ptr = kkv_put;
	kkv_get_ptr = kkv_get;
	return 0;
}

void fridge_exit(void)
{
	pr_info("Removing fridge\n");

	//may need lock here
	//
	kkv_init_ptr = NULL;
	kkv_destory_ptr = NULL;
	kkv_put_ptr = NULL;
	kkv_get_ptr = NULL;
}

module_init(fridge_init);
module_exit(fridge_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(MODULE_NAME);
MODULE_AUTHOR("cs4118");

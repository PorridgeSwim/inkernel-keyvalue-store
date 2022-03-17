/*
 * fridge.c
 *
 * A kernel-level key-value store. Accessed via user-defined
 * system calls. This is the module implementation.
 */

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "fridge_data_structures.h"

#define MODULE_NAME "Fridge"

extern void * kkv_init_ptr;
extern void * kkv_destory_ptr;
extern void * kkv_put_ptr;
extern void * kkv_get_ptr;

long kkv_init(int flags)
{
	static struct kkv_ht_bucket *hashtable = kmalloc(17*sizeof(struct kkv_ht_bucket), GFP_KERNEL);
	int i = 0;
	struct kkv_ht_bucket *current;

	if (hashtable == NULL) {
		printk(KERN_ERR "kmalloc() failed to allocate memory\n");
		return -1;
	}

	for (i=0; i<17; i++){
		current = hashtable+i;
		spin_lock_init(current->lock);
		INIT_LIST_HEAD(&current->entries);
		current->count = 0;
	}
	return 0;
}

long kkv_destory(int flags)
{
	static struct kkv_ht_bucket *hashtable;
	struct kkv_ht_bucket *current;
	struct kkv_ht_entry *cur;
	struct kkv_ht_entry *nxt;
	struct list_head *tem_list;
	long sum = 0;

	for (i=0; i<17; i++){
		current = hashtable + i;
		tem_list = &current->entries;
		list_for_each_entry_safe(cur, nxt, tem_list, entries){
			list_del(&cur->entries);
			kfree(cur);
			current->count--;
			sum++;
		}
	}
	kfree(hashtable);
	return sum;
}

long kkv_get(uint32_t key, void __user *val, size_t size, int flags)
{
	static struct kkv_ht_bucket *hashtable;
	struct kkv_ht_bucket *current;
	struct kkv_ht_entry *cur;
	int index = key % 17;

	current = hashtable + index;
	list_for_each_entry(cur, &current->entries, entries){
		if((cur->kv_pair).key == key){
			list_del(&cur->entries);
			current->count--;
			if(copy_to_user(val, (cur->kv_pair).val, size))
				return -EFAULT;
			kfree(cur);
			return 0;
		}
	}
	return -ENOENT;
}

long kkv_put(uint32_t key, void __user *val, size_t size, int flags)
{
	static struct kkv_ht_bucket *hashtable;
	struct kkv_ht_bucket *current;
	struct kkv_ht_entry *cur;
	struct kkv_ht_entry *new_entry;
	int index = key % 17;

	current = hashtable + index;
	list_for_each_entry(cur, &current->entries, entries){
		if ((cur->kv_pair).key == key){
			if(copy_from_user((cur->kv_pair).val, val, size))
				return -EFAULT;
			return 0;
		}
	}
	

	new_entry = kmalloc(sizeof(struct kkv_ht_entry), GFP_KERNEL);
	if (new_entry == NULL) {
		printk(KERN_ERR "kmalloc() failed to allocate memory\n");
		return -1;
	}
	INIT_LIST_HEAD(&new_entry->entries);
	list_add_tail(&new_entry->entries, &current->entries);
	(new_entry->kv_pair).key = key;
	(new_entry->kv_pair).size = size;

	//maybe we don't need to copy from use twice
	if (copy_from_user((new_entry->kv_pair).val, val, size))
		return -EFAULT;
	current->count++;
	return 0;
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

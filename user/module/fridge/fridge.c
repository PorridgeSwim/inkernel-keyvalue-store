/*
 * fridge.c
 *
 * A kernel-level key-value store. Accessed via user-defined
 * system calls. This is the module implementation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/kernel.h>
#include "fridge_data_structures.h"

#define MODULE_NAME "Fridge"

extern long (*kkv_init_ptr)(int flags);
extern long (*kkv_destroy_ptr)(int flags);
extern long (*kkv_put_ptr)(uint32_t key, void *val, size_t size, int flags);
extern long (*kkv_get_ptr)(uint32_t key, void *val, size_t size, int flags);

static struct kkv_ht_bucket *hashtable;

long kkv_init(int flags)
{
	int i = 0;
	struct kkv_ht_bucket *CUR;

	hashtable = kmalloc_array(HASH_TABLE_LENGTH, sizeof(struct kkv_ht_bucket), GFP_KERNEL);
	if (hashtable == NULL)
		return -ENOMEM;

	for (i = 0; i < HASH_TABLE_LENGTH; i++) {
		CUR = hashtable+i;
		spin_lock_init(&CUR->lock);
		INIT_LIST_HEAD(&CUR->entries);
		CUR->count = 0;
	}

	return 0;
}

long kkv_destroy(int flags)
{
	struct kkv_ht_bucket *CUR;
	struct kkv_ht_entry *cur;
	struct kkv_ht_entry *nxt;
	struct list_head *tem_list;
	int i;
	long sum = 0;

	for (i = 0; i < HASH_TABLE_LENGTH; i++) {
		CUR = hashtable + i;
		tem_list = &CUR->entries;
		list_for_each_entry_safe(cur, nxt, tem_list, entries) {
			list_del(&cur->entries);
			kfree((cur->kv_pair).val);
			kfree(cur);
			CUR->count--;
			sum++;
		}
	}
	kfree(hashtable);
	return sum;
}

long kkv_get(uint32_t key, void __user *val, size_t size, int flags)
{
	struct kkv_ht_bucket *CUR;
	struct kkv_ht_entry *cur;
	void *pos;
	int index = key % HASH_TABLE_LENGTH;
	size_t cur_size;

	CUR = hashtable + index;
	pos = kmalloc_array(size, sizeof(char), GFP_KERNEL);
	if (pos == NULL)
		return -ENOMEM;

	spin_lock(&CUR->lock);
	list_for_each_entry(cur, &CUR->entries, entries) {
		if ((cur->kv_pair).key == key) {
			list_del(&cur->entries);
			CUR->count--;
			spin_unlock(&CUR->lock);

			cur_size = (cur->kv_pair).size;
			strcpy(pos, (cur->kv_pair).val);
			kfree((cur->kv_pair).val);
			kfree(cur);
			if (copy_to_user(val, pos, min(size, cur_size))) {
				kfree(pos);
				return -EFAULT;
			}
			kfree(pos);
			return 0;
		}
	}
	spin_unlock(&CUR->lock);

	kfree(pos);
	return -ENOENT;
}

long kkv_put(uint32_t key, void __user *val, size_t size, int flags)
{
	struct kkv_ht_bucket *CUR;
	struct kkv_ht_entry *cur;
	struct kkv_ht_entry *new_entry;
	void *pos;
	void *tem;
	int index = key % HASH_TABLE_LENGTH;

	pos = kmalloc_array(size, sizeof(char), GFP_KERNEL);
	if (pos == NULL)
		return -ENOMEM;

	CUR = hashtable + index;
	if (copy_from_user(pos, val, size)) {
		kfree(pos);
		return -EFAULT;
	}

	new_entry = kmalloc(sizeof(struct kkv_ht_entry), GFP_KERNEL);
	if (new_entry == NULL) {
		kfree(pos);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&new_entry->entries);
	(new_entry->kv_pair).key = key;
	(new_entry->kv_pair).size = size;
	(new_entry->kv_pair).val = pos;

	spin_lock(&CUR->lock);
	list_for_each_entry(cur, &CUR->entries, entries) {
		if ((cur->kv_pair).key == key) {
			tem = (cur->kv_pair).val;
			(cur->kv_pair).val = pos;
			(cur->kv_pair).size = size;
			spin_unlock(&CUR->lock);

			kfree(tem);
			kfree(new_entry);
			return 0;
		}
	}
	list_add_tail(&new_entry->entries, &CUR->entries);
	CUR->count++;
	spin_unlock(&CUR->lock);

	return 0;
}

int fridge_init(void)
{
	kkv_init_ptr = kkv_init;
	kkv_destroy_ptr = kkv_destroy;
	kkv_put_ptr = kkv_put;
	kkv_get_ptr = kkv_get;

	pr_info("Installing fridge\n");

	return 0;
}

void fridge_exit(void)
{
	kkv_init_ptr = NULL;
	kkv_destroy_ptr = NULL;
	kkv_put_ptr = NULL;
	kkv_get_ptr = NULL;

	pr_info("Removing fridge\n");
}

module_init(fridge_init);
module_exit(fridge_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(MODULE_NAME);
MODULE_AUTHOR("cs4118");

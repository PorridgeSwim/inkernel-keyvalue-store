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
static rwlock_t rwlock;	// init, destroy are writer, and put, get are reader

long kkv_init(int flags)
{
	int i = 0;
	struct kkv_ht_bucket *CUR;
	struct kkv_ht_bucket *tem_table;

	tem_table = kmalloc_array(HASH_TABLE_LENGTH, sizeof(struct kkv_ht_bucket), GFP_KERNEL);
	if (tem_table == NULL)
		return -ENOMEM;

	// create the table first, then assign it to hashtable

	for (i = 0; i < HASH_TABLE_LENGTH; i++) {
		CUR = tem_table+i;
		spin_lock_init(&CUR->lock);
		INIT_LIST_HEAD(&CUR->entries);
		CUR->count = 0;
	}

	if (!write_trylock(&rwlock)) {
		kfree(tem_table);
		return -EPERM;
	}
	if (hashtable != NULL) {
		write_unlock(&rwlock);
		kfree(tem_table);
		return -EPERM;
	}
	hashtable = tem_table;
	write_unlock(&rwlock);
	return 0;
}

long kkv_destroy(int flags)
{
	struct kkv_ht_bucket *CUR;
	struct kkv_ht_entry *cur;
	struct kkv_ht_entry *nxt;
	struct list_head *tem_list;
	struct kkv_ht_bucket *tem_table;
	int i;
	long sum = 0;

	// release the hashtable first, then free the memory is previously inside it

	if (!write_trylock(&rwlock))
		return -EPERM;
	if (hashtable == NULL) {
		write_unlock(&rwlock);
		return -EPERM;
	}
	tem_table = hashtable;
	hashtable = NULL;
	write_unlock(&rwlock);

	for (i = 0; i < HASH_TABLE_LENGTH; i++) {
		CUR = tem_table + i;
		tem_list = &CUR->entries;
		list_for_each_entry_safe(cur, nxt, tem_list, entries) {
			list_del(&cur->entries);
			kfree((cur->kv_pair).val);
			kfree(cur);
			CUR->count--;
			sum++;
		}
	}
	kfree(tem_table);
	return sum;
}

long kkv_get(uint32_t key, void __user *val, size_t size, int flags)
{
	struct kkv_ht_bucket *CUR;
	struct kkv_ht_entry *cur;
	void *pos;
	int index = key % HASH_TABLE_LENGTH;
	size_t cur_size;

	pos = kmalloc_array(size, sizeof(char), GFP_KERNEL);
	if (pos == NULL)
		return -ENOMEM;

	if (!read_trylock(&rwlock)) {
		kfree(pos);
		return -EPERM;
	}
	if (hashtable == NULL) {
		read_unlock(&rwlock);
		kfree(pos);
		return -EPERM;
	}
	CUR = hashtable + index;
	spin_lock(&CUR->lock);
	list_for_each_entry(cur, &CUR->entries, entries) {
		if ((cur->kv_pair).key == key) {
			list_del(&cur->entries);
			CUR->count--;
			spin_unlock(&CUR->lock);
			read_unlock(&rwlock);

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
	read_unlock(&rwlock);

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

	if (!read_trylock(&rwlock)) {
		kfree(pos);
		kfree(new_entry);
		return -EPERM;
	}
	if (hashtable == NULL) {
		read_unlock(&rwlock);
		kfree(pos);
		kfree(new_entry);
		return -EPERM;
	}
	CUR = hashtable + index;
	spin_lock(&CUR->lock);
	list_for_each_entry(cur, &CUR->entries, entries) {
		if ((cur->kv_pair).key == key) {
			tem = (cur->kv_pair).val;
			(cur->kv_pair).val = pos;
			(cur->kv_pair).size = size;
			spin_unlock(&CUR->lock);
			read_unlock(&rwlock);

			kfree(tem);
			kfree(new_entry);
			return 0;
		}
	}
	list_add_tail(&new_entry->entries, &CUR->entries);
	CUR->count++;
	spin_unlock(&CUR->lock);
	read_unlock(&rwlock);

	return 0;
}

int fridge_init(void)
{
	rwlock_init(&rwlock);
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

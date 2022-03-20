/*
 * fridge.c
 *
 * A kernel-level key-value store. Accessed via user-defined
 * system calls. This is the module implementation.
 */

#include <linux/module.h>
#include <linux/printk.h>
#include "fridge_data_structures.h"
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/spinlock.h>

#define MODULE_NAME "Fridge"

extern long (*kkv_init_ptr)(int flags);
extern long (*kkv_destroy_ptr)(int flags);
extern long (*kkv_put_ptr)(uint32_t key, void __user *val, size_t size, int flags);
extern long (*kkv_get_ptr)(uint32_t key, void __user *val, size_t size, int flags);

// static struct kkv_ht_bucket *hash_table = NULL;

struct bucket_withlock {
	spinlock_t lock;
	int active_count;
	struct kkv_ht_bucket *hash_table_withlock;
};

static struct bucket_withlock init_bucket;
// spin_lock_init(&(init_bucket.lock));
// init_bucket.hash_table_withlock = NULL;

void active_count_control(int n);


long kkv_init(int flags)
{
	struct kkv_ht_bucket *hash_table;
	int i;
	struct kkv_ht_bucket *index;

	hash_table = kmalloc_array(HASH_TABLE_LENGTH, sizeof(*hash_table), GFP_KERNEL);
	if (hash_table == NULL)
		return -ENOMEM;

	for (i = 0; i < HASH_TABLE_LENGTH; i++) {
		index = &hash_table[i];
		INIT_LIST_HEAD(&index->entries);
		spin_lock_init(&index->lock);
		index->count = 0;
	}

	spin_lock(&(init_bucket.lock));
	if (init_bucket.hash_table_withlock != NULL) {
		spin_unlock(&(init_bucket.lock));
		kfree(hash_table);
		return -EPERM;
	} else {
		init_bucket.hash_table_withlock = hash_table;
		spin_unlock(&(init_bucket.lock));
	}

	return 0;
}

long kkv_destroy(int flags)
{
	struct kkv_ht_bucket *hash_table;
	int count = 0;
	int i;
	struct kkv_ht_entry *cur, *nxt;
	struct list_head *head_ptr;

	spin_lock(&(init_bucket.lock));
	if ((init_bucket.hash_table_withlock != NULL) && (init_bucket.active_count == 0)) {
		hash_table = init_bucket.hash_table_withlock;
		init_bucket.hash_table_withlock = NULL;
		spin_unlock(&(init_bucket.lock));
	} else {
		spin_unlock(&(init_bucket.lock));
		return -EPERM;
	}


	for (i = 0; i < HASH_TABLE_LENGTH; i++) {
		head_ptr = &(&hash_table[i])->entries;
		list_for_each_entry_safe(cur, nxt, head_ptr, entries) {
			list_del(&cur->entries);
			kfree(cur->kv_pair.val);
			kfree(cur);
			count++;
		}
	}

	kfree(hash_table);

	return count;
}

long kkv_put(uint32_t key, void __user *val, size_t size, int flags)
{
	struct kkv_ht_bucket *hash_table;
	struct kkv_ht_entry *new_entry;
	struct kkv_ht_entry *ht_entry;
	struct kkv_ht_bucket *bucket_ptr;
	struct list_head *head_ptr;
	int bucket_index;
	typeof(*val) *val_kernel;
	void *tmp;

	spin_lock(&(init_bucket.lock));
	if (init_bucket.hash_table_withlock != NULL) {
		hash_table = init_bucket.hash_table_withlock;
		init_bucket.active_count++;
		spin_unlock(&(init_bucket.lock));
	} else {
		spin_unlock(&(init_bucket.lock));
		return -EPERM;
	}

	// val_kernel = kmalloc(sizeof(*val), GFP_KERNEL);

	val_kernel = kmalloc_array(size, sizeof(char), GFP_KERNEL);
	if (val_kernel == NULL)
		return -ENOMEM;

	bucket_index = key%HASH_TABLE_LENGTH;
	bucket_ptr = &hash_table[bucket_index];
	head_ptr = &bucket_ptr->entries;

	if (copy_from_user(val_kernel, val, size)) {
		kfree(val_kernel);
		return -EFAULT;
	}

	new_entry = kmalloc(sizeof(*new_entry), GFP_KERNEL);
	if (new_entry == NULL) {
		kfree(val_kernel);
		return -ENOMEM;
	}

	new_entry->kv_pair.key = key;
	new_entry->kv_pair.val = val_kernel;
	new_entry->kv_pair.size = size;

	INIT_LIST_HEAD(&new_entry->entries);

	spin_lock(&bucket_ptr->lock);
	list_for_each_entry(ht_entry, head_ptr, entries) {
		if (ht_entry->kv_pair.key == key) {
			tmp = ht_entry->kv_pair.val;
			ht_entry->kv_pair.val = val_kernel;
			ht_entry->kv_pair.size = size;
			spin_unlock(&bucket_ptr->lock);

			active_count_control(-1);

			kfree(tmp);
			kfree(new_entry);

			return 0;
		}
	}

	list_add_tail(&new_entry->entries, head_ptr);
	bucket_ptr->count++;
	spin_unlock(&bucket_ptr->lock);

	active_count_control(-1);

	return 0;
}

long kkv_get(uint32_t key, void __user *val, size_t size, int flags)
{
	struct kkv_ht_bucket *hash_table;
	struct kkv_ht_entry *cur, *nxt;
	struct list_head *head_ptr;
	struct kkv_ht_bucket *bucket_ptr;
	int bucket_index;

	spin_lock(&(init_bucket.lock));
	if (init_bucket.hash_table_withlock != NULL) {
		hash_table = init_bucket.hash_table_withlock;
		init_bucket.active_count++;
		spin_unlock(&(init_bucket.lock));
	} else {
		spin_unlock(&(init_bucket.lock));
		return -EPERM;
	}

	bucket_index = key%HASH_TABLE_LENGTH;
	bucket_ptr = &hash_table[bucket_index];
	head_ptr = &bucket_ptr->entries;

	spin_lock(&bucket_ptr->lock);
	list_for_each_entry_safe(cur, nxt, head_ptr, entries) {
		if (cur->kv_pair.key == key) {
			list_del(&cur->entries);
			bucket_ptr->count--;
			spin_unlock(&bucket_ptr->lock);
			goto out_get;
		}
	}
	spin_unlock(&bucket_ptr->lock);

	active_count_control(-1);

	return -ENOENT;

out_get:
	if (copy_to_user(val, cur->kv_pair.val, min(size, cur->kv_pair.size)))
		return -EFAULT;

	kfree(cur->kv_pair.val);
	kfree(cur);

	active_count_control(-1);

	return 0;
}

void active_count_control(int n)
{
	if (n == 1) {
		spin_lock(&(init_bucket.lock));
		init_bucket.active_count++;
		spin_unlock(&(init_bucket.lock));
	} else {
		spin_lock(&(init_bucket.lock));
		init_bucket.active_count--;
		spin_unlock(&(init_bucket.lock));
	}
}

int fridge_init(void)
{
	spin_lock_init(&(init_bucket.lock));
	init_bucket.hash_table_withlock = NULL;
	init_bucket.active_count = 0;
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

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/types.h>

struct entity {
	char *info;
	int seq;
	bool avail;
	struct list_head list;
};

struct list_stats {
	uint64_t total;
	uint64_t deleted;
	uint64_t available;
	uint64_t footprint;
};

static struct dentry *ov_dirp;
static char page_buf[PAGE_SIZE];
static char list_buf[PAGE_SIZE];
static LIST_HEAD(head);
static struct list_stats stats;
static loff_t size;
static DEFINE_MUTEX(io_mutex);

/*
 * List implementation
 */

static int entity_add(char *info, int seq)
{
	struct entity *new_el;

	if (info == NULL)
		return -EINVAL;

	new_el = kmalloc(sizeof(struct entity), GFP_KERNEL);
	if (new_el == NULL)
		return -ENOMEM;

	new_el->info = kmalloc(strlen(info) + 1, GFP_KERNEL);
	if (new_el->info == NULL) {
		kfree(new_el);
		return -ENOMEM;
	}

	new_el->avail = false;
	new_el->seq = seq;
	strcpy(new_el->info, info);

	list_add(&new_el->list, &head);

	return 0;
}

static int entity_remove(int seq)
{
	size_t len;
	struct entity *current_entry, *next;

	list_for_each_entry_safe(current_entry, next, &head, list)
		if (current_entry->seq == seq) {
			len = strlen(current_entry->info);

			list_del(&current_entry->list);
			kfree(current_entry->info);
			kfree(current_entry);

			return len;
		}

	return -EINVAL;
}

static int entity_cleanall(void)
{
	struct entity *current_entry, *next;

	list_for_each_entry_safe(current_entry, next, &head, list) {
		list_del(&current_entry->list);
		kfree(current_entry->info);
		kfree(current_entry);
	}

	return 0;
}

/* Add file */

static ssize_t add_write(struct file *file, const char __user *buf,
			    size_t len, loff_t *ppos)
{
	ssize_t ret;
	int seq;

	pr_debug("Entering function: %s, with len = %lu\n", __func__, len);

	mutex_lock(&io_mutex);

	/*
	 * The last byte needs to be a null terminator in order to be able
	 * to add the string to the list.
	 */
	ret = simple_write_to_buffer(list_buf, PAGE_SIZE - 1, ppos, buf, len);
	if (ret != len) {
		pr_err("simple_write_to_buffer error.\n");
		ret = -EINVAL;
		goto fail;
	}
	list_buf[len] = '\0';

	seq = stats.total;

	ret = entity_add(list_buf, seq);
	if (ret != 0) {
		pr_err("Could not add element: (%s, %d) to list.\n",
		       list_buf, seq);
		goto fail;
	}

	++stats.total;
	++stats.available;
	stats.footprint += (sizeof(struct entity) + strlen(list_buf));

	pr_debug("Added element to list: (%s, %d)\n", list_buf, seq);
	ret = len;
fail:
	pr_debug("Exiting function: %s\n", __func__);
	mutex_unlock(&io_mutex);
	return ret;
}

static const struct file_operations add_fops = {
	.owner = THIS_MODULE,
	.write = add_write,
};


/* Del file */

static ssize_t del_write(struct file *file, const char __user *buf,
			    size_t len, loff_t *ppos)
{
	ssize_t ret;
	int seq;

	pr_debug("Entering function %s, with len = %lu\n", __func__, len);
	mutex_lock(&io_mutex);

	ret = -EINVAL;

	if (len > PAGE_SIZE - 1) {
		pr_err("Maximum size allowed: %lu\n", PAGE_SIZE - 1);
		goto exit;
	}

	ret = kstrtoint_from_user(buf, len, 10, &seq);
	if (ret < 0) {
		pr_err("kstrtoint_from_user - Invalid input.\n");
		goto exit;
	}

	if (seq == 0) {
		entity_cleanall();
		stats.deleted += stats.available;
		stats.available = 0;
		stats.footprint = 0;
		
		goto exit;
	}
	
	ret = entity_remove(seq);
	if (ret < 0) {
		pr_err("%d doesn't exist in the list. Exiting.\n", seq);
		goto exit;
	}

	++stats.deleted;
	--stats.available;
	stats.footprint -= (sizeof(struct entity) + ret);

	pr_debug("Deleted element with seq: %d\n", seq);
	ret = len;
exit:
	pr_debug("Exiting function: %s\n", __func__);
	mutex_unlock(&io_mutex);
	return ret;
}

static const struct file_operations del_fops = {
	.owner = THIS_MODULE,
	.write = del_write,
};

/* Stats file */

static ssize_t stats_read(struct file *file, char __user *buf,
			 size_t len, loff_t *ppos)
{
	ssize_t ret;
	size_t size;

	size = 0;

	mutex_lock(&io_mutex);

	size += scnprintf(list_buf, PAGE_SIZE, "Stats:\n");
	size += scnprintf(list_buf + size, PAGE_SIZE - size,
			  "Total nr. of items added so far: %llu\n",
			  stats.total);
	size += scnprintf(list_buf + size, PAGE_SIZE - size,
			  "Nr. of items deleted so far: %llu\n",
			  stats.deleted);
	size += scnprintf(list_buf + size, PAGE_SIZE - size,
			  "Current nr. of items in list: %llu\n",
			  stats.available);
	size += scnprintf(list_buf + size, PAGE_SIZE - size,
			  "Memory footprint: %llu bytes\n",
			  stats.footprint);

	ret = simple_read_from_buffer(buf, len, ppos, list_buf, size);

	mutex_unlock(&io_mutex);

	return ret;
}

static const struct file_operations stats_fops = {
	.owner = THIS_MODULE,
	.read = stats_read,
};

/* Page */

static ssize_t page_write(struct file *file, const char __user *buf,
			    size_t len, loff_t *ppos)
{
	ssize_t ret;

	mutex_lock(&io_mutex);

	pr_debug("Entering function %s\n", __func__);

	ret = simple_write_to_buffer(page_buf, PAGE_SIZE, ppos, buf, len);
	if (ret != len) {
		ret = -EINVAL;
		pr_debug("Failed to write to buffer\n");
	}

	size = *ppos;

	pr_debug("Exiting function %s\n", __func__);

	mutex_unlock(&io_mutex);

	return ret;
}

static ssize_t page_read(struct file *file, char __user *buf,
			 size_t len, loff_t *ppos)
{
	ssize_t ret;

	mutex_lock(&io_mutex);
	pr_debug("Entering function %s\n", __func__);

	ret = simple_read_from_buffer(buf, len, ppos, page_buf, size);

	pr_debug("Exiting function %s\n", __func__);
	mutex_unlock(&io_mutex);

	return ret;
}

static const struct file_operations page_fops = {
	.owner = THIS_MODULE,
	.write = page_write,
	.read = page_read,
};

/* Init and cleanup */

static int __init klist_init(void)
{
	struct dentry *fp;
	ssize_t ret;

	pr_debug("Entering function %s\n", __func__);
	
	ret = -ENOENT;

	ov_dirp = debugfs_create_dir("ovidiu", NULL);
	if (IS_ERR_OR_NULL(ov_dirp)) {
		pr_err("Failed to create debugfs directory %s\n",
		       "ovidiu");
		return ret;
	}

	fp = debugfs_create_file("add", 0220, ov_dirp, NULL, &add_fops);
	if (IS_ERR_OR_NULL(fp)) {
		pr_err("Failed to create debugfs file %s\n",
		       "add");
		goto fail;
	}

	fp = debugfs_create_file("del", 0220, ov_dirp, NULL, &del_fops);
	if (IS_ERR_OR_NULL(fp)) {
		pr_err("Failed to create debugfs file %s\n",
		       "del");
		goto fail;
	}

	fp = debugfs_create_file("stats", 0444, ov_dirp, NULL, &stats_fops);
	if (IS_ERR_OR_NULL(fp)) {
		pr_err("Failed to create debugfs file %s\n",
		       "stats");
		goto fail;
	}

	fp = debugfs_create_file("page", 0444, ov_dirp, NULL, &page_fops);
	if (IS_ERR_OR_NULL(fp)) {
		pr_err("Failed to create debugfs file %s\n",
		       "page");
		goto fail;
	}

	pr_debug("Function %s terminated successfully\n", __func__);
	return 0;

fail:
	entity_cleanall();
	debugfs_remove_recursive(ov_dirp);
	return ret;
}

static void __exit klist_exit(void)
{
	pr_debug("Entering function %s\n", __func__);
	
	entity_cleanall();
	debugfs_remove_recursive(ov_dirp);
	pr_debug("Function %s terminated successfully\n", __func__);
}

module_init(klist_init);
module_exit(klist_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Testing kernel linked list implementation");

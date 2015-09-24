#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

void __fuzz_coverage(void)
{
	struct task_struct *t;
	unsigned long pos;

	t = current;
	if (!t || !t->ecover || in_interrupt())
		return;
	pos = t->cover_pos & t->cover_mask;
	t->ecover[pos] = (u32)_RET_IP_;
	t->cover_pos++;
}
EXPORT_SYMBOL(__fuzz_coverage);

void sancov_syscall_enter(void)
{
	struct task_struct *t;

	t = current;
	/* In case we prematurely exited from the previous syscall.
	   This is wrong and should be replaced with the BUG,
	   but it fails sometimes as of now.
	   BUG_ON(t->ecover); */
	t->cover_committed = t->cover_pos;
	t->ecover = t->cover;
}

void sancov_syscall_exit(void)
{
	struct task_struct *t;

	t = current;
	t->ecover = NULL;
	t->cover_committed = t->cover_pos;
}

static ssize_t cover_write(struct file *file, const char __user *addr,
	size_t len, loff_t *pos)
{
	struct task_struct *t;
	u32 *cover;
	unsigned long size;
	char buf[32];

	if (len >= sizeof(buf))
		return -E2BIG;
	memset(buf, 0, sizeof(buf));
	if (copy_from_user(buf, addr, len))
		return -EFAULT;

	t = current;
	BUG_ON(t->cover != t->ecover);
	if (!strncmp(buf, "enable=", sizeof("enable=") - 1)) {
		if (t->cover)
			return -EEXIST;
		if (kstrtoul(buf + sizeof("enable=") - 1, 10, &size))
			return -EINVAL;
		if (size <= 0 || size > (128<<20) || size & (size - 1))
			return -EINVAL;
		cover = kmalloc(size * sizeof(u32), GFP_KERNEL);
		if (!cover)
			return -ENOMEM;
		t->cover = cover;
		t->cover_pos = 0;
		t->cover_mask = size - 1;
		BUG_ON(t->ecover);
	} else if (!strcmp(buf, "disable")) {
		cover = t->cover;
		if (!cover)
			return -EEXIST;
		t->ecover = NULL;
		t->cover = NULL;
		t->cover_pos = 0;
		t->cover_mask = 0;
		kfree(cover);
	} else if (!strcmp(buf, "reset")) {
		if (!t->cover)
			return -ENOTTY;
		t->ecover = NULL;
		t->cover_pos = 0;
	} else
		return -ENXIO;

	return len;
}

static ssize_t cover_read(struct file *file, char __user *addr, size_t len,
	loff_t *pos)
{
	struct task_struct *t;

	t = current;
	BUG_ON(t->cover != t->ecover);
	if (!t->cover)
		return -ENODEV;
	len = min(len, t->cover_committed * sizeof(u32));
	len = min(len, (t->cover_mask + 1) * sizeof(u32));
	if (copy_to_user(addr, t->cover, len))
		return -EFAULT;
	return len;
}

static const struct file_operations cover_ops = {
	.open		= simple_open,
	.llseek		= noop_llseek,
	.read		= cover_read,
	.write		= cover_write,
};

static __init int coverage_init(void)
{
	proc_create("cover", S_IRUSR|S_IRGRP|S_IROTH, NULL, &cover_ops);
	return 0;
}

device_initcall(coverage_init);

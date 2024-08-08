#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/mm_types.h>
#include <asm/tlbflush.h>

#define PROC_FILENAME "manual_flush"

static char cmd_buf[20];
static unsigned long addr_buf[512];
static int addr_count = 0;

static void batch_manual_flush(void) {
    struct tlbflush_unmap_batch *tlb_batch = &current->tlb_ubc;
    struct mm_struct *mm = current->mm;
    for (int i = 0; i < addr_count; i++) {
        arch_tlbbatch_add_pending(tlb_batch, mm, addr_buf[i]);
    }
}

static void single_manual_flush(void) {

}

static void get_addrs(void) {
    addr_count = 0;
    for (int i = 0; i < 512; i++) {
        addr_buf[i] = 0;
    }
    struct mm_struct *mm = current->mm;
    // struct tlbflush_unmap_batch *tlb_batch = &current->tlb_ubc;

    // Iterate over all the VMA regions
    VMA_ITERATOR(vmi, mm, 0);
    struct vm_area_struct *vma;

    for_each_vma(vmi, vma) {
        for (unsigned long addr = vma->vm_start; addr < vma->vm_end && addr_count < 512; addr += PAGE_SIZE) {
            addr_buf[addr_count++] = addr;
        }
    }

    if (addr_count != 512) {
        pr_warn("manual_flush: only %d addresses found\n", addr_count);
    }
}

// File operations structure for the proc file
static ssize_t proc_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
    if (count >= 20) {
        pr_info("manual_flush: command too long\n");
        return 0;
    }
    ssize_t ret = count;

    // get the command, and parse
    if (copy_from_user(cmd_buf, buf, count)) {
        return -EFAULT;
    }

    // Get the addresses to flush
    get_addrs();

    // Check if the command is "batch"
    if (strncmp(cmd_buf, "batch", 5) == 0) {
        pr_info("manual_flush: batch command\n");
        batch_manual_flush();
    } else if (strncmp(cmd_buf, "single", 5) == 0) {
        pr_info("manual_flush: single command\n");
        single_manual_flush();
    } else {
        pr_info("manual_flush: invalid command\n");
        return -EINVAL;
    }

    return ret;
}

// File operations structure
static const struct proc_ops proc_fops = {
    .proc_write = proc_write,
};

// Module initialization function
static int __init manual_flush_init(void) {
    struct proc_dir_entry *entry;

    // Create the /proc file
    entry = proc_create(PROC_FILENAME, 0666, NULL, &proc_fops);
    if (!entry) {
        printk(KERN_ALERT "manual_flush: failed to create /proc file\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "manual_flush: /proc/%s created\n", PROC_FILENAME);
    return 0;
}

// Module cleanup function
static void __exit manual_flush_exit(void) {
    // Remove the /proc file
    remove_proc_entry(PROC_FILENAME, NULL);
    printk(KERN_INFO "manual_flush: /proc/%s removed\n", PROC_FILENAME);
}

module_init(manual_flush_init);
module_exit(manual_flush_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Xingkai");
MODULE_DESCRIPTION("A kernel module that flushes the TLB on writing to /proc/manual_flush.");

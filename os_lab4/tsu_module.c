#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/seq_file.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student TSU");
MODULE_DESCRIPTION("TSU module with Lunar New Year 2025 calculations");

#define PROC_FILENAME "tsulab"

#define LNY2025_START_YEAR   2025
#define LNY2025_START_MONTH  1
#define LNY2025_START_DAY    29

#define LNY2025_END_YEAR     2025
#define LNY2025_END_MONTH    2
#define LNY2025_END_DAY      12

static unsigned long calc_days_since(int year, int month, int day)
{
    struct tm tm = {
        .tm_year = year - 1900,
        .tm_mon  = month - 1,
        .tm_mday = day,
        .tm_hour = 0,
        .tm_min  = 0,
        .tm_sec  = 0
    };

    time64_t event_time = mktime64(&tm);
    time64_t now = ktime_get_real_seconds();

    if (now < event_time)
        return 0;

    return (now - event_time) / 86400;
}

static int proc_show(struct seq_file *m, void *v)
{
    unsigned long days_since_start =
        calc_days_since(LNY2025_START_YEAR, LNY2025_START_MONTH, LNY2025_START_DAY);

    unsigned long days_since_end =
        calc_days_since(LNY2025_END_YEAR, LNY2025_END_MONTH, LNY2025_END_DAY);

    seq_printf(m,
        "Days since Lunar New Year 2025 start (29 Jan 2025): %lu\n"
        "Days since Lunar New Year 2025 end (12 Feb 2025): %lu\n",
        days_since_start,
        days_since_end
    );

    return 0;
}

static int proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, proc_show, NULL);
}

static const struct proc_ops proc_file_ops = {
    .proc_open    = proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

static int __init tsu_init(void)
{
    pr_info("Welcome to the Tomsk State University\n");

    if (!proc_create(PROC_FILENAME, 0, NULL, &proc_file_ops)) {
        pr_err("Failed to create /proc/%s\n", PROC_FILENAME);
        return -ENOMEM;
    }

    pr_info("/proc/%s created\n", PROC_FILENAME);
    return 0;
}

static void __exit tsu_exit(void)
{
    proc_remove(proc_create(PROC_FILENAME, 0, NULL, &proc_file_ops));
    pr_info("Tomsk State University forever!\n");
}

module_init(tsu_init);
module_exit(tsu_exit);

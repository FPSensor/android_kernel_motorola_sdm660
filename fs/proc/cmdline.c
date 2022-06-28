// SPDX-License-Identifier: GPL-2.0
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <asm/setup.h>

#ifdef CONFIG_INITRAMFS_IGNORE_SKIP_FLAG
#define INITRAMFS_STR_FIND "skip_initramf"
#define INITRAMFS_STR_REPLACE "want_initramf"
#define INITRAMFS_STR_LEN (sizeof(INITRAMFS_STR_FIND) - 1)
#endif

static char proc_command_line[COMMAND_LINE_SIZE];

static void __init proc_command_line_init(void)
{
	strlcpy(proc_command_line, saved_command_line,
		sizeof(proc_command_line));

#ifdef CONFIG_INITRAMFS_IGNORE_SKIP_FLAG
	{
		char *offset_addr = strstr(proc_command_line,
					   INITRAMFS_STR_FIND);

		if (offset_addr)
			memcpy(offset_addr, INITRAMFS_STR_REPLACE,
			       INITRAMFS_STR_LEN);
	}
#endif
}

static int cmdline_proc_show(struct seq_file *m, void *v)
{
	seq_puts(m, proc_command_line);
	seq_putc(m, '\n');
	return 0;
}

static int __init proc_cmdline_init(void)
{
	proc_command_line_init();
	proc_create_single("cmdline", 0, NULL, cmdline_proc_show);
	return 0;
}
fs_initcall(proc_cmdline_init);

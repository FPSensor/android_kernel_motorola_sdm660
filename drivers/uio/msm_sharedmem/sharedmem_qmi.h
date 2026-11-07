/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2014, 2017, The Linux Foundation. All rights reserved.
 */

#ifndef __SHAREDMEM_QMI_H__
#define __SHAREDMEM_QMI_H__

#include <linux/types.h>

struct sharemem_qmi_entry {
	const char *client_name;
	u32 client_id;
	u64 address;
	u32 size;
	bool is_addr_dynamic;
};

int sharedmem_qmi_init(void);
void sharedmem_qmi_exit(void);
void sharedmem_qmi_add_entry(struct sharemem_qmi_entry *qmi_entry);

#endif /* __SHAREDMEM_QMI_H__ */

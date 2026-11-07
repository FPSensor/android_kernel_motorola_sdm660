// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2014-2015, 2017, The Linux Foundation. All rights reserved.
 *
 * RFSA (remote filesystem access) QMI service: tells the modem the
 * address of the shared memory buffers set up by msm_sharedmem.
 * Ported from the 4.4 msm QMI interface to the 4.19 QMI helpers.
 */

#define pr_fmt(fmt) "sharedmem_qmi: " fmt

#include <linux/list.h>
#include <linux/module.h>
#include <linux/rwsem.h>
#include <linux/slab.h>
#include <linux/soc/qcom/qmi.h>
#include <linux/workqueue.h>
#include "sharedmem_qmi.h"

#define RFSA_SERVICE_ID_V01			0x1C
#define RFSA_SERVICE_VERS_V01			0x01
#define RFSA_SERVICE_INSTANCE_NUM		1
#define QMI_RFSA_GET_BUFF_ADDR_REQ_MSG_V01	0x0023
#define QMI_RFSA_GET_BUFF_ADDR_RESP_MSG_V01	0x0023
#define RFSA_GET_BUFF_ADDR_REQ_MSG_MAX_LEN_V01	14
#define RFSA_GET_BUFF_ADDR_RESP_MSG_MAX_LEN_V01	18

struct rfsa_get_buff_addr_req_msg_v01 {
	u32 client_id;
	u32 size;
};

struct rfsa_get_buff_addr_resp_msg_v01 {
	struct qmi_response_type_v01 resp;
	u8 address_valid;
	u64 address;
};

static struct qmi_elem_info rfsa_get_buff_addr_req_msg_v01_ei[] = {
	{
		.data_type	= QMI_UNSIGNED_4_BYTE,
		.elem_len	= 1,
		.elem_size	= sizeof(u32),
		.array_type	= NO_ARRAY,
		.tlv_type	= 0x01,
		.offset		= offsetof(struct rfsa_get_buff_addr_req_msg_v01,
					   client_id),
	},
	{
		.data_type	= QMI_UNSIGNED_4_BYTE,
		.elem_len	= 1,
		.elem_size	= sizeof(u32),
		.array_type	= NO_ARRAY,
		.tlv_type	= 0x02,
		.offset		= offsetof(struct rfsa_get_buff_addr_req_msg_v01,
					   size),
	},
	{
		.data_type	= QMI_EOTI,
		.array_type	= NO_ARRAY,
		.tlv_type	= QMI_COMMON_TLV_TYPE,
	},
};

static struct qmi_elem_info rfsa_get_buff_addr_resp_msg_v01_ei[] = {
	{
		.data_type	= QMI_STRUCT,
		.elem_len	= 1,
		.elem_size	= sizeof(struct qmi_response_type_v01),
		.array_type	= NO_ARRAY,
		.tlv_type	= 0x02,
		.offset		= offsetof(struct rfsa_get_buff_addr_resp_msg_v01,
					   resp),
		.ei_array	= qmi_response_type_v01_ei,
	},
	{
		.data_type	= QMI_OPT_FLAG,
		.elem_len	= 1,
		.elem_size	= sizeof(u8),
		.array_type	= NO_ARRAY,
		.tlv_type	= 0x10,
		.offset		= offsetof(struct rfsa_get_buff_addr_resp_msg_v01,
					   address_valid),
	},
	{
		.data_type	= QMI_UNSIGNED_8_BYTE,
		.elem_len	= 1,
		.elem_size	= sizeof(u64),
		.array_type	= NO_ARRAY,
		.tlv_type	= 0x10,
		.offset		= offsetof(struct rfsa_get_buff_addr_resp_msg_v01,
					   address),
	},
	{
		.data_type	= QMI_EOTI,
		.array_type	= NO_ARRAY,
		.tlv_type	= QMI_COMMON_TLV_TYPE,
	},
};

struct shared_addr_entry {
	struct list_head node;
	struct sharemem_qmi_entry entry;
	u64 request_count;
};

static LIST_HEAD(sharedmem_list);
static DECLARE_RWSEM(sharedmem_list_lock);
static struct qmi_handle *sharedmem_qmi_handle;

void sharedmem_qmi_add_entry(struct sharemem_qmi_entry *qmi_entry)
{
	struct shared_addr_entry *e = kzalloc(sizeof(*e), GFP_KERNEL);

	if (!e)
		return;
	e->entry = *qmi_entry;
	down_write(&sharedmem_list_lock);
	list_add_tail(&e->node, &sharedmem_list);
	up_write(&sharedmem_list_lock);
}

static int get_buffer_for_client(u32 id, u32 size, u64 *address)
{
	struct shared_addr_entry *e;
	int ret = -ENOENT;

	down_read(&sharedmem_list_lock);
	list_for_each_entry(e, &sharedmem_list, node) {
		if (e->entry.client_id != id)
			continue;
		if (e->entry.size < size) {
			pr_err("request of 0x%x bytes too large for client %u\n",
			       size, id);
			ret = -ENOMEM;
			break;
		}
		*address = e->entry.address;
		e->request_count++;
		ret = 0;
		break;
	}
	up_read(&sharedmem_list_lock);

	if (ret == -ENOENT)
		pr_err("unknown client id %u\n", id);
	return ret;
}

static void rfsa_get_buff_addr_handler(struct qmi_handle *handle,
				       struct sockaddr_qrtr *sq,
				       struct qmi_txn *txn,
				       const void *decoded)
{
	const struct rfsa_get_buff_addr_req_msg_v01 *req = decoded;
	struct rfsa_get_buff_addr_resp_msg_v01 resp = { };
	u64 address = 0;
	int ret;

	ret = get_buffer_for_client(req->client_id, req->size, &address);
	if (!ret && address) {
		resp.address_valid = 1;
		resp.address = address;
		resp.resp.result = QMI_RESULT_SUCCESS_V01;
		resp.resp.error = QMI_ERR_NONE_V01;
	} else {
		resp.resp.result = QMI_RESULT_FAILURE_V01;
		resp.resp.error = QMI_ERR_INTERNAL_V01;
	}
	pr_info("client %u asked for 0x%x bytes: %s\n", req->client_id,
		req->size, resp.address_valid ? "sent address" : "failed");

	ret = qmi_send_response(handle, sq, txn,
				QMI_RFSA_GET_BUFF_ADDR_RESP_MSG_V01,
				RFSA_GET_BUFF_ADDR_RESP_MSG_MAX_LEN_V01,
				rfsa_get_buff_addr_resp_msg_v01_ei, &resp);
	if (ret < 0)
		pr_err("failed to send response: %d\n", ret);
}

static struct qmi_msg_handler sharedmem_qmi_handlers[] = {
	{
		.type		= QMI_REQUEST,
		.msg_id		= QMI_RFSA_GET_BUFF_ADDR_REQ_MSG_V01,
		.ei		= rfsa_get_buff_addr_req_msg_v01_ei,
		.decoded_size	= sizeof(struct rfsa_get_buff_addr_req_msg_v01),
		.fn		= rfsa_get_buff_addr_handler,
	},
	{}
};

static int sharedmem_qmi_register(void)
{
	int ret;

	sharedmem_qmi_handle = kzalloc(sizeof(*sharedmem_qmi_handle),
				       GFP_KERNEL);
	if (!sharedmem_qmi_handle)
		return -ENOMEM;

	ret = qmi_handle_init(sharedmem_qmi_handle,
			      RFSA_GET_BUFF_ADDR_REQ_MSG_MAX_LEN_V01,
			      NULL, sharedmem_qmi_handlers);
	if (ret < 0)
		goto free;

	ret = qmi_add_server(sharedmem_qmi_handle, RFSA_SERVICE_ID_V01,
			     RFSA_SERVICE_VERS_V01, RFSA_SERVICE_INSTANCE_NUM);
	if (ret < 0)
		goto release;

	pr_info("RFSA service registered\n");
	return 0;

release:
	qmi_handle_release(sharedmem_qmi_handle);
free:
	kfree(sharedmem_qmi_handle);
	sharedmem_qmi_handle = NULL;
	return ret;
}

/*
 * The QRTR socket family may register after this driver, so retry the
 * registration from a work item for a while instead of failing.
 */
static void sharedmem_qmi_init_worker(struct work_struct *work);
static DECLARE_DELAYED_WORK(sharedmem_qmi_init_work, sharedmem_qmi_init_worker);
static int sharedmem_qmi_tries;

static void sharedmem_qmi_init_worker(struct work_struct *work)
{
	int ret = sharedmem_qmi_register();

	if (ret == -EAFNOSUPPORT && ++sharedmem_qmi_tries < 30) {
		schedule_delayed_work(&sharedmem_qmi_init_work, HZ);
		return;
	}
	if (ret)
		pr_err("failed to register the RFSA service: %d\n", ret);
}

int sharedmem_qmi_init(void)
{
	schedule_delayed_work(&sharedmem_qmi_init_work, 0);
	return 0;
}

void sharedmem_qmi_exit(void)
{
	struct shared_addr_entry *e, *tmp;

	cancel_delayed_work_sync(&sharedmem_qmi_init_work);
	if (sharedmem_qmi_handle) {
		qmi_handle_release(sharedmem_qmi_handle);
		kfree(sharedmem_qmi_handle);
		sharedmem_qmi_handle = NULL;
	}
	down_write(&sharedmem_list_lock);
	list_for_each_entry_safe(e, tmp, &sharedmem_list, node) {
		list_del(&e->node);
		kfree(e);
	}
	up_write(&sharedmem_list_lock);
}

/*
 * Copyright (c) 2004-2011 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef HIF_H
#define HIF_H

#include "common.h"
#include "core.h"

#include <linux/scatterlist.h>

#define BUS_REQUEST_MAX_NUM                64
#define HIF_MBOX_BLOCK_SIZE                128
#define HIF_MBOX0_BLOCK_SIZE               1

#define HIF_DMA_BUFFER_SIZE (32 * 1024)
#define CMD53_FIXED_ADDRESS 1
#define CMD53_INCR_ADDRESS  2

#define MAX_SCATTER_REQUESTS             4
#define MAX_SCATTER_ENTRIES_PER_REQ      16
#define MAX_SCATTER_REQ_TRANSFER_SIZE    (32 * 1024)

#define MANUFACTURER_ID_AR6003_BASE        0x300
    /* SDIO manufacturer ID and Codes */
#define MANUFACTURER_ID_ATH6KL_BASE_MASK     0xFF00
#define MANUFACTURER_CODE                  0x271	/* Atheros */

/* Mailbox address in SDIO address space */
#define HIF_MBOX_BASE_ADDR                 0x800
#define HIF_MBOX_WIDTH                     0x800

#define HIF_MBOX_END_ADDR  (HTC_MAILBOX_NUM_MAX * HIF_MBOX_WIDTH - 1)

/* version 1 of the chip has only a 12K extended mbox range */
#define HIF_MBOX0_EXT_BASE_ADDR  0x4000
#define HIF_MBOX0_EXT_WIDTH      (12*1024)

/* GMBOX addresses */
#define HIF_GMBOX_BASE_ADDR                0x7000
#define HIF_GMBOX_WIDTH                    0x4000

/* interrupt mode register */
#define CCCR_SDIO_IRQ_MODE_REG         0xF0

/* mode to enable special 4-bit interrupt assertion without clock */
#define SDIO_IRQ_MODE_ASYNC_4BIT_IRQ   (1 << 0)

struct bus_request {
	struct list_head list;

	/* request data */
	u32 address;

	u8 *buffer;
	u32 length;
	u32 request;
	struct htc_packet *packet;
	int status;

	/* this is a scatter request */
	struct hif_scatter_req *scat_req;
};

/* direction of transfer (read/write) */
#define HIF_READ                    0x00000001
#define HIF_WRITE                   0x00000002
#define HIF_DIR_MASK                (HIF_READ | HIF_WRITE)

/*
 *     emode - This indicates the whether the command is to be executed in a
 *             blocking or non-blocking fashion (HIF_SYNCHRONOUS/
 *             HIF_ASYNCHRONOUS). The read/write data paths in HTC have been
 *             implemented using the asynchronous mode allowing the the bus
 *             driver to indicate the completion of operation through the
 *             registered callback routine. The requirement primarily comes
 *             from the contexts these operations get called from (a driver's
 *             transmit context or the ISR context in case of receive).
 *             Support for both of these modes is essential.
 */
#define HIF_SYNCHRONOUS             0x00000010
#define HIF_ASYNCHRONOUS            0x00000020
#define HIF_EMODE_MASK              (HIF_SYNCHRONOUS | HIF_ASYNCHRONOUS)

/*
 *     dmode - An interface may support different kinds of commands based on
 *             the tradeoff between the amount of data it can carry and the
 *             setup time. Byte and Block modes are supported (HIF_BYTE_BASIS/
 *             HIF_BLOCK_BASIS). In case of latter, the data is rounded off
 *             to the nearest block size by padding. The size of the block is
 *             configurable at compile time using the HIF_BLOCK_SIZE and is
 *             negotiated with the target during initialization after the
 *             ATH6KL interrupts are enabled.
 */
#define HIF_BYTE_BASIS              0x00000040
#define HIF_BLOCK_BASIS             0x00000080
#define HIF_DMODE_MASK              (HIF_BYTE_BASIS | HIF_BLOCK_BASIS)

/*
 *     amode - This indicates if the address has to be incremented on ATH6KL
 *             after every read/write operation (HIF?FIXED_ADDRESS/
 *             HIF_INCREMENTAL_ADDRESS).
 */
#define HIF_FIXED_ADDRESS           0x00000100
#define HIF_INCREMENTAL_ADDRESS     0x00000200
#define HIF_AMODE_MASK		  (HIF_FIXED_ADDRESS | HIF_INCREMENTAL_ADDRESS)

#define HIF_WR_ASYNC_BYTE_INC					\
	(HIF_WRITE | HIF_ASYNCHRONOUS |				\
	 HIF_BYTE_BASIS | HIF_INCREMENTAL_ADDRESS)

#define HIF_WR_ASYNC_BLOCK_INC					\
	(HIF_WRITE | HIF_ASYNCHRONOUS |				\
	 HIF_BLOCK_BASIS | HIF_INCREMENTAL_ADDRESS)

#define HIF_WR_SYNC_BYTE_FIX					\
	(HIF_WRITE | HIF_SYNCHRONOUS |				\
	 HIF_BYTE_BASIS | HIF_FIXED_ADDRESS)

#define HIF_WR_SYNC_BYTE_INC					\
	(HIF_WRITE | HIF_SYNCHRONOUS |				\
	 HIF_BYTE_BASIS | HIF_INCREMENTAL_ADDRESS)

#define HIF_WR_SYNC_BLOCK_INC					\
	(HIF_WRITE | HIF_SYNCHRONOUS |				\
	 HIF_BLOCK_BASIS | HIF_INCREMENTAL_ADDRESS)

#define HIF_RD_SYNC_BYTE_INC						\
	(HIF_READ | HIF_SYNCHRONOUS |					\
	 HIF_BYTE_BASIS | HIF_INCREMENTAL_ADDRESS)

#define HIF_RD_SYNC_BYTE_FIX						\
	(HIF_READ | HIF_SYNCHRONOUS |					\
	 HIF_BYTE_BASIS | HIF_FIXED_ADDRESS)

#define HIF_RD_ASYNC_BLOCK_FIX						\
	(HIF_READ | HIF_ASYNCHRONOUS |					\
	 HIF_BLOCK_BASIS | HIF_FIXED_ADDRESS)

#define HIF_RD_SYNC_BLOCK_FIX						\
	(HIF_READ | HIF_SYNCHRONOUS |					\
	 HIF_BLOCK_BASIS | HIF_FIXED_ADDRESS)

struct hif_scatter_item {
	u8 *buf;
	int len;
	struct htc_packet *packet;
};

struct hif_scatter_req {
	struct list_head list;
	/* address for the read/write operation */
	u32 addr;

	/* request flags */
	u32 req;

	/* total length of entire transfer */
	u32 len;

	bool virt_scat;

	void (*complete) (struct htc_target *, struct hif_scatter_req *);
	int status;
	int scat_entries;

	struct bus_request *busrequest;
	struct scatterlist *sgentries;

	/* bounce buffer for upper layers to copy to/from */
	u8 *virt_dma_buf;

	struct hif_scatter_item scat_list[1];
};

struct ath6kl_hif_ops {
	int (*read_write_sync)(struct ath6kl *ar, u32 addr, u8 *buf,
			       u32 len, u32 request);
	int (*write_async)(struct ath6kl *ar, u32 address, u8 *buffer,
			   u32 length, u32 request, struct htc_packet *packet);

	void (*irq_enable)(struct ath6kl *ar);
	void (*irq_disable)(struct ath6kl *ar);

	struct hif_scatter_req *(*scatter_req_get)(struct ath6kl *ar);
	void (*scatter_req_add)(struct ath6kl *ar,
				struct hif_scatter_req *s_req);
	int (*enable_scatter)(struct ath6kl *ar);
	int (*scat_req_rw) (struct ath6kl *ar,
			    struct hif_scatter_req *scat_req);
	void (*cleanup_scatter)(struct ath6kl *ar);
	int (*suspend)(struct ath6kl *ar);

	int (*diag_read32)(struct ath6kl *ar, u32 address,
			u32 *value);
	int (*diag_write32)(struct ath6kl *ar, u32 address,
			__le32 value);
	int (*bmi_recv_buf)(struct ath6kl *ar,
			u8 *buf, u32 len, bool want_timeout);
	int (*bmi_send_buf)(struct ath6kl *ar, u8 *buf, u32 len);
	int (*get_stat)(struct ath6kl *ar, u8 *buf, int buf_len);
	int (*set_max_sche)(struct ath6kl *ar, u32 max_sche_tx, u32 max_sche_rx);
};

#endif

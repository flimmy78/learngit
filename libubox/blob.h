/*
 * blob - library for generating/parsing tagged binary data
 *
 * Copyright (C) 2010 Felix Fietkau <nbd@openwrt.org>
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
 *
 * 功能描述：实现二进制格式传递数据，核心是创建一个可以在任何socket上传递的blob,数据被抽象为 类型-值 的形式，支持数据嵌套
 * 备注：blob.h是blobmsg.h的底层封装
 */

#ifndef _BLOB_H__
#define _BLOB_H__

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "utils.h"

#define BLOB_COOKIE		0x01234567

// blob层的数据类型(跟上层blobmsg相比，主要少了array和table类型)
enum {
	BLOB_ATTR_UNSPEC,
	BLOB_ATTR_NESTED,
	BLOB_ATTR_BINARY,
	BLOB_ATTR_STRING,
	BLOB_ATTR_INT8,
	BLOB_ATTR_INT16,
	BLOB_ATTR_INT32,
	BLOB_ATTR_INT64,
	BLOB_ATTR_LAST
};

#define BLOB_ATTR_ID_MASK  0x7f000000   // 属性空间ID号，24～30位
#define BLOB_ATTR_ID_SHIFT 24
#define BLOB_ATTR_LEN_MASK 0x00ffffff   // 属性空间数据区长度，低0～23位
#define BLOB_ATTR_ALIGN    4
#define BLOB_ATTR_EXTENDED 0x80000000   // 属性空间扩展标志，31位(用于标记上层是否存在消息头blobmsg_hdr)

// blob的属性空间
struct blob_attr {
	uint32_t id_len;    // [0-23]:数据区长度(包含了本身blob_attr结构长度);[24-30]:ID;[31]:扩展标志
	char data[];        // 指向数据区(即上层的消息)
} __packed;

struct blob_attr_info {
	unsigned int type;
	unsigned int minlen;
	unsigned int maxlen;
	bool (*validate)(const struct blob_attr_info *, struct blob_attr *);
};

/* blob模块控制块
 * 注意： blob_buf数据使用完后一定要调用blob_buf_free销毁，特别是当定义成局部变量的blob_buf，切记！！
 */
struct blob_buf {
	struct blob_attr *head; // 指向当前嵌套级别的领袖blob的属性空间（属性空间其实是在缓冲区中分配的，刚初始化时head和buf地址重合）
	bool (*grow)(struct blob_buf *buf, int minlen); // 指向blob缓冲区容量调整函数
	int buflen;             // blob缓冲区长度
	void *buf;              // blob缓冲区指针
};

/*
 * blob_data: returns the data pointer for an attribute
 * 返回数据区指针
 */
static inline void *
blob_data(const struct blob_attr *attr)
{
	return (void *) attr->data;
}

/*
 * blob_id: returns the id of an attribute
 * 返回数据类型 
 */
static inline unsigned int
blob_id(const struct blob_attr *attr)
{
	int id = (be32_to_cpu(attr->id_len) & BLOB_ATTR_ID_MASK) >> BLOB_ATTR_ID_SHIFT;
	return id;
}

// 检测扩展标记位，判断上层是否存在消息头
static inline bool
blob_is_extended(const struct blob_attr *attr)
{
	return !!(attr->id_len & cpu_to_be32(BLOB_ATTR_EXTENDED));
}

/*
 * blob_len: returns the length of the attribute's payload
 * 返回数据区payload（即上层消息总长，包含上层消息头（假如存在），不包含本层blob_attr结构长）
 */
static inline unsigned int
blob_len(const struct blob_attr *attr)
{
	return (be32_to_cpu(attr->id_len) & BLOB_ATTR_LEN_MASK) - sizeof(struct blob_attr);
}

/*
 * blob_raw_len: returns the complete length of an attribute (including the header)
 * 返回数据区全长(不带pad)（包含本层blob_attr结构长）
 */
static inline unsigned int
blob_raw_len(const struct blob_attr *attr)
{
	return blob_len(attr) + sizeof(struct blob_attr);
}

/*
 * blob_pad_len: returns the padded length of an attribute (including the header)
 * 返回4字节对齐的数据区全长(不带pad)（包含本层blob_attr结构长）
 */
static inline unsigned int
blob_pad_len(const struct blob_attr *attr)
{
	unsigned int len = blob_raw_len(attr);
	len = (len + BLOB_ATTR_ALIGN - 1) & ~(BLOB_ATTR_ALIGN - 1);
	return len;
}

// 获取uint8_t类型的数据
static inline uint8_t
blob_get_u8(const struct blob_attr *attr)
{
	return *((uint8_t *) attr->data);
}

// 获取uint16_t类型的数据
static inline uint16_t
blob_get_u16(const struct blob_attr *attr)
{
	uint16_t *tmp = (uint16_t*)attr->data;
	return be16_to_cpu(*tmp);
}

// 获取uint32_t类型的数据
static inline uint32_t
blob_get_u32(const struct blob_attr *attr)
{
	uint32_t *tmp = (uint32_t*)attr->data;
	return be32_to_cpu(*tmp);
}

// 获取uint64_t类型的数据
static inline uint64_t
blob_get_u64(const struct blob_attr *attr)
{
	uint32_t *ptr = (uint32_t *) blob_data(attr);
	uint64_t tmp = ((uint64_t) be32_to_cpu(ptr[0])) << 32;
	tmp |= be32_to_cpu(ptr[1]);
	return tmp;
}

// 获取int8_t类型的数据
static inline int8_t
blob_get_int8(const struct blob_attr *attr)
{
	return blob_get_u8(attr);
}

// 获取int16_t类型的数据
static inline int16_t
blob_get_int16(const struct blob_attr *attr)
{
	return blob_get_u16(attr);
}

// 获取int32_t类型的数据
static inline int32_t
blob_get_int32(const struct blob_attr *attr)
{
	return blob_get_u32(attr);
}

// 获取int64_t类型的数据
static inline int64_t
blob_get_int64(const struct blob_attr *attr)
{
	return blob_get_u64(attr);
}

// 获取字符串类型的数据
static inline const char *
blob_get_string(const struct blob_attr *attr)
{
	return attr->data;
}

// 获取下一个blob的属性空间
static inline struct blob_attr *
blob_next(const struct blob_attr *attr)
{
	return (struct blob_attr *) ((char *) attr + blob_pad_len(attr));
}

extern void blob_fill_pad(struct blob_attr *attr);
extern void blob_set_raw_len(struct blob_attr *attr, unsigned int len);
extern bool blob_attr_equal(const struct blob_attr *a1, const struct blob_attr *a2);
extern int blob_buf_init(struct blob_buf *buf, int id);
extern void blob_buf_free(struct blob_buf *buf);
extern bool blob_buf_grow(struct blob_buf *buf, int required);
extern struct blob_attr *blob_new(struct blob_buf *buf, int id, int payload);
extern void *blob_nest_start(struct blob_buf *buf, int id);
extern void blob_nest_end(struct blob_buf *buf, void *cookie);
extern struct blob_attr *blob_put(struct blob_buf *buf, int id, const void *ptr, unsigned int len);
extern bool blob_check_type(const void *ptr, unsigned int len, int type);
extern int blob_parse(struct blob_attr *attr, struct blob_attr **data, const struct blob_attr_info *info, int max);
extern struct blob_attr *blob_memdup(struct blob_attr *attr);
extern struct blob_attr *blob_put_raw(struct blob_buf *buf, const void *ptr, unsigned int len);

static inline struct blob_attr *
blob_put_string(struct blob_buf *buf, int id, const char *str)
{
	return blob_put(buf, id, str, strlen(str) + 1);
}

static inline struct blob_attr *
blob_put_u8(struct blob_buf *buf, int id, uint8_t val)
{
	return blob_put(buf, id, &val, sizeof(val));
}

static inline struct blob_attr *
blob_put_u16(struct blob_buf *buf, int id, uint16_t val)
{
	val = cpu_to_be16(val);
	return blob_put(buf, id, &val, sizeof(val));
}

static inline struct blob_attr *
blob_put_u32(struct blob_buf *buf, int id, uint32_t val)
{
	val = cpu_to_be32(val);
	return blob_put(buf, id, &val, sizeof(val));
}

static inline struct blob_attr *
blob_put_u64(struct blob_buf *buf, int id, uint64_t val)
{
	val = cpu_to_be64(val);
	return blob_put(buf, id, &val, sizeof(val));
}

#define blob_put_int8	blob_put_u8
#define blob_put_int16	blob_put_u16
#define blob_put_int32	blob_put_u32
#define blob_put_int64	blob_put_u64

#define __blob_for_each_attr(pos, attr, rem) \
	for (pos = (void *) attr; \
	     rem > 0 && (blob_pad_len(pos) <= rem) && \
	     (blob_pad_len(pos) >= sizeof(struct blob_attr)); \
	     rem -= blob_pad_len(pos), pos = blob_next(pos))


// 遍历blob中的子blob
#define blob_for_each_attr(pos, attr, rem) \
	for (rem = attr ? blob_len(attr) : 0, \
	     pos = attr ? blob_data(attr) : 0; \
	     rem > 0 && (blob_pad_len(pos) <= rem) && \
	     (blob_pad_len(pos) >= sizeof(struct blob_attr)); \
	     rem -= blob_pad_len(pos), pos = blob_next(pos))


#endif

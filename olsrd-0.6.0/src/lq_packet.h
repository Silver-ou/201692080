
/*
 * The olsr.org Optimized Link-State Routing daemon(olsrd)
 * Copyright (c) 2004, Thomas Lopatic (thomas@lopatic.de)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of olsr.org, olsrd nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Visit http://www.olsr.org for more information.
 *
 * If you find this software useful feel free to make a donation
 * to the project. For more information see the website or contact
 * the copyright holders.
 *
 */

#ifndef _OLSR_LQ_PACKET_H
#define _OLSR_LQ_PACKET_H

#include "olsr_types.h"
#include "packet.h"
#include "mantissa.h"
#include "ipcalc.h"

#define LQ_HELLO_MESSAGE      201
#define LQ_TC_MESSAGE         202

/* deserialized OLSR header */

struct olsr_common {      //olsr 的首部
  uint8_t type;             //消息类型
  olsr_reltime vtime;      //有效时间
  uint16_t size;            //消息大小
  union olsr_ip_addr orig;   //发端地址
  uint8_t ttl;              //跳数
  uint8_t hops;             //经历的条数
  uint16_t seqno;           //消息序列号
};

/* serialized IPv4 OLSR header */        //ipv4的首部

struct olsr_header_v4 {
  uint8_t type;
  uint8_t vtime;
  uint16_t size;
  uint32_t orig;
  uint8_t ttl;
  uint8_t hops;
  uint16_t seqno;
};

/* serialized IPv6 OLSR header */        //ipv6的首部

struct olsr_header_v6 {
  uint8_t type;
  uint8_t vtime;
  uint16_t size;
  unsigned char orig[16];
  uint8_t ttl;
  uint8_t hops;
  uint16_t seqno;
};

/* deserialized LQ_HELLO */        

struct lq_hello_neighbor {            //邻居节点集hello消息的头部 
  uint8_t link_type;                  //链接类型
  uint8_t neigh_type;                 //邻居类型
  union olsr_ip_addr addr;            //地址
  struct lq_hello_neighbor *next;     //将要把hello消息传递给的下一个邻居的节点
  uint32_t linkquality[0];             //链路的质量
};

struct lq_hello_message {             //消息数据包的首部
  struct olsr_common comm;
  olsr_reltime htime;               //hello消息发送间隔
  uint8_t will;                     //指定节点的意愿进行
  struct lq_hello_neighbor *neigh;    //消息传递的下一节点
};

/* serialized LQ_HELLO */
struct lq_hello_info_header {         //107 - 117  共同形成hello消息数据包的头部
  uint8_t link_code;                //链路类型
  uint8_t reserved;                  //保留字段，设置为000000000000
  uint16_t size;                      //链路消息的大小
};

struct lq_hello_header {
  uint16_t reserved;                  //保留字段，设置为00000000
  uint8_t htime;                       //hello消息发送的时间间隔
  uint8_t will;                       //描述一个节点携带网络流量的意愿，默认为willing_default
};

/* deserialized LQ_TC */
struct lq_tc_message {
  struct olsr_common comm;
  union olsr_ip_addr from;
  uint16_t ansn;
  struct tc_mpr_addr *neigh;
};

/* serialized LQ_TC */

struct lq_tc_header {
  uint16_t ansn;
  uint8_t lower_border;
  uint8_t upper_border;
};

static INLINE void
pkt_get_u8(const uint8_t ** p, uint8_t * var)
{
  *var = *(const uint8_t *)(*p);
  *p += sizeof(uint8_t);
}
static INLINE void
pkt_get_u16(const uint8_t ** p, uint16_t * var)
{
  *var = ntohs(**((const uint16_t **)p));
  *p += sizeof(uint16_t);
}
static INLINE void
pkt_get_u32(const uint8_t ** p, uint32_t * var)
{
  *var = ntohl(**((const uint32_t **)p));
  *p += sizeof(uint32_t);
}
static INLINE void
pkt_get_s8(const uint8_t ** p, int8_t * var)
{
  *var = *(const int8_t *)(*p);
  *p += sizeof(int8_t);
}
static INLINE void
pkt_get_s16(const uint8_t ** p, int16_t * var)
{
  *var = ntohs(**((const int16_t **)p));
  *p += sizeof(int16_t);
}
static INLINE void
pkt_get_s32(const uint8_t ** p, int32_t * var)
{
  *var = ntohl(**((const int32_t **)p));
  *p += sizeof(int32_t);
}
static INLINE void
pkt_get_reltime(const uint8_t ** p, olsr_reltime * var)
{
  *var = me_to_reltime(**p);
  *p += sizeof(uint8_t);
}
static INLINE void
pkt_get_ipaddress(const uint8_t ** p, union olsr_ip_addr *var)
{
  memcpy(var, *p, olsr_cnf->ipsize);
  *p += olsr_cnf->ipsize;
}
static INLINE void
pkt_get_prefixlen(const uint8_t ** p, uint8_t * var)
{
  *var = netmask_to_prefix(*p, olsr_cnf->ipsize);
  *p += olsr_cnf->ipsize;
}

static INLINE void
pkt_ignore_u8(const uint8_t ** p)
{
  *p += sizeof(uint8_t);
}
static INLINE void
pkt_ignore_u16(const uint8_t ** p)
{
  *p += sizeof(uint16_t);
}
static INLINE void
pkt_ignore_u32(const uint8_t ** p)
{
  *p += sizeof(uint32_t);
}
static INLINE void
pkt_ignore_s8(const uint8_t ** p)
{
  *p += sizeof(int8_t);
}
static INLINE void
pkt_ignore_s16(const uint8_t ** p)
{
  *p += sizeof(int16_t);
}
static INLINE void
pkt_ignore_s32(const uint8_t ** p)
{
  *p += sizeof(int32_t);
}
static INLINE void
pkt_ignore_ipaddress(const uint8_t ** p)
{
  *p += olsr_cnf->ipsize;
}
static INLINE void
pkt_ignore_prefixlen(const uint8_t ** p)
{
  *p += olsr_cnf->ipsize;
}

static INLINE void
pkt_put_u8(uint8_t ** p, uint8_t var)
{
  **((uint8_t **)p) = var;
  *p += sizeof(uint8_t);
}
static INLINE void
pkt_put_u16(uint8_t ** p, uint16_t var)
{
  **((uint16_t **)p) = htons(var);
  *p += sizeof(uint16_t);
}
static INLINE void
pkt_put_u32(uint8_t ** p, uint32_t var)
{
  **((uint32_t **)p) = htonl(var);
  *p += sizeof(uint32_t);
}
static INLINE void
pkt_put_s8(uint8_t ** p, int8_t var)
{
  **((int8_t **)p) = var;
  *p += sizeof(int8_t);
}
static INLINE void
pkt_put_s16(uint8_t ** p, int16_t var)
{
  **((int16_t **)p) = htons(var);
  *p += sizeof(int16_t);
}
static INLINE void
pkt_put_s32(uint8_t ** p, int32_t var)
{
  **((int32_t **)p) = htonl(var);
  *p += sizeof(int32_t);
}
static INLINE void
pkt_put_reltime(uint8_t ** p, olsr_reltime var)
{
  **p = reltime_to_me(var);
  *p += sizeof(uint8_t);
}
static INLINE void
pkt_put_ipaddress(uint8_t ** p, const union olsr_ip_addr *var)
{
  memcpy(*p, var, olsr_cnf->ipsize);
  *p += olsr_cnf->ipsize;
}

void olsr_output_lq_hello(void *para);

void olsr_output_lq_tc(void *para);

void olsr_input_lq_hello(union olsr_message *ser, struct interface *inif, union olsr_ip_addr *from);

extern bool lq_tc_pending;

#endif

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

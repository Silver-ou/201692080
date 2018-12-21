
/*
 * The olsr.org Optimized Link-State Routing daemon(olsrd)
 * Copyright (c) 2004, Andreas Tonnesen(andreto@olsr.org)
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
//关于 MPR 的一些操作
#include "ipcalc.h"
#include "defs.h"
#include "mpr.h"
#include "two_hop_neighbor_table.h"
#include "olsr.h"
#include "neighbor_table.h"
#include "scheduler.h"
#include "net_olsr.h"

/* Begin:
 * Prototypes for internal functions
 */

static uint16_t add_will_always_nodes(void);

static void olsr_optimize_mpr_set(void);

static void olsr_clear_mprs(void);

static void olsr_clear_two_hop_processed(void);

static struct neighbor_entry *olsr_find_maximum_covered(int);

static uint16_t olsr_calculate_two_hop_neighbors(void);

static int olsr_check_mpr_changes(void);

static int olsr_chosen_mpr(struct neighbor_entry *, uint16_t *);

static struct neighbor_2_list_entry *olsr_find_2_hop_neighbors_with_1_link(int);

/* End:
 * Prototypes for internal functions
 */

/**
 *Find all 2 hop neighbors with 1 link
 *connecting them to us trough neighbors
 *with a given willingness.
 *
 *@param willingness the willigness of the neighbors
 *
 *@return a linked list of allocated neighbor_2_list_entry structures
 */
static struct neighbor_2_list_entry *          //查找存在一条连接的两跳邻居节点链表
olsr_find_2_hop_neighbors_with_1_link(int willingness)
{

  uint8_t idx;
  struct neighbor_2_list_entry *two_hop_list_tmp = NULL;
  struct neighbor_2_list_entry *two_hop_list = NULL;
  struct neighbor_entry *dup_neighbor;                     //记录在邻居节点集合中已经存在的邻居节点
  struct neighbor_2_entry *two_hop_neighbor = NULL;        //记录一个两跳邻居节点的局部变量

  for (idx = 0; idx < HASHSIZE; idx++) {             
    //遍历两跳邻居表two_hop_neighbortable
    for (two_hop_neighbor = two_hop_neighbortable[idx].next; two_hop_neighbor != &two_hop_neighbortable[idx];
         two_hop_neighbor = two_hop_neighbor->next) {

      //two_hop_neighbor->neighbor_2_state=0;
      //two_hop_neighbor->mpr_covered_count = 0;

      dup_neighbor = olsr_lookup_neighbor_table(&two_hop_neighbor->neighbor_2_addr);

      if ((dup_neighbor != NULL) && (dup_neighbor->status != NOT_SYM)) {

        OLSR_PRINTF(1, "(1)Skipping 2h neighbor %s - already 1hop\n", olsr_ip_to_string(&buf, &two_hop_neighbor->neighbor_2_addr));

        continue;
      }

      if (two_hop_neighbor->neighbor_2_pointer == 1) {        //寻找在邻居表中已经存在的邻居地址neighbor_2_addr
        if ((two_hop_neighbor->neighbor_2_nblist.next->neighbor->willingness == willingness)
            && (two_hop_neighbor->neighbor_2_nblist.next->neighbor->status == SYM)) {
          two_hop_list_tmp = olsr_malloc(sizeof(struct neighbor_2_list_entry), "MPR two hop list");
          //如果存在，且该邻居节点与本节点不是非对称的节点忽略
          OLSR_PRINTF(1, "ONE LINK ADDING %s\n", olsr_ip_to_string(&buf, &two_hop_neighbor->neighbor_2_addr));  

          /* Only queue one way here */
          two_hop_list_tmp->neighbor_2 = two_hop_neighbor;  //如果不存在且该两跳邻居节点只有一个邻居节点，将两跳邻居节点添加到两跳邻居节点链表中

          two_hop_list_tmp->next = two_hop_list;       //two_hop_list为两跳邻居节点链表

          two_hop_list = two_hop_list_tmp;
        }
      }

    }

  }

  return (two_hop_list_tmp);
}

/**
 *This function processes the chosen MPRs and updates the counters
 *used in calculations
 */
static int    //用来处理已经选你定的MPR节点，对mpr计时器的更新
olsr_chosen_mpr(struct neighbor_entry *one_hop_neighbor, uint16_t * two_hop_covered_count)
{
  struct neighbor_list_entry *the_one_hop_list;
  struct neighbor_2_list_entry *second_hop_entries;
  struct neighbor_entry *dup_neighbor;
  uint16_t count;
  struct ipaddr_str buf;
  count = *two_hop_covered_count;

  OLSR_PRINTF(1, "Setting %s as MPR\n", olsr_ip_to_string(&buf, &one_hop_neighbor->neighbor_main_addr));

  //printf("PRE COUNT: %d\n\n", count);

  one_hop_neighbor->is_mpr = true;      //NBS_MPR;

  for (second_hop_entries = one_hop_neighbor->neighbor_2_list.next; second_hop_entries != &one_hop_neighbor->neighbor_2_list;
       second_hop_entries = second_hop_entries->next) {             //对second_hop_etries进行遍历
    dup_neighbor = olsr_lookup_neighbor_table(&second_hop_entries->neighbor_2->neighbor_2_addr);

    if ((dup_neighbor != NULL) && (dup_neighbor->status == SYM)) {     //忽略已经存在的邻居节点
      //OLSR_PRINTF(7, "(2)Skipping 2h neighbor %s - already 1hop\n", olsr_ip_to_string(&buf, &second_hop_entries->neighbor_2->neighbor_2_addr));
      continue;
    }
    //      if(!second_hop_entries->neighbor_2->neighbor_2_state)
    //if(second_hop_entries->neighbor_2->mpr_covered_count < olsr_cnf->mpr_coverage)
    //{
    /*
       Now the neighbor is covered by this mpr
     */
    second_hop_entries->neighbor_2->mpr_covered_count++;         //增加两跳邻居节点被覆盖的mpr的数量
    the_one_hop_list = second_hop_entries->neighbor_2->neighbor_2_nblist.next;     //将两跳邻居节点的邻居节点赋给the_one_hop_list

    //OLSR_PRINTF(1, "[%s](%x) has coverage %d\n", olsr_ip_to_string(&buf, &second_hop_entries->neighbor_2->neighbor_2_addr), second_hop_entries->neighbor_2, second_hop_entries->neighbor_2->mpr_covered_count);

    if (second_hop_entries->neighbor_2->mpr_covered_count >= olsr_cnf->mpr_coverage)  //如果两跳节点的mpr数量多于全局变量的mpr_coverage,将count数量加1
      count++;                                                                        

    while (the_one_hop_list != &second_hop_entries->neighbor_2->neighbor_2_nblist) {

      if ((the_one_hop_list->neighbor->status == SYM)) {                     
        if (second_hop_entries->neighbor_2->mpr_covered_count >= olsr_cnf->mpr_coverage) {
          the_one_hop_list->neighbor->neighbor_2_nocov--;          
        }
      }
      the_one_hop_list = the_one_hop_list->next;
    }

    //}
  }

  //printf("POST COUNT %d\n\n", count);

  *two_hop_covered_count = count;
  return count;

}

/**
 *Find the neighbor that covers the most 2 hop neighbors
 *with a given willingness
 *
 *@param willingness the willingness of the neighbor
 *
 *@return a pointer to the neighbor_entry struct
 */
static struct neighbor_entry *         //找到能够覆盖到最多两跳节点的MPR
olsr_find_maximum_covered(int willingness)
{
  uint16_t maximum;
  struct neighbor_entry *a_neighbor;
  struct neighbor_entry *mpr_candidate = NULL;

  maximum = 0;

  OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {

#if 0
    printf("[%s] nocov: %d mpr: %d will: %d max: %d\n\n", olsr_ip_to_string(&buf, &a_neighbor->neighbor_main_addr),
           a_neighbor->neighbor_2_nocov, a_neighbor->is_mpr, a_neighbor->willingness, maximum);
#endif
    //如果a_neighbor不是mpr，并且覆盖两条节点数据值max小于a_neighbor邻居节点的该节点的数量
    if ((!a_neighbor->is_mpr) && (a_neighbor->willingness == willingness) && (maximum < a_neighbor->neighbor_2_nocov)) {
 
      maximum = a_neighbor->neighbor_2_nocov;        //更新maximum
      mpr_candidate = a_neighbor;                    //将a_neighbor选为mpr候选节点
    }
  }
  OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);

  return mpr_candidate;
}

/**
 *Remove all MPR registrations
 */
static void
olsr_clear_mprs(void)          //清除mpr的记录
{
  struct neighbor_entry *a_neighbor;
  struct neighbor_2_list_entry *two_hop_list;

  OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {

    /* Clear MPR selection. */
    if (a_neighbor->is_mpr) {  //如果是mpr节点
      a_neighbor->was_mpr = true;   //将was_mpr置为真
      a_neighbor->is_mpr = false;   //将is_mpr置为假
    }

    /* Clear two hop neighbors coverage count/ */
    for (two_hop_list = a_neighbor->neighbor_2_list.next; two_hop_list != &a_neighbor->neighbor_2_list;
         two_hop_list = two_hop_list->next) {  //遍历a_neighbor的两跳邻居节点
      two_hop_list->neighbor_2->mpr_covered_count = 0;      //将其数量置于0
    }

  }
  OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);
}

/**
 *Check for changes in the MPR set
 *
 *@return 1 if changes occured 0 if not
 */
static int       //遍历所有节点，查看mpr状态是否发生变化
olsr_check_mpr_changes(void)
{
  struct neighbor_entry *a_neighbor;
  int retval;

  retval = 0;

  OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {

    if (a_neighbor->was_mpr) {
      a_neighbor->was_mpr = false;

      if (!a_neighbor->is_mpr) {
        retval = 1;                        //曾经是mpr，现在不是mpr，则发生改变
      }
    }
  }
  OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);

  return retval;        //变化返回1，没变返回0
}

/**
 *Clears out proccess registration
 *on two hop neighbors
 */
static void
olsr_clear_two_hop_processed(void)
{
  int idx;

  for (idx = 0; idx < HASHSIZE; idx++) {
    struct neighbor_2_entry *neighbor_2;
    for (neighbor_2 = two_hop_neighbortable[idx].next; neighbor_2 != &two_hop_neighbortable[idx]; neighbor_2 = neighbor_2->next) {
      /* Clear */
      neighbor_2->processed = 0;
    }
  }

}

/**
 *This function calculates the number of two hop neighbors
 */
static uint16_t
olsr_calculate_two_hop_neighbors(void)
{
  struct neighbor_entry *a_neighbor, *dup_neighbor;
  struct neighbor_2_list_entry *twohop_neighbors;
  uint16_t count = 0;
  uint16_t n_count = 0;
  uint16_t sum = 0;

  /* Clear 2 hop neighs */
  olsr_clear_two_hop_processed();

  OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {

    if (a_neighbor->status == NOT_SYM) {
      a_neighbor->neighbor_2_nocov = count;
      continue;
    }

    for (twohop_neighbors = a_neighbor->neighbor_2_list.next; twohop_neighbors != &a_neighbor->neighbor_2_list;
         twohop_neighbors = twohop_neighbors->next) {

      dup_neighbor = olsr_lookup_neighbor_table(&twohop_neighbors->neighbor_2->neighbor_2_addr);

      if ((dup_neighbor == NULL) || (dup_neighbor->status != SYM)) {
        n_count++;
        if (!twohop_neighbors->neighbor_2->processed) {
          count++;
          twohop_neighbors->neighbor_2->processed = 1;
        }
      }
    }
    a_neighbor->neighbor_2_nocov = n_count;

    /* Add the two hop count */
    sum += count;

  }
  OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);

  OLSR_PRINTF(3, "Two hop neighbors: %d\n", sum);
  return sum;
}

/**
 * Adds all nodes with willingness set to WILL_ALWAYS
 */
static uint16_t     
add_will_always_nodes(void)                        //添加MPR节点（即will_always节点）
{
  struct neighbor_entry *a_neighbor;
  uint16_t count = 0;

#if 0
  printf("\nAdding WILL ALWAYS nodes....\n");
#endif

  OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {   //将Will_always的节点添加到mpr集中
    struct ipaddr_str buf;
    if ((a_neighbor->status == NOT_SYM) || (a_neighbor->willingness != WILL_ALWAYS)) { //忽略掉非对称的节点和非will_always的节点
      continue;
    }
    olsr_chosen_mpr(a_neighbor, &count);          //添加到mpr集，并返回数量

    OLSR_PRINTF(3, "Adding WILL_ALWAYS: %s\n", olsr_ip_to_string(&buf, &a_neighbor->neighbor_main_addr));

  }
  OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);

#if 0
  OLSR_PRINTF(1, "Count: %d\n", count);
#endif
  return count;
}

/**
 *This function calculates the mpr neighbors
 *@return nada
 */
void
olsr_calculate_mpr(void)
{
  uint16_t two_hop_covered_count;
  uint16_t two_hop_count;
  int i;

  OLSR_PRINTF(3, "\n**RECALCULATING MPR**\n\n");

  olsr_clear_mprs();
  two_hop_count = olsr_calculate_two_hop_neighbors();
  two_hop_covered_count = add_will_always_nodes();

  /*
   *Calculate MPRs based on WILLINGNESS
   */

  for (i = WILL_ALWAYS - 1; i > WILL_NEVER; i--) {
    struct neighbor_entry *mprs;
    struct neighbor_2_list_entry *two_hop_list = olsr_find_2_hop_neighbors_with_1_link(i);

    while (two_hop_list != NULL) {
      struct neighbor_2_list_entry *tmp;
      //printf("CHOSEN FROM 1 LINK\n");
      if (!two_hop_list->neighbor_2->neighbor_2_nblist.next->neighbor->is_mpr)
        olsr_chosen_mpr(two_hop_list->neighbor_2->neighbor_2_nblist.next->neighbor, &two_hop_covered_count);
      tmp = two_hop_list;
      two_hop_list = two_hop_list->next;;
      free(tmp);
    }

    if (two_hop_covered_count >= two_hop_count) {
      i = WILL_NEVER;
      break;
    }
    //printf("two hop covered count: %d\n", two_hop_covered_count);

    while ((mprs = olsr_find_maximum_covered(i)) != NULL) {
      //printf("CHOSEN FROM MAXCOV\n");
      olsr_chosen_mpr(mprs, &two_hop_covered_count);

      if (two_hop_covered_count >= two_hop_count) {
        i = WILL_NEVER;
        break;
      }

    }
  }

  /*
     increment the mpr sequence number
   */
  //neighbortable.neighbor_mpr_seq++;

  /* Optimize selection */
  olsr_optimize_mpr_set();

  if (olsr_check_mpr_changes()) {
    OLSR_PRINTF(3, "CHANGES IN MPR SET\n");
    if (olsr_cnf->tc_redundancy > 0)
      signal_link_changes(true);
  }

}

/**
 *Optimize MPR set by removing all entries
 *where all 2 hop neighbors actually is
 *covered by enough MPRs already
 *Described in RFC3626 section 8.3.1
 *point 5
 *
 *@return nada
 */
static void
olsr_optimize_mpr_set(void)
{
  struct neighbor_entry *a_neighbor, *dup_neighbor;
  struct neighbor_2_list_entry *two_hop_list;
  int i, removeit;

#if 0
  printf("\n**MPR OPTIMIZING**\n\n");
#endif

  for (i = WILL_NEVER + 1; i < WILL_ALWAYS; i++) {

    OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {

      if (a_neighbor->willingness != i) {
        continue;
      }

      if (a_neighbor->is_mpr) {
        removeit = 1;

        for (two_hop_list = a_neighbor->neighbor_2_list.next; two_hop_list != &a_neighbor->neighbor_2_list;
             two_hop_list = two_hop_list->next) {

          dup_neighbor = olsr_lookup_neighbor_table(&two_hop_list->neighbor_2->neighbor_2_addr);

          if ((dup_neighbor != NULL) && (dup_neighbor->status != NOT_SYM)) {
            continue;
          }
          //printf("\t[%s] coverage %d\n", olsr_ip_to_string(&buf, &two_hop_list->neighbor_2->neighbor_2_addr), two_hop_list->neighbor_2->mpr_covered_count);
          /* Do not remove if we find a entry which need this MPR */
          if (two_hop_list->neighbor_2->mpr_covered_count <= olsr_cnf->mpr_coverage) {
            removeit = 0;
          }
        }

        if (removeit) {
          struct ipaddr_str buf;
          OLSR_PRINTF(3, "MPR OPTIMIZE: removiong mpr %s\n\n", olsr_ip_to_string(&buf, &a_neighbor->neighbor_main_addr));
          a_neighbor->is_mpr = false;
        }
      }
    } OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);
  }
}

void
olsr_print_mpr_set(void)
{
#ifndef NODEBUG
  /* The whole function makes no sense without it. */
  struct neighbor_entry *a_neighbor;

  OLSR_PRINTF(1, "MPR SET: ");

  OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {

    /*
     * Remove MPR settings
     */
    if (a_neighbor->is_mpr) {
      struct ipaddr_str buf;
      OLSR_PRINTF(1, "[%s] ", olsr_ip_to_string(&buf, &a_neighbor->neighbor_main_addr));
    }
  } OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);

  OLSR_PRINTF(1, "\n");
#endif
}

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */

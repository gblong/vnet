
#ifndef __BLOCKING_QUEUE_H
#define __BLOCKING_QUEUE_H

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/slab.h>

struct blocking_queue {
  struct list_head list;
  spinlock_t lock;
  wait_queue_head_t wait;
  size_t size;
  size_t capacity;
  char name[20];
};

struct blocking_queue_item {
  struct list_head list;
  void* data;
};

void blocking_queue_init(struct blocking_queue* blocking_queue, size_t capacity, const char* name);
void blocking_queue_clean(struct blocking_queue* blocking_queue);
void* blocking_queue_get(struct blocking_queue* blocking_queue, int nonblock);
int blocking_queue_put(struct blocking_queue* blocking_queue, void* data);

#endif

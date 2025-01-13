
#include "blocking_queue.h"

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/sched.h>
#include <linux/skbuff.h>
#include <linux/string.h>
#include <linux/types.h>

void blocking_queue_init(struct blocking_queue* blocking_queue, size_t capacity,
                         const char* name) {
  size_t n = 0;
  INIT_LIST_HEAD(&blocking_queue->list);
  spin_lock_init(&blocking_queue->lock);
  init_waitqueue_head(&blocking_queue->wait);
  blocking_queue->size = 0;
  blocking_queue->capacity = capacity;
  n = sizeof(blocking_queue->name) / sizeof(blocking_queue->name[0]);
  strncpy(blocking_queue->name, name, n);
  printk(KERN_INFO "%s: blocking queue initialized.\n", blocking_queue->name);
}

void blocking_queue_clean(struct blocking_queue* blocking_queue) {
  size_t size = blocking_queue->size;
  struct blocking_queue_item *entry, *tmp;
  list_for_each_entry_safe(entry, tmp, &blocking_queue->list, list) {
    list_del(&entry->list);
    kfree(entry);
  }
  blocking_queue->size = 0;
  printk(KERN_INFO "%s: %ld items removed from blocking queue.\n",
         blocking_queue->name, size);
}

int blocking_queue_empty(struct blocking_queue* blocking_queue) {
  return blocking_queue->size == 0;
}

int blocking_queue_put(struct blocking_queue* blocking_queue, void* data) {
  int queue_full = 0;
  struct blocking_queue_item* item =
      kmalloc(sizeof(struct blocking_queue_item), GFP_KERNEL);
  if (item == NULL) {
    printk(KERN_ERR "%s: blocking queue kmalloc failed\n",
           blocking_queue->name);
    return -1;
  }

  item->data = data;

  spin_lock(&blocking_queue->lock);
  if (blocking_queue->size < blocking_queue->capacity) {
    list_add_tail(&item->list, &blocking_queue->list);
    blocking_queue->size++;
  } else {
    queue_full = 1;
  }
  spin_unlock(&blocking_queue->lock);

  if (queue_full == 1) {
    printk(KERN_WARNING "%s: blocking queue is full, capacity: %lu\n",
           blocking_queue->name, blocking_queue->capacity);
    return 0;
  }

  wake_up_interruptible(&blocking_queue->wait);
  return 1;
}

void* blocking_queue_get(struct blocking_queue* blocking_queue, int nonblock) {
  struct blocking_queue_item* item;
  void* data;
  struct list_head* front;

  DECLARE_WAITQUEUE(wait, current);
  add_wait_queue(&blocking_queue->wait, &wait);
  for (;;) {
    set_current_state(TASK_INTERRUPTIBLE);
    if (nonblock) break;
    if (signal_pending(current)) break;
    if (blocking_queue->size > 0) break;
    schedule();
  }
  set_current_state(TASK_RUNNING);
  remove_wait_queue(&blocking_queue->wait, &wait);

  {
    spin_lock(&blocking_queue->lock);
    if (blocking_queue->size == 0) {
      spin_unlock(&blocking_queue->lock);
      return NULL;
    }
    front = blocking_queue->list.next;
    item = list_entry(front, struct blocking_queue_item, list);
    data = item->data;
    list_del(front);
    blocking_queue->size--;
    spin_unlock(&blocking_queue->lock);
  }
  kfree(item);

  return data;
}

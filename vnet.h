#ifndef __VNET_H
#define __VNET_H

#include "blocking_queue.h"

#define VNET_COUNT 2

struct vnic;

struct vnet {
  struct blocking_queue blocking_queue;
  struct vnic* vnic;
};

struct vnet* get_vnet(size_t index);

#endif
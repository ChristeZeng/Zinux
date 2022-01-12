#pragma once
#include "defs.h"
#include "types.h"
#include "string.h"
#include "printk.h"
#include "mm.h"

void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm);
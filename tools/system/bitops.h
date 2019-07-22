// bitops.h
// yang.liu@amlogic.com

#ifndef _BITOPTS_H_
#define _BITOPTS_H_

static inline int popcount(unsigned int x) {
	return __builtin_popcount(x);
}

#endif

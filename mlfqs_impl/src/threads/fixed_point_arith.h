#ifndef __THREAD_FIXED_POINT_H
#define __THREAD_FIXED_POINT_H 



typedef int FP_t;    /*declaring Fixed Point ineger type */

/* 16 bit is reserved for the fractional part */

#define FIXED_SHIFT 16

#define FP_CONST (x) ((FP_t) (x << FIXED_SHIFT)) /* Convering x to a fixed point arith value*/

#define FP_ADD (x, y) (x + y)   /* Addition of two fixed-point values */

#define FP_SUB (x, y) (x - y)   /* Subtraction of atwo fixed-point values */

#define FP_MULTIPLICATION (x, y) ((FP_t)(((int64_t) x) * y >> FIXED_SHIFT))

#define FP_DIVISION (x, y) ((FP_t)((((int64_t) x) << FIXED_SHIFT) / y))


#endif  /* fixed_point-arith.h*/
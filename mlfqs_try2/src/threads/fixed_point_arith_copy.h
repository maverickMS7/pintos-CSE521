#ifndef __THREAD_FIXED_POINT_H
#define __THREAD_FIXED_POINT_H 



typedef int FP_t;    /*declaring Fixed Point ineger type */

/* 16 bit is reserved for the fractional part */

#define FIXED_SHIFT 16

#define FP_F 1<<FIXED_SHIFT;

#define FP_TO_INT (x) ((int) x / FIXED_SHIFT)

#define FP_CONST (x) ((FP_t) (x << FIXED_SHIFT)) /* Convering x to a fixed point arith value*/

#define FP_ROUND (x) x >= 0 ? ((int) (x+FP_F / 2) / FP_F) : ((int) (x-FP_F / 2) / FP_F)

#define FP_ADD (x, y) (x + y)   /* Addition of two fixed-point values */

#define FP_SUB (x, y) (x - y)   /* Subtraction of atwo fixed-point values */

#define FP_MUL (x, y) ((FP_t)(((int64_t) x) * y / FP_F))

#define FP_DIV (x, y) ((FP_t)((((int64_t) x) * FP_F / y))



#endif  /* fixed_point-arith.h*/
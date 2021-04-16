#ifndef __INTERNAL_H__
#define __INTERNAL_H__

#define kdebug(format, ...) printk(KERN_DEBUG, format, __VA_ARGS__)
// #define kdebug(format, ...) printk(KERN_DEBUG, "[DEBUG] "##format, __VA_ARGS__)

#endif

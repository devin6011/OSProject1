#include <linux/syscalls.h>
#include <linux/kernel.h>

SYSCALL_DEFINE5(my_printk, int, pid, long, start_time_sec, long, start_time_nsec, long, finish_time_sec, long, finish_time_nsec) {
	return printk(KERN_INFO "[Project1] %d %ld.%09ld %ld.%09ld\n", pid, start_time_sec, start_time_nsec, finish_time_sec, finish_time_nsec);
}

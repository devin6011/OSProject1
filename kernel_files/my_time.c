#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/timekeeping.h>

SYSCALL_DEFINE2(my_time, long*, sec, long*, nsec) {
	struct timespec64 ts;
	ktime_get_real_ts64(&ts);
	*sec = ts.tv_sec, *nsec = ts.tv_nsec;
	return 0;
}

#ifndef __MALI_IOCTL_H
#define __MALI_IOCTL_H

#include <sys/ioctl.h>
#include <stdint.h>

#define KBASE_IOCTL_TYPE 0x80
#define KBASE_IOCTL_EXTRA_TYPE (KBASE_IOCTL_TYPE + 2)

struct kbase_ioctl_version_check {
	uint16_t major;
	uint16_t minor;
};

#define KBASE_IOCTL_VERSION_CHECK \
	_IOWR(KBASE_IOCTL_TYPE, 0, struct kbase_ioctl_version_check)

struct kbase_ioctl_set_flags {
	uint32_t create_flags;
};

#define KBASE_IOCTL_SET_FLAGS \
	_IOW(KBASE_IOCTL_TYPE, 1, struct kbase_ioctl_set_flags)

struct kbase_ioctl_lock_page_info {
  uint64_t cpuaddr;
};

#define KBASE_IOCTL_LOCK_PAGE \
	_IOW(KBASE_IOCTL_EXTRA_TYPE, 8, struct kbase_ioctl_lock_page_info)

#define KBASE_IOCTL_UNLOCK_PAGE \
	_IOW(KBASE_IOCTL_EXTRA_TYPE, 12, struct kbase_ioctl_lock_page_info)

#endif

#ifndef TABLET_MOCK_ABI_H
#define TABLET_MOCK_ABI_H
#define ONYX_SYSCALLS_H
#define SYS_mouse_read 86
long _onyx_ioctl(int, long, long);
long _onyx_syscall1(long, long);
#endif

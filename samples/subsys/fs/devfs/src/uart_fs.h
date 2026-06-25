#ifndef __UART_FS_H__
#define __UART_FS_H__

#define UART_FS_RB_SIZE                         (1024U)

enum uart_ioctl
{
    TCGETATTR,
    TCSETATTR,
};

#endif // __UART_FS_H__
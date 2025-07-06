// i_input_evdev.c - Doom fb input with full keyboard support via evdev

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>

#include "doomtype.h"
#include "doomkeys.h"
#include "d_event.h"

int vanilla_keyboard_mapping = 1;

static int kbd_fd = -1;
static int shiftdown = 0;

static void UpdateShiftStatus(int pressed, unsigned short code)
{
    int change = pressed ? 1 : -1;
    if (code == KEY_LEFTSHIFT || code == KEY_RIGHTSHIFT)
        shiftdown += change;
}

static unsigned char TranslateKey(unsigned short code)
{
    switch (code)
    {
        // 移动方向：WASD
        case KEY_E:         return KEY_UPARROW;
        case KEY_S:         return KEY_DOWNARROW;
        case KEY_A:         return KEY_LEFTARROW;
        case KEY_D:         return KEY_RIGHTARROW;

        // 动作键
        case KEY_K:         return KEY_FIRE;        // 开火
        case KEY_U:         return KEY_USE;         // 使用
        case KEY_L:         return KEY_ENTER;       // 确认
        case KEY_SPACE:     return ' ';             // 空格跳跃或确认
        case KEY_TAB:       return KEY_TAB;         // 地图
        case KEY_ESC:       return KEY_ESCAPE;      // 暂停 / 菜单
        case KEY_LEFTSHIFT: return KEY_RSHIFT;      // 奔跑

        // 武器切换（数字键）
        case KEY_1:         return '1';
        case KEY_2:         return '2';
        case KEY_3:         return '3';
        case KEY_4:         return '4';
        case KEY_5:         return '5';
        case KEY_6:         return '6';
        case KEY_7:         return '7';

        default:            return 0;               // 忽略未映射键
    }
}

static int kbd_read(int *pressed, unsigned short *key)
{
    struct input_event ev;
    ssize_t n = read(kbd_fd, &ev, sizeof(ev));
    if (n != sizeof(ev))
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            perror("kbd_read");
        return 0;
    }

    if (ev.type == EV_KEY && (ev.value == 0 || ev.value == 1))
    {
        *pressed = ev.value;  // 1=keydown, 0=keyup
        *key = ev.code;
        return 1;
    }

    return 0;
}

static int try_open_keyboard(void)
{
    char path[64], name[80];
    for (int i = 0; i < 16; ++i)
    {
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0)
        {
            if (strstr(name, "Keyboard") || strstr(name, "Espressif"))
            {
                printf("Found keyboard device: %s (%s)\n", path, name);
                return fd;
            }
        }
        close(fd);
    }
    return -1;
}

void I_InitInput(void)
{
    printf("Waiting for keyboard device...\n");
    while (1)
    {
        kbd_fd = try_open_keyboard();
        if (kbd_fd >= 0)
        {
            printf("MYGO!!! → ID=%d\n", kbd_fd);
            break;
        }
        sleep(1);
    }
}

            break;
        }
        sleep(1);
    }
}

void I_GetEvent(void)
{
    event_t event;
    int pressed;
    unsigned short key;

    while (kbd_read(&pressed, &key))
    {
        UpdateShiftStatus(pressed, key);

        unsigned char doomkey = TranslateKey(key);
        if (doomkey == 0)
            continue;

        event.type = pressed ? ev_keydown : ev_keyup;
        event.data1 = doomkey;
        event.data2 = 0;
        event.data3 = 0;

        D_PostEvent(&event);
    }
}

void I_ShutdownInput(void)
{
    if (kbd_fd >= 0)
    {
        close(kbd_fd);
        kbd_fd = -1;
    }
}

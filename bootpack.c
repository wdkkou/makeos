#include "bootpack.h"

struct FIFO8 keyfifo, mousefifo;
void enable_mouse(void);
void init_keyboard(void);

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)0xff0;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];

    init_gdtidt();
    init_pic();
    io_sti(); /* IDTとPICの初期化が終わったのでCPUの割り込み禁止を解除 */

    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);

    io_out8(PIC0_IMR, 0xf9); /* PIC1とキーボードを許可 */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可 */

    init_keyboard();

    init_palette();
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);

    putfont8_asc(binfo->vram, binfo->scrnx, 8, 8, WHITE, "WDK");
    putfont8_asc(binfo->vram, binfo->scrnx, 30, 30, WHITE, "oreore OS");

    init_mouse_cursor8(mcursor, COL8_008400);
    int mx = (binfo->scrnx - 16) / 2;
    int my = (binfo->scrny - 28 - 16) / 2;
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    sprintf(s, "(%d, %d)", mx, my);
    putfont8_asc(binfo->vram, binfo->scrnx, 16, 64, WHITE, s);

    enable_mouse();

    int i;
    for (;;)
    {
        io_cli();
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0)
        {
            io_stihlt();
        }
        else
        {
            if (fifo8_status(&keyfifo) != 0)
            {
                i = fifo8_get(&keyfifo);
                io_sti();
                sprintf(s, "%x", i);
                boxfill8(binfo->vram, binfo->scrnx, COL8_008400, 0, 16, 15, 31);
                putfont8_asc(binfo->vram, binfo->scrnx, 0, 16, WHITE, s);
            }
            else if (fifo8_status(&mousefifo) != 0)
            {
                i = fifo8_get(&mousefifo);
                io_sti();
                sprintf(s, "%x", i);
                boxfill8(binfo->vram, binfo->scrnx, COL8_008400, 32, 16, 47, 31);
                putfont8_asc(binfo->vram, binfo->scrnx, 32, 16, WHITE, s);
            }
        }
    }
}

#define PORT_KEYDAT 0x0060
#define PORT_KEYSTA 0x0064
#define PORT_KEYCMD 0x0064
#define KEYSTA_SEND_NOTREADY 0x02
#define KEYCMD_WRITE_MODE 0x60
#define KBC_MODE 0x47

/* キーボードコントローラが送信可能になるのを待つ */
void wait_KBC_sendready(void)
{
    for (;;)
    {
        if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0)
        {
            break;
        }
    }
    return;
}

void init_keyboard(void)
{
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, KBC_MODE);
    return;
}

#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4

void enable_mouse(void)
{
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    return;
}

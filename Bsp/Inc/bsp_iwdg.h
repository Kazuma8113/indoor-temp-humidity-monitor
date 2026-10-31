#ifndef BSP_IWDG_H
#define BSP_IWDG_H

/* 独立看门狗：喂一次狗，把计数器重装回 Reload 值。*/
void Bsp_Iwdg_Feed(void);

#endif /* BSP_IWDG_H */

#ifndef DEVICE_WARM_H
#define DEVICE_WARM_H

#include <stdint.h>     /* uint8_t */

/* ============================================================================
 * 器件驱动：加热片（控制脚低电平有效）
 * ========================================================================== */


void Warm_Init(void);
void Warm_Set(uint8_t on);

#endif /* DEVICE_WARM_H */


#ifndef BLINKLED_H
#define BLINKLED_H

#include "custom_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    void blinkled_init(u8 pin);
    void blinkled_enable(void);
    void blinkled_disable(void);
    void blinkled_toggle(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BLINKLED_H

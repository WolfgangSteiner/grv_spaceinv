#include "grvgm.h"

fx32 grvgm_sin(fx32 x) {
    double phi = fx32_to_f64(x) * 2.0 * M_PI;
    return fx32_from_f64(sin(phi));
}

fx32 grvgm_cos(fx32 x) {
    double phi = fx32_to_f64(x) * 2.0 * M_PI;
    return fx32_from_f64(cos(phi));
}

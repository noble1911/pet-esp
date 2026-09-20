#pragma once
#include <stdbool.h>
// QMI8658 gravity-based tilt: stable at rest, no integrated gyro drift.
bool motion_init(void);
bool motion_read(float *x,float *y);

#ifndef DENORMAL_KILL_H
#define DENORMAL_KILL_H

static inline void denormal_kill(float *v);

static inline void denormal_kill(float *v)
{
    static const float offset = 1e-18f;
    *v += offset;
    *v -= offset;
}

#endif

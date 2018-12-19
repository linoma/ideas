#include "ideastypes.h"
#include <math.h>

//---------------------------------------------------------------------------
#ifndef math3H
#define math3H
//---------------------------------------------------------------------------
float mtx3x3_det(float a1, float a2, float a3, float b1, float b2, float b3, float c1, float c2, float c3);
float mtx4x4_det(float *m);
void mtx4x4_inverse(float *m,float *m1);
void mtx4x3_mult(float *m,float *m1);
void mtx4x4_mult(float *m,float *m1);
void vet3_norm(float *v);
void mtx3x3_inverse(float *m2,float *m1);
void mtx3x3_mtx4x4(float *m4,float *m3);
void mtx4x4_transpose(float *m,float *m1);
void vet4_mtx4x4(float *v,float *m);
void vet3_mtx4x4(float *v,float *m);
void mtx3x3_mult(float *m,float *m1);
void scale_mtx4x4(float *m,float *m1,float v);
void q_mtx4x4(float *q,float *m);
void mtx4x4_q(float *m,float *q);
void q_norm(float *q);
void vet2_mtx4x4(float *v,float *m);
float vet3_dot(float *v,float *v1);
void vet3_sub(float *v,float *v1);
void vet3_cross(float *v,float *v1);
void vet4_norm(float *v);
void mtx3x4_mult(float *m,float *m1);

#endif



/* This file was automatically generated by CasADi 3.6.3.
 *  It consists of:
 *   1) content generated by CasADi runtime: not copyrighted
 *   2) template code copied from CasADi source: permissively licensed (MIT-0)
 *   3) user code: owned by the user
 *
 */
#ifdef __cplusplus
extern "C" {
#endif

/* How to prefix internal symbols */
#ifdef CASADI_CODEGEN_PREFIX
#define CASADI_NAMESPACE_CONCAT(NS, ID) _CASADI_NAMESPACE_CONCAT(NS, ID)
#define _CASADI_NAMESPACE_CONCAT(NS, ID) NS##ID
#define CASADI_PREFIX(ID) CASADI_NAMESPACE_CONCAT(CODEGEN_PREFIX, ID)
#else
#define CASADI_PREFIX(ID) strapdown_ins_##ID
#endif

#include <math.h>

#ifndef casadi_real
#define casadi_real double
#endif

#ifndef casadi_int
#define casadi_int long long int
#endif

/* Add prefix to internal symbols */
#define casadi_f0 CASADI_PREFIX(f0)
#define casadi_f1 CASADI_PREFIX(f1)
#define casadi_fabs CASADI_PREFIX(fabs)
#define casadi_s0 CASADI_PREFIX(s0)
#define casadi_s1 CASADI_PREFIX(s1)
#define casadi_s2 CASADI_PREFIX(s2)
#define casadi_s3 CASADI_PREFIX(s3)
#define casadi_sq CASADI_PREFIX(sq)

casadi_real casadi_sq(casadi_real x)
{
    return x * x;
}

casadi_real casadi_fabs(casadi_real x)
{
/* Pre-c99 compatibility */
#if __STDC_VERSION__ < 199901L
    return x > 0 ? x : -x;
#else
    return fabs(x);
#endif
}

static const casadi_int casadi_s0[13] = { 9, 1, 0, 9, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
static const casadi_int casadi_s1[7] = { 3, 1, 0, 3, 0, 1, 2 };
static const casadi_int casadi_s2[5] = { 1, 1, 0, 1, 0 };
static const casadi_int casadi_s3[8] = { 4, 1, 0, 4, 0, 1, 2, 3 };

/* strapdown_ins_propagate:(x0[9],a_b[3],omega_b[3],g,dt)->(x1[9]) */
static int casadi_f0(const casadi_real** arg, casadi_real** res, casadi_int* iw, casadi_real* w, int mem)
{
    w[0] = 1.;
    w[1] = 8.;
    w[2] = arg[0] ? arg[0][8] : 0;
    w[3] = casadi_sq(w[2]);
    w[4] = arg[0] ? arg[0][7] : 0;
    w[5] = casadi_sq(w[4]);
    w[3] = (w[3] + w[5]);
    w[3] = (w[1] * w[3]);
    w[5] = arg[0] ? arg[0][6] : 0;
    w[6] = casadi_sq(w[5]);
    w[7] = casadi_sq(w[4]);
    w[6] = (w[6] + w[7]);
    w[7] = casadi_sq(w[2]);
    w[6] = (w[6] + w[7]);
    w[7] = (w[0] + w[6]);
    w[7] = casadi_sq(w[7]);
    w[3] = (w[3] / w[7]);
    w[3] = (w[0] - w[3]);
    w[8] = arg[4] ? arg[4][0] : 0;
    w[9] = arg[1] ? arg[1][0] : 0;
    w[9] = (w[8] * w[9]);
    w[10] = (w[9] * w[8]);
    w[11] = 2.;
    w[10] = (w[10] / w[11]);
    w[12] = arg[2] ? arg[2][1] : 0;
    w[12] = (w[8] * w[12]);
    w[13] = arg[1] ? arg[1][2] : 0;
    w[13] = (w[8] * w[13]);
    w[14] = (w[12] * w[13]);
    w[15] = arg[2] ? arg[2][2] : 0;
    w[15] = (w[8] * w[15]);
    w[16] = arg[1] ? arg[1][1] : 0;
    w[16] = (w[8] * w[16]);
    w[17] = (w[15] * w[16]);
    w[14] = (w[14] - w[17]);
    w[17] = arg[2] ? arg[2][0] : 0;
    w[17] = (w[8] * w[17]);
    w[18] = casadi_sq(w[17]);
    w[19] = casadi_sq(w[12]);
    w[18] = (w[18] + w[19]);
    w[19] = casadi_sq(w[15]);
    w[18] = (w[18] + w[19]);
    w[19] = sqrt(w[18]);
    w[20] = casadi_fabs(w[19]);
    w[21] = 9.9999999999999995e-08;
    w[20] = (w[20] < w[21]);
    w[22] = 1.6666666666666666e-01;
    w[23] = -8.3333333333333332e-03;
    w[23] = (w[23] * w[18]);
    w[22] = (w[22] + w[23]);
    w[23] = 1.9841269841269841e-04;
    w[24] = casadi_sq(w[18]);
    w[23] = (w[23] * w[24]);
    w[22] = (w[22] + w[23]);
    w[22] = (w[20] ? w[22] : 0);
    w[20] = (!w[20]);
    w[23] = sin(w[19]);
    w[23] = (w[19] - w[23]);
    w[24] = (w[19] * w[18]);
    w[23] = (w[23] / w[24]);
    w[20] = (w[20] ? w[23] : 0);
    w[22] = (w[22] + w[20]);
    w[20] = (w[22] * w[8]);
    w[23] = (w[14] * w[20]);
    w[10] = (w[10] + w[23]);
    w[23] = (w[12] * w[17]);
    w[23] = (w[23] * w[16]);
    w[24] = casadi_sq(w[15]);
    w[25] = casadi_sq(w[12]);
    w[24] = (w[24] + w[25]);
    w[24] = (w[24] * w[9]);
    w[23] = (w[23] - w[24]);
    w[24] = (w[15] * w[17]);
    w[24] = (w[24] * w[13]);
    w[23] = (w[23] + w[24]);
    w[24] = casadi_fabs(w[19]);
    w[24] = (w[24] < w[21]);
    w[25] = 4.1666666666666664e-02;
    w[26] = -1.3888888888888889e-03;
    w[26] = (w[26] * w[18]);
    w[25] = (w[25] + w[26]);
    w[26] = 2.4801587301587302e-05;
    w[27] = casadi_sq(w[18]);
    w[26] = (w[26] * w[27]);
    w[25] = (w[25] + w[26]);
    w[25] = (w[24] ? w[25] : 0);
    w[24] = (!w[24]);
    w[26] = -1.;
    w[27] = 5.0000000000000000e-01;
    w[28] = (w[27] * w[18]);
    w[26] = (w[26] + w[28]);
    w[28] = cos(w[19]);
    w[26] = (w[26] + w[28]);
    w[28] = casadi_sq(w[18]);
    w[26] = (w[26] / w[28]);
    w[24] = (w[24] ? w[26] : 0);
    w[25] = (w[25] + w[24]);
    w[25] = (w[25] * w[8]);
    w[24] = (w[23] * w[25]);
    w[10] = (w[10] + w[24]);
    w[24] = (w[3] * w[10]);
    w[26] = (w[5] * w[4]);
    w[26] = (w[1] * w[26]);
    w[28] = 4.;
    w[6] = (w[0] - w[6]);
    w[6] = (w[28] * w[6]);
    w[29] = (w[6] * w[2]);
    w[26] = (w[26] - w[29]);
    w[26] = (w[26] / w[7]);
    w[29] = (w[16] * w[8]);
    w[29] = (w[29] / w[11]);
    w[30] = (w[15] * w[9]);
    w[31] = (w[17] * w[13]);
    w[30] = (w[30] - w[31]);
    w[31] = (w[30] * w[20]);
    w[29] = (w[29] + w[31]);
    w[31] = (w[17] * w[12]);
    w[31] = (w[31] * w[9]);
    w[32] = casadi_sq(w[15]);
    w[33] = casadi_sq(w[17]);
    w[32] = (w[32] + w[33]);
    w[32] = (w[32] * w[16]);
    w[31] = (w[31] - w[32]);
    w[32] = (w[15] * w[12]);
    w[32] = (w[32] * w[13]);
    w[31] = (w[31] + w[32]);
    w[32] = (w[31] * w[25]);
    w[29] = (w[29] + w[32]);
    w[32] = (w[26] * w[29]);
    w[24] = (w[24] + w[32]);
    w[32] = (w[5] * w[2]);
    w[32] = (w[1] * w[32]);
    w[33] = (w[6] * w[4]);
    w[32] = (w[32] + w[33]);
    w[32] = (w[32] / w[7]);
    w[33] = (w[13] * w[8]);
    w[33] = (w[33] / w[11]);
    w[34] = (w[17] * w[16]);
    w[35] = (w[12] * w[9]);
    w[34] = (w[34] - w[35]);
    w[20] = (w[34] * w[20]);
    w[33] = (w[33] + w[20]);
    w[20] = (w[17] * w[15]);
    w[20] = (w[20] * w[9]);
    w[35] = (w[12] * w[15]);
    w[35] = (w[35] * w[16]);
    w[20] = (w[20] + w[35]);
    w[35] = casadi_sq(w[12]);
    w[36] = casadi_sq(w[17]);
    w[35] = (w[35] + w[36]);
    w[35] = (w[35] * w[13]);
    w[20] = (w[20] - w[35]);
    w[25] = (w[20] * w[25]);
    w[33] = (w[33] + w[25]);
    w[25] = (w[32] * w[33]);
    w[24] = (w[24] + w[25]);
    w[25] = arg[0] ? arg[0][3] : 0;
    w[35] = (w[25] * w[8]);
    w[36] = arg[0] ? arg[0][0] : 0;
    w[35] = (w[35] + w[36]);
    w[24] = (w[24] + w[35]);
    if (res[0] != 0)
        res[0][0] = w[24];
    w[24] = (w[4] * w[5]);
    w[24] = (w[1] * w[24]);
    w[35] = (w[6] * w[2]);
    w[24] = (w[24] + w[35]);
    w[24] = (w[24] / w[7]);
    w[35] = (w[24] * w[10]);
    w[36] = casadi_sq(w[2]);
    w[37] = casadi_sq(w[5]);
    w[36] = (w[36] + w[37]);
    w[36] = (w[1] * w[36]);
    w[36] = (w[36] / w[7]);
    w[36] = (w[0] - w[36]);
    w[37] = (w[36] * w[29]);
    w[35] = (w[35] + w[37]);
    w[37] = (w[4] * w[2]);
    w[37] = (w[1] * w[37]);
    w[38] = (w[6] * w[5]);
    w[37] = (w[37] - w[38]);
    w[37] = (w[37] / w[7]);
    w[38] = (w[37] * w[33]);
    w[35] = (w[35] + w[38]);
    w[38] = arg[0] ? arg[0][4] : 0;
    w[39] = (w[38] * w[8]);
    w[40] = arg[0] ? arg[0][1] : 0;
    w[39] = (w[39] + w[40]);
    w[35] = (w[35] + w[39]);
    if (res[0] != 0)
        res[0][1] = w[35];
    w[35] = (w[2] * w[5]);
    w[35] = (w[1] * w[35]);
    w[39] = (w[6] * w[4]);
    w[35] = (w[35] - w[39]);
    w[35] = (w[35] / w[7]);
    w[10] = (w[35] * w[10]);
    w[39] = (w[2] * w[4]);
    w[39] = (w[1] * w[39]);
    w[6] = (w[6] * w[5]);
    w[39] = (w[39] + w[6]);
    w[39] = (w[39] / w[7]);
    w[29] = (w[39] * w[29]);
    w[10] = (w[10] + w[29]);
    w[29] = casadi_sq(w[4]);
    w[6] = casadi_sq(w[5]);
    w[29] = (w[29] + w[6]);
    w[1] = (w[1] * w[29]);
    w[1] = (w[1] / w[7]);
    w[1] = (w[0] - w[1]);
    w[33] = (w[1] * w[33]);
    w[10] = (w[10] + w[33]);
    w[33] = arg[0] ? arg[0][5] : 0;
    w[7] = arg[3] ? arg[3][0] : 0;
    w[7] = (w[8] * w[7]);
    w[33] = (w[33] + w[7]);
    w[29] = (w[33] * w[8]);
    w[6] = arg[0] ? arg[0][2] : 0;
    w[7] = (w[7] * w[8]);
    w[7] = (w[7] / w[11]);
    w[6] = (w[6] - w[7]);
    w[29] = (w[29] + w[6]);
    w[10] = (w[10] + w[29]);
    if (res[0] != 0)
        res[0][2] = w[10];
    w[10] = casadi_fabs(w[19]);
    w[10] = (w[10] < w[21]);
    w[29] = -4.1666666666666664e-02;
    w[29] = (w[29] * w[18]);
    w[27] = (w[27] + w[29]);
    w[29] = 1.3888888888888889e-03;
    w[6] = casadi_sq(w[18]);
    w[29] = (w[29] * w[6]);
    w[27] = (w[27] + w[29]);
    w[27] = (w[10] ? w[27] : 0);
    w[10] = (!w[10]);
    w[19] = cos(w[19]);
    w[19] = (w[0] - w[19]);
    w[19] = (w[19] / w[18]);
    w[10] = (w[10] ? w[19] : 0);
    w[27] = (w[27] + w[10]);
    w[14] = (w[14] * w[27]);
    w[9] = (w[9] + w[14]);
    w[23] = (w[23] * w[22]);
    w[9] = (w[9] + w[23]);
    w[3] = (w[3] * w[9]);
    w[30] = (w[30] * w[27]);
    w[16] = (w[16] + w[30]);
    w[31] = (w[31] * w[22]);
    w[16] = (w[16] + w[31]);
    w[26] = (w[26] * w[16]);
    w[3] = (w[3] + w[26]);
    w[34] = (w[34] * w[27]);
    w[13] = (w[13] + w[34]);
    w[20] = (w[20] * w[22]);
    w[13] = (w[13] + w[20]);
    w[32] = (w[32] * w[13]);
    w[3] = (w[3] + w[32]);
    w[3] = (w[3] + w[25]);
    if (res[0] != 0)
        res[0][3] = w[3];
    w[24] = (w[24] * w[9]);
    w[36] = (w[36] * w[16]);
    w[24] = (w[24] + w[36]);
    w[37] = (w[37] * w[13]);
    w[24] = (w[24] + w[37]);
    w[24] = (w[24] + w[38]);
    if (res[0] != 0)
        res[0][4] = w[24];
    w[35] = (w[35] * w[9]);
    w[39] = (w[39] * w[16]);
    w[35] = (w[35] + w[39]);
    w[1] = (w[1] * w[13]);
    w[35] = (w[35] + w[1]);
    w[35] = (w[35] + w[33]);
    if (res[0] != 0)
        res[0][5] = w[35];
    w[35] = casadi_sq(w[5]);
    w[33] = casadi_sq(w[4]);
    w[35] = (w[35] + w[33]);
    w[33] = casadi_sq(w[2]);
    w[35] = (w[35] + w[33]);
    w[33] = (w[0] - w[35]);
    w[1] = casadi_sq(w[17]);
    w[13] = casadi_sq(w[12]);
    w[1] = (w[1] + w[13]);
    w[13] = casadi_sq(w[15]);
    w[1] = (w[1] + w[13]);
    w[1] = sqrt(w[1]);
    w[21] = (w[21] < w[1]);
    w[28] = (w[1] / w[28]);
    w[28] = tan(w[28]);
    w[17] = (w[28] * w[17]);
    w[17] = (w[17] / w[1]);
    w[13] = casadi_sq(w[17]);
    w[12] = (w[28] * w[12]);
    w[12] = (w[12] / w[1]);
    w[39] = casadi_sq(w[12]);
    w[13] = (w[13] + w[39]);
    w[28] = (w[28] * w[15]);
    w[28] = (w[28] / w[1]);
    w[1] = casadi_sq(w[28]);
    w[13] = (w[13] + w[1]);
    w[13] = (w[21] ? w[13] : 0);
    w[13] = (w[21] ? w[13] : 0);
    w[13] = sqrt(w[13]);
    w[13] = (w[0] < w[13]);
    w[1] = (w[21] ? w[17] : 0);
    w[17] = casadi_sq(w[17]);
    w[15] = casadi_sq(w[12]);
    w[17] = (w[17] + w[15]);
    w[15] = casadi_sq(w[28]);
    w[17] = (w[17] + w[15]);
    w[17] = (w[21] ? w[17] : 0);
    w[17] = (w[21] ? w[17] : 0);
    w[15] = (w[1] / w[17]);
    w[15] = (-w[15]);
    w[15] = (w[13] ? w[15] : 0);
    w[39] = (!w[13]);
    w[1] = (w[39] ? w[1] : 0);
    w[15] = (w[15] + w[1]);
    w[1] = (w[33] * w[15]);
    w[16] = casadi_sq(w[15]);
    w[12] = (w[21] ? w[12] : 0);
    w[9] = (w[12] / w[17]);
    w[9] = (-w[9]);
    w[9] = (w[13] ? w[9] : 0);
    w[12] = (w[39] ? w[12] : 0);
    w[9] = (w[9] + w[12]);
    w[12] = casadi_sq(w[9]);
    w[16] = (w[16] + w[12]);
    w[21] = (w[21] ? w[28] : 0);
    w[17] = (w[21] / w[17]);
    w[17] = (-w[17]);
    w[13] = (w[13] ? w[17] : 0);
    w[39] = (w[39] ? w[21] : 0);
    w[13] = (w[13] + w[39]);
    w[39] = casadi_sq(w[13]);
    w[16] = (w[16] + w[39]);
    w[39] = (w[0] - w[16]);
    w[21] = (w[39] * w[5]);
    w[1] = (w[1] + w[21]);
    w[21] = (w[9] * w[2]);
    w[17] = (w[13] * w[4]);
    w[21] = (w[21] - w[17]);
    w[21] = (w[11] * w[21]);
    w[1] = (w[1] - w[21]);
    w[35] = (w[35] * w[16]);
    w[35] = (w[0] + w[35]);
    w[16] = (w[15] * w[5]);
    w[21] = (w[9] * w[4]);
    w[16] = (w[16] + w[21]);
    w[21] = (w[13] * w[2]);
    w[16] = (w[16] + w[21]);
    w[16] = (w[11] * w[16]);
    w[35] = (w[35] - w[16]);
    w[1] = (w[1] / w[35]);
    w[16] = casadi_sq(w[1]);
    w[21] = (w[33] * w[9]);
    w[17] = (w[39] * w[4]);
    w[21] = (w[21] + w[17]);
    w[17] = (w[13] * w[5]);
    w[28] = (w[15] * w[2]);
    w[17] = (w[17] - w[28]);
    w[17] = (w[11] * w[17]);
    w[21] = (w[21] - w[17]);
    w[21] = (w[21] / w[35]);
    w[17] = casadi_sq(w[21]);
    w[16] = (w[16] + w[17]);
    w[33] = (w[33] * w[13]);
    w[39] = (w[39] * w[2]);
    w[33] = (w[33] + w[39]);
    w[15] = (w[15] * w[4]);
    w[9] = (w[9] * w[5]);
    w[15] = (w[15] - w[9]);
    w[11] = (w[11] * w[15]);
    w[33] = (w[33] - w[11]);
    w[33] = (w[33] / w[35]);
    w[35] = casadi_sq(w[33]);
    w[16] = (w[16] + w[35]);
    w[16] = sqrt(w[16]);
    w[0] = (w[0] < w[16]);
    w[16] = casadi_sq(w[1]);
    w[35] = casadi_sq(w[21]);
    w[16] = (w[16] + w[35]);
    w[35] = casadi_sq(w[33]);
    w[16] = (w[16] + w[35]);
    w[35] = (w[1] / w[16]);
    w[35] = (-w[35]);
    w[35] = (w[0] ? w[35] : 0);
    w[11] = (!w[0]);
    w[1] = (w[11] ? w[1] : 0);
    w[35] = (w[35] + w[1]);
    if (res[0] != 0)
        res[0][6] = w[35];
    w[35] = (w[21] / w[16]);
    w[35] = (-w[35]);
    w[35] = (w[0] ? w[35] : 0);
    w[21] = (w[11] ? w[21] : 0);
    w[35] = (w[35] + w[21]);
    if (res[0] != 0)
        res[0][7] = w[35];
    w[16] = (w[33] / w[16]);
    w[16] = (-w[16]);
    w[0] = (w[0] ? w[16] : 0);
    w[11] = (w[11] ? w[33] : 0);
    w[0] = (w[0] + w[11]);
    if (res[0] != 0)
        res[0][8] = w[0];
    return 0;
}

int strapdown_ins_propagate(const casadi_real** arg, casadi_real** res, casadi_int* iw, casadi_real* w, int mem)
{
    return casadi_f0(arg, res, iw, w, mem);
}

int strapdown_ins_propagate_alloc_mem(void)
{
    return 0;
}

int strapdown_ins_propagate_init_mem(int mem)
{
    return 0;
}

void strapdown_ins_propagate_free_mem(int mem)
{
}

int strapdown_ins_propagate_checkout(void)
{
    return 0;
}

void strapdown_ins_propagate_release(int mem)
{
}

void strapdown_ins_propagate_incref(void)
{
}

void strapdown_ins_propagate_decref(void)
{
}

casadi_int strapdown_ins_propagate_n_in(void) { return 5; }

casadi_int strapdown_ins_propagate_n_out(void) { return 1; }

casadi_real strapdown_ins_propagate_default_in(casadi_int i)
{
    switch (i) {
    default:
        return 0;
    }
}

const char* strapdown_ins_propagate_name_in(casadi_int i)
{
    switch (i) {
    case 0:
        return "x0";
    case 1:
        return "a_b";
    case 2:
        return "omega_b";
    case 3:
        return "g";
    case 4:
        return "dt";
    default:
        return 0;
    }
}

const char* strapdown_ins_propagate_name_out(casadi_int i)
{
    switch (i) {
    case 0:
        return "x1";
    default:
        return 0;
    }
}

const casadi_int* strapdown_ins_propagate_sparsity_in(casadi_int i)
{
    switch (i) {
    case 0:
        return casadi_s0;
    case 1:
        return casadi_s1;
    case 2:
        return casadi_s1;
    case 3:
        return casadi_s2;
    case 4:
        return casadi_s2;
    default:
        return 0;
    }
}

const casadi_int* strapdown_ins_propagate_sparsity_out(casadi_int i)
{
    switch (i) {
    case 0:
        return casadi_s0;
    default:
        return 0;
    }
}

int strapdown_ins_propagate_work(casadi_int* sz_arg, casadi_int* sz_res, casadi_int* sz_iw, casadi_int* sz_w)
{
    if (sz_arg)
        *sz_arg = 5;
    if (sz_res)
        *sz_res = 1;
    if (sz_iw)
        *sz_iw = 0;
    if (sz_w)
        *sz_w = 41;
    return 0;
}

/* quat_from_mrp:(r[3])->(q[4]) */
static int casadi_f1(const casadi_real** arg, casadi_real** res, casadi_int* iw, casadi_real* w, int mem)
{
    w[0] = 1.;
    w[1] = arg[0] ? arg[0][0] : 0;
    w[2] = casadi_sq(w[1]);
    w[3] = arg[0] ? arg[0][1] : 0;
    w[4] = casadi_sq(w[3]);
    w[2] = (w[2] + w[4]);
    w[4] = arg[0] ? arg[0][2] : 0;
    w[5] = casadi_sq(w[4]);
    w[2] = (w[2] + w[5]);
    w[5] = (w[0] - w[2]);
    w[0] = (w[0] + w[2]);
    w[5] = (w[5] / w[0]);
    if (res[0] != 0)
        res[0][0] = w[5];
    w[5] = 2.;
    w[1] = (w[5] * w[1]);
    w[1] = (w[1] / w[0]);
    if (res[0] != 0)
        res[0][1] = w[1];
    w[3] = (w[5] * w[3]);
    w[3] = (w[3] / w[0]);
    if (res[0] != 0)
        res[0][2] = w[3];
    w[5] = (w[5] * w[4]);
    w[5] = (w[5] / w[0]);
    if (res[0] != 0)
        res[0][3] = w[5];
    return 0;
}

int quat_from_mrp(const casadi_real** arg, casadi_real** res, casadi_int* iw, casadi_real* w, int mem)
{
    return casadi_f1(arg, res, iw, w, mem);
}

int quat_from_mrp_alloc_mem(void)
{
    return 0;
}

int quat_from_mrp_init_mem(int mem)
{
    return 0;
}

void quat_from_mrp_free_mem(int mem)
{
}

int quat_from_mrp_checkout(void)
{
    return 0;
}

void quat_from_mrp_release(int mem)
{
}

void quat_from_mrp_incref(void)
{
}

void quat_from_mrp_decref(void)
{
}

casadi_int quat_from_mrp_n_in(void) { return 1; }

casadi_int quat_from_mrp_n_out(void) { return 1; }

casadi_real quat_from_mrp_default_in(casadi_int i)
{
    switch (i) {
    default:
        return 0;
    }
}

const char* quat_from_mrp_name_in(casadi_int i)
{
    switch (i) {
    case 0:
        return "r";
    default:
        return 0;
    }
}

const char* quat_from_mrp_name_out(casadi_int i)
{
    switch (i) {
    case 0:
        return "q";
    default:
        return 0;
    }
}

const casadi_int* quat_from_mrp_sparsity_in(casadi_int i)
{
    switch (i) {
    case 0:
        return casadi_s1;
    default:
        return 0;
    }
}

const casadi_int* quat_from_mrp_sparsity_out(casadi_int i)
{
    switch (i) {
    case 0:
        return casadi_s3;
    default:
        return 0;
    }
}

int quat_from_mrp_work(casadi_int* sz_arg, casadi_int* sz_res, casadi_int* sz_iw, casadi_int* sz_w)
{
    if (sz_arg)
        *sz_arg = 1;
    if (sz_res)
        *sz_res = 1;
    if (sz_iw)
        *sz_iw = 0;
    if (sz_w)
        *sz_w = 6;
    return 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

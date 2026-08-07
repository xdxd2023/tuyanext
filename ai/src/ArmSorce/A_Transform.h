#pragma once

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include "RbConfig.h"

void Dxmat(float dx,float matrix[4][4]);
void Dymat(float dy,float matrix[4][4]);
void Dzmat(float dz,float matrix[4][4]);
void Rxmat(float gamma,float matrix[4][4]);
void Rymat(float beta,float matrix[4][4]);
void Rzmat(float alpha,float matrix[4][4]);
void DHmat(float alpha, float a, float theta, float d,float matrix[4][4]);
void inverse(float source[4][4],float result[4][4]);
void Matrix_4X4_mul(float a[4][4],float b[4][4],float c[4][4]);
void Matrix_3X1_mul(float a[3][3],float b[3][1],float c[3][1]);
void euler2rmat(float roll, float pitch, float yaw,float result[3][3]);
void rmat2euler(float roll,float pitch,float yaw,float rmat[3][3]);
float A_round(float data);
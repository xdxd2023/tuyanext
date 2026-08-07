#include "A_Transform.h"
#include <math.h>
#include <string.h>

void Dxmat(float dx,float matrix[4][4])
{
	matrix[0][0] = 1;
	matrix[0][1] = 0;
	matrix[0][2] = 0;
	matrix[0][3] = dx;

	matrix[1][0] = 0;
	matrix[1][1] = 1;
	matrix[1][2] = 0;
	matrix[1][3] = 0;	
	
	matrix[2][0] = 0;
	matrix[2][1] = 0;
	matrix[2][2] = 1;
	matrix[2][3] = 0;	

	matrix[3][0] = 0;
	matrix[3][1] = 0;
	matrix[3][2] = 0;
	matrix[3][3] = 1;		
}

void Dymat(float dy,float matrix[4][4])
{
	matrix[0][0] = 1;
	matrix[0][1] = 0;
	matrix[0][2] = 0;
	matrix[0][3] = 0;

	matrix[1][0] = 0;
	matrix[1][1] = 1;
	matrix[1][2] = 0;
	matrix[1][3] = dy;	
	
	matrix[2][0] = 0;
	matrix[2][1] = 0;
	matrix[2][2] = 1;
	matrix[2][3] = 0;	

	matrix[3][0] = 0;
	matrix[3][1] = 0;
	matrix[3][2] = 0;
	matrix[3][3] = 1;		
}

void Dzmat(float dz,float matrix[4][4])
{
	matrix[0][0] = 1;
	matrix[0][1] = 0;
	matrix[0][2] = 0;
	matrix[0][3] = 0;

	matrix[1][0] = 0;
	matrix[1][1] = 1;
	matrix[1][2] = 0;
	matrix[1][3] = 0;	
	
	matrix[2][0] = 0;
	matrix[2][1] = 0;
	matrix[2][2] = 1;
	matrix[2][3] = dz;	

	matrix[3][0] = 0;
	matrix[3][1] = 0;
	matrix[3][2] = 0;
	matrix[3][3] = 1;			
}

/*
	[1, 0,           0,          0],
	[0, cos(gamma), -sin(gamma), 0],
	[0, sin(gamma),  cos(gamma), 0],
	[0, 0,           0,          1]
*/
void Rxmat(float gamma,float matrix[4][4]) //'''??X?????'''
{
	matrix[0][0] = 1;
	matrix[0][1] = 0;
	matrix[0][2] = 0;
	matrix[0][3] = 0;

	matrix[1][0] = 0;
	matrix[1][1] = cos(gamma);
	matrix[1][2] = -sin(gamma);
	matrix[1][3] = 0;	
	
	matrix[2][0] = 0;
	matrix[2][1] = sin(gamma);
	matrix[2][2] = cos(gamma);
	matrix[2][3] = 0;	

	matrix[3][0] = 0;
	matrix[3][1] = 0;
	matrix[3][2] = 0;
	matrix[3][3] = 1;	
}
/*
	[cos(beta),  0,  sin(beta), 0],
	[0,          1,  0,         0],
	[-sin(beta), 0,  cos(beta), 0],
	[0,          0,  0,         1]
*/
void Rymat(float beta,float matrix[4][4]) //'''??Y?????'''
{
	matrix[0][0] = cos(beta);
	matrix[0][1] = 0;
	matrix[0][2] = sin(beta);
	matrix[0][3] = 0;

	matrix[1][0] = 0;
	matrix[1][1] = 1;
	matrix[1][2] = 0;
	matrix[1][3] = 0;	
	
	matrix[2][0] = -sin(beta);
	matrix[2][1] = 0;
	matrix[2][2] = cos(beta);
	matrix[2][3] = 0;	

	matrix[3][0] = 0;
	matrix[3][1] = 0;
	matrix[3][2] = 0;
	matrix[3][3] = 1;	
}
/*
	[cos(alpha), -sin(alpha), 0, 0],
	[sin(alpha),  cos(alpha), 0, 0],
	[0,           0,          1, 0],
	[0,           0,          0, 1]
*/
void Rzmat(float alpha,float matrix[4][4]) //'''??Z?????'''
{
	matrix[0][0] = cos(alpha);
	matrix[0][1] = -sin(alpha);
	matrix[0][2] = 0;
	matrix[0][3] = 0;

	matrix[1][0] = sin(alpha);
	matrix[1][1] = cos(alpha);
	matrix[1][2] = 0;
	matrix[1][3] = 0;	
	
	matrix[2][0] = 0;
	matrix[2][1] = 0;
	matrix[2][2] = 1;
	matrix[2][3] = 0;	

	matrix[3][0] = 0;
	matrix[3][1] = 0;
	matrix[3][2] = 0;
	matrix[3][3] = 1;	
}

void Matrix_4X4_mul(float a[4][4],float b[4][4],float c[4][4])
{
	c[0][0] = a[0][0]*b[0][0] + a[0][1]*b[1][0] + a[0][2]*b[2][0] + a[0][3]*b[3][0];
	c[0][1] = a[0][0]*b[0][1] + a[0][1]*b[1][1] + a[0][2]*b[2][1] + a[0][3]*b[3][1];
	c[0][2] = a[0][0]*b[0][2] + a[0][1]*b[1][2] + a[0][2]*b[2][2] + a[0][3]*b[3][2];
	c[0][3] = a[0][0]*b[0][3] + a[0][1]*b[1][3] + a[0][2]*b[2][3] + a[0][3]*b[3][3];
	
	c[1][0] = a[1][0]*b[0][0] + a[1][1]*b[1][0] + a[1][2]*b[2][0] + a[1][3]*b[3][0];
	c[1][1] = a[1][0]*b[0][1] + a[1][1]*b[1][1] + a[1][2]*b[2][1] + a[1][3]*b[3][1];
	c[1][2] = a[1][0]*b[0][2] + a[1][1]*b[1][2] + a[1][2]*b[2][2] + a[1][3]*b[3][2];
	c[1][3] = a[1][0]*b[0][3] + a[1][1]*b[1][3] + a[1][2]*b[2][3] + a[1][3]*b[3][3];

	c[2][0] = a[2][0]*b[0][0] + a[2][1]*b[1][0] + a[2][2]*b[2][0] + a[2][3]*b[3][0];
	c[2][1] = a[2][0]*b[0][1] + a[2][1]*b[1][1] + a[2][2]*b[2][1] + a[2][3]*b[3][1];
	c[2][2] = a[2][0]*b[0][2] + a[2][1]*b[1][2] + a[2][2]*b[2][2] + a[2][3]*b[3][2];
	c[2][3] = a[2][0]*b[0][3] + a[2][1]*b[1][3] + a[2][2]*b[2][3] + a[2][3]*b[3][3];	
	
	c[3][0] = a[3][0]*b[0][0] + a[3][1]*b[1][0] + a[3][2]*b[2][0] + a[3][3]*b[3][0];
	c[3][1] = a[3][0]*b[0][1] + a[3][1]*b[1][1] + a[3][2]*b[2][1] + a[3][3]*b[3][1];
	c[3][2] = a[3][0]*b[0][2] + a[3][1]*b[1][2] + a[3][2]*b[2][2] + a[3][3]*b[3][2];
	c[3][3] = a[3][0]*b[0][3] + a[3][1]*b[1][3] + a[3][2]*b[2][3] + a[3][3]*b[3][3];	
}

void Matrix_3X1_mul(float a[3][3],float b[3][1],float c[3][1])
{
	c[0][0] = a[0][0]*b[0][0] + a[0][1]*b[1][0] + a[0][2]*b[2][0];
	c[1][0] = a[1][0]*b[0][0] + a[1][1]*b[1][0] + a[1][2]*b[2][0];
	c[2][0] = a[2][0]*b[0][0] + a[2][1]*b[1][0] + a[2][2]*b[2][0];

}

//	def dhmat(alpha, a, theta, d):
//		'''DH?任????'''
//		dhmat = Transform.rxmat(alpha)
//		dhmat = dhmat.dot(Transform.dxmat(a))
//		dhmat = dhmat.dot(Transform.rzmat(theta))
//		dhmat = dhmat.dot(Transform.dzmat(d))
//		return dhmat
			
float temp_1[4][4] = {0};	
float temp_2[4][4] = {0};
float temp_3[4][4] = {0};
void DHmat(float alpha, float a, float theta, float d,float matrix[4][4])
{
	Rxmat(alpha,temp_1);
	Dxmat(a,temp_2);
	Matrix_4X4_mul(temp_1,temp_2,temp_3);
	memset(temp_1,0, sizeof(temp_1[0][0])*16);
	memset(temp_2,0, sizeof(temp_2[0][0])*16);
	memcpy(temp_1,temp_3, sizeof(temp_1[0][0])*16);
	Rzmat(theta,temp_2);
	Matrix_4X4_mul(temp_3,temp_1,temp_2);
	memset(temp_1,0, sizeof(temp_1[0][0])*16);
	memset(temp_2,0, sizeof(temp_2[0][0])*16);
	memcpy(temp_1,temp_3, sizeof(temp_1[0][0])*16);
	Dzmat(d,temp_2);
	Matrix_4X4_mul(matrix,temp_1,temp_2);
}
/*
	@staticmethod
	def euler2rmat(roll=0, pitch=0, yaw=0):
		'''????????????????''' 
		alpha, beta, gamma = yaw, pitch, roll
		cos_gamma = np.cos(gamma)
		sin_gamma = np.sin(gamma)
		cos_beta = np.cos(beta)
		sin_beta = np.sin(beta)
		cos_alpha = np.cos(alpha)
		sin_alpha = np.sin(alpha)

		r11 = cos_alpha*cos_beta
		r12 = -sin_alpha*cos_gamma + sin_beta*sin_gamma*cos_alpha
		r13 = sin_alpha*sin_gamma + sin_beta*cos_alpha*cos_gamma
		r21 = sin_alpha*cos_beta
		r22 = sin_alpha*sin_beta*sin_gamma + cos_alpha*cos_gamma
		r23 = sin_alpha*sin_beta*cos_gamma - sin_gamma*cos_alpha
		r31 = -sin_beta
		r32 = sin_gamma*cos_beta
		r33 = cos_beta*cos_gamma
		return np.array([
			[r11, r12, r13],
			[r21, r22, r23],
			[r31, r32, r33]])
*/

void euler2rmat(float roll, float pitch, float yaw,float result[3][3])  //'''????????????????''' 
{
	float alpha = yaw;
	float beta = pitch;
	float gamma = roll;
	
	float cos_gamma = cos(gamma);
	float sin_gamma = sin(gamma);
	float cos_beta = cos(beta);
	float sin_beta = sin(beta);
	float cos_alpha = cos(alpha);
	float sin_alpha = sin(alpha);	
	
	result[0][0] = cos_alpha*cos_beta;
	result[0][1] = -sin_alpha*cos_gamma + sin_beta*sin_gamma*cos_alpha;
	result[0][2] = sin_alpha*sin_gamma + sin_beta*cos_alpha*cos_gamma;
	result[1][0] = sin_alpha*cos_beta;
	result[1][1] = sin_alpha*sin_beta*sin_gamma + cos_alpha*cos_gamma;
	result[1][2] = sin_alpha*sin_beta*cos_gamma - sin_gamma*cos_alpha;
	result[2][0] = -sin_beta;
	result[2][1] = sin_gamma*cos_beta;
	result[2][2] = cos_beta*cos_gamma;
}

/*
	def rmat2euler(rmat):
		'''????????????????'''
		alpha = None # ????? yaw
		beta = None  # ?????? pitch
		gamma = None # ????? roll
		
		r11, r12, r13, r21, r22, r23, r31, r32, r33 = rmat.reshape(-1)
		if abs(r31) >= (1 - 0.000001):
			# ????????????????
			if r31 < 0:
				gamma = 0
				beta = np.pi/2
				alpha = math.atan2(r23, r22)
				return [[gamma, beta, alpha]]
			else:
				gamma = 0
				beta = -np.pi/2
				alpha = math.atan2(-r23, r22)
				return [[gamma, beta, alpha]]
		else:
			# ???????
			cos_beta = np.sqrt(r32*r32 +r33*r33)
			cos_beta_list = [cos_beta, -cos_beta]
			rpy_list = []
			for cos_beta in cos_beta_list:
				beta = math.atan2(-r31, cos_beta)
				alpha = math.atan2(r21/cos_beta, r11/cos_beta)
				gamma = math.atan2(r32/cos_beta, r33/cos_beta)
				rpy_list.append([gamma, beta, alpha])
			return rpy_list
*/

void rmat2euler(float roll,float pitch,float yaw,float rmat[3][3]) //'''????????????????'''
{
//		alpha = None # ????? yaw
//		beta = None  # ?????? pitch
//		gamma = None # ????? roll	
	float cos_beta = 0; 
	
	if (fabs(rmat[2][0]) >= (1 - 0.000001)) //# ????????????????
	{	
		if(rmat[2][0] < 0)
		{
			roll = 0;
			pitch = pi/2;
			yaw = atan2(rmat[1][2], rmat[1][1]);
		}
		else
		{
			roll = 0;
			pitch = -(pi/2);
			yaw = atan2(-rmat[1][2], rmat[1][1]);				
		}
	}
	else //# ???????
	{		
		cos_beta = sqrt(rmat[2][1]*rmat[2][1] + rmat[2][2]*rmat[2][2]);
		pitch = atan2(-rmat[2][0], cos_beta);
		yaw = atan2(rmat[1][0]/cos_beta, rmat[0][0]/cos_beta);
		roll = atan2(rmat[2][1]/cos_beta, rmat[2][2]/cos_beta);
	}
}



/*
	def inverse(T):
		'''??α任????????'''
		R = T[:3, :3]                  ??4x4????α任???????????9???????3X3?????
		t = T[:3, 3].reshape((3, 1))   ??4x4????α任???????????9???????3X3????????????3?е??3X1?????
		R_T = R.T                      ??R????????????б???У???????3X3????
		T_inv = np.eye(4)              ?4X4??λ
		T_inv[:3, :3] = R_T            ??4X4??λ?????????9???????3X3??????滻?R_T
		T_inv[:3, 3] = -R_T.dot(t).reshape(-1)  R_T???t???3X1????
		return T_inv

*/
float R[3][3] = {0};
float R_T[3][3] = {0};
float t[3][1] = {0};
float t_temp[3][1] = {0};
float T_inv_temp[3][3] ={0};
float T_inv[4][4] ={0};

void inverse(float source[4][4],float result[4][4])
{
//	float R[3][3] = {0};
//	float R_T[3][3] = {0};
//	float t[3][1] = {0};
//	float t_temp[3][1] = {0};
//	float T_inv_temp[3][3] ={0};
//	float T_inv[4][4] ={0};
	
	for(int i=0;i<3;i++)  //?3x3
	{
		for(int j=0;j<3;j++)
		{
			R[i][j] = source[i][j];
		}
	}
	
	for(int k=0;k<3;k++)         //???3??
	{
		t[k][0] = source[k][3];
	}
	
	for(int m=0;m<3;m++)       //???
	{
		for(int n=0;n<3;n++)
		{
			R_T[n][m] = R[m][n];
		}
	}
	
//	T_inv_temp[0][0] = 1;  //3x3??λ??
//	T_inv_temp[1][1] = 1;
//	T_inv_temp[2][2] = 1;
	
	Matrix_3X1_mul(R_T,t,t_temp);
	
	t_temp[0][0] = t_temp[0][0] * (-1);
	t_temp[1][0] = t_temp[1][0] * (-1);
	t_temp[2][0] = t_temp[2][0] * (-1);
	
	for(int x=0;x<3;x++)
	{
		for(int y=0;y<3;y++)
		{
			T_inv[x][y] = R_T[x][y];
		}
	}
	T_inv[0][3] = t_temp[0][0];
	T_inv[1][3] = t_temp[1][0];
	T_inv[2][3] = t_temp[2][0];	
	T_inv[3][3] = 1;
	
    memcpy(result,T_inv, sizeof(T_inv[0][0])*16);	
}

float A_round(float data)
{
	int integer = data*1000000;
	return (integer/1000000.0);
}
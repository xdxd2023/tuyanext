#include "A_IK.h"


#define __sqrtf  sqrtf

void Inverse_Kinematics(float x,float y,float z,float *gamma,float *alfa,float *beta,LEGS_Enum legs)  //���ȵ��˶�ѧ���
{
	float dyz=0.0f,lyz=0.0f,lxzp=0.0f,n=0.0f;
	float alfa_xzp=0.0f,alfa_off=0.0f;
	float gamma_yz=0.0f,gamma_h_offset=0.0f;

	dyz=__sqrtf((powf(y,2)+powf(z,2)));  
	lyz=__sqrtf((powf(dyz,2)-powf(H,2)));
	
    if(legs==rf||legs==rb)
	{
		if(z==0||lyz==0)
		{
			return;
		}
		gamma_yz=-atanf(y/z);
		gamma_h_offset=-atanf(H/lyz);
		*gamma=gamma_yz-gamma_h_offset;  //�粿����Ƕ�
		*gamma=floor(*gamma*180.0f/pi+0.5f);

		lxzp=__sqrtf(pow(lyz,2)+pow(x,2));
		if(lxzp==0)
		{	
			return;
		}  	
		n=(powf(lxzp,2)-powf(HL,2)-powf(HU,2))/(2.0f*HU);		
		*beta=-acosf(n/HL);             //С�ȶ���Ƕ�
		*beta=floor(*beta*180.0f/pi+0.5f);

		alfa_xzp=-atanf(x/lyz);
		alfa_off=acosf((HU+n)/lxzp);
		*alfa=alfa_xzp+alfa_off;      //���ȶ���Ƕ�

		*alfa=floor(*alfa*180.0f/pi+0.5f);
	
	}
	if(legs==lf||legs==lb)
	{
		if(z==0||lyz==0)
		{
			return;
		}
		gamma_yz=-atanf(y/z);
		gamma_h_offset=atanf(H/lyz);
		*gamma=gamma_yz-gamma_h_offset;  //�粿����Ƕ�
		*gamma=floor(*gamma*180.0f/pi+0.5f);
		
		lxzp=__sqrtf(powf(lyz,2.0f)+powf(x,2.0f));	
		if(lxzp==0)
		{
			return;
		} 
		
		n=(powf(lxzp,2.0f)-powf(HL,2.0f)-powf(HU,2.0f))/(2.0f*HU);
		*beta=-acosf(n/HL);             //С�ȶ���Ƕ�
		*beta=floor(*beta*180.0f/pi+0.5f);
		alfa_xzp=-atanf(x/lyz);
		alfa_off=acosf((HU+n)/lxzp);
		*alfa=alfa_xzp+alfa_off;      //���ȶ���Ƕ�
		*alfa=floor(*alfa*180.0f/pi+0.5f);
	}

}

void Inverse_Kinematics_all(LEGS_COORDINATE_ *coordinate)
{
	Inverse_Kinematics(coordinate->rf_x,coordinate->rf_y,coordinate->rf_z,&rf_gamma,&rf_alfa,&rf_beta,rf);	
	Inverse_Kinematics(coordinate->rb_x,coordinate->rb_y,coordinate->rb_z,&rb_gamma,&rb_alfa,&rb_beta,rb);		
	Inverse_Kinematics(coordinate->lf_x,coordinate->lf_y,coordinate->lf_z,&lf_gamma,&lf_alfa,&lf_beta,lf);	
	Inverse_Kinematics(coordinate->lb_x,coordinate->lb_y,coordinate->lb_z,&lb_gamma,&lb_alfa,&lb_beta,lb);	
}
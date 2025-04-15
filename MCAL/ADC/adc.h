/*
 * adc.h
 *
 *  Created on: Sep 29, 2024
 *      Author: dell
 */

/*
 * adc.h
 *
 *  Created on: Apr 13, 2024
 *      Author: DELL
 */

#ifndef MCAL_ADC_ADC_H_
#define MCAL_ADC_ADC_H_

#include "Common/std_types.h"
//#include "Common/delay.h"

typedef volatile uint32 * register_t;
#define REGISTER_CAST(X) ((volatile uint32 *) (X))
#define REGISTER_VAL(X) *REGISTER_CAST(X)

#define ADC0_ACTSS_R            (REGISTER_VAL(0x40038000))
#define ADC0_RIS_R              (REGISTER_VAL(0x40038004))
#define ADC0_IM_R               (REGISTER_VAL(0x40038008))
#define ADC0_ISC_R              (REGISTER_VAL(0x4003800C))
#define ADC0_OSTAT_R            (REGISTER_VAL(0x40038010))
#define ADC0_EMUX_R             (REGISTER_VAL(0x40038014))
#define ADC0_USTAT_R            (REGISTER_VAL(0x40038018))
#define ADC0_TSSEL_R            (REGISTER_VAL(0x4003801C))
#define ADC0_SSPRI_R            (REGISTER_VAL(0x40038020))
#define ADC0_SPC_R              (REGISTER_VAL(0x40038024))
#define ADC0_PSSI_R             (REGISTER_VAL(0x40038028))
#define ADC0_SAC_R              (REGISTER_VAL(0x40038030))
#define ADC0_DCISC_R            (REGISTER_VAL(0x40038034))
#define ADC0_CTL_R              (REGISTER_VAL(0x40038038))
#define ADC0_SSMUX0_R           (REGISTER_VAL(0x40038040))
#define ADC0_SSCTL0_R           (REGISTER_VAL(0x40038044))
#define ADC0_SSFIFO0_R          (REGISTER_VAL(0x40038048))
#define ADC0_SSFSTAT0_R         (REGISTER_VAL(0x4003804C))
#define ADC0_SSOP0_R            (REGISTER_VAL(0x40038050))
#define ADC0_SSDC0_R            (REGISTER_VAL(0x40038054))
#define ADC0_SSMUX1_R           (REGISTER_VAL(0x40038060))
#define ADC0_SSCTL1_R           (REGISTER_VAL(0x40038064))
#define ADC0_SSFIFO1_R          (REGISTER_VAL(0x40038068))
#define ADC0_SSFSTAT1_R         (REGISTER_VAL(0x4003806C))
#define ADC0_SSOP1_R            (REGISTER_VAL(0x40038070))
#define ADC0_SSDC1_R            (REGISTER_VAL(0x40038074))
#define ADC0_SSMUX2_R           (REGISTER_VAL(0x40038080))
#define ADC0_SSCTL2_R           (REGISTER_VAL(0x40038084))
#define ADC0_SSFIFO2_R          (REGISTER_VAL(0x40038088))
#define ADC0_SSFSTAT2_R         (REGISTER_VAL(0x4003808C))
#define ADC0_SSOP2_R            (REGISTER_VAL(0x40038090))
#define ADC0_SSDC2_R            (REGISTER_VAL(0x40038094))
#define ADC0_SSMUX3_R           (REGISTER_VAL(0x400380A0))
#define ADC0_SSCTL3_R           (REGISTER_VAL(0x400380A4))
#define ADC0_SSFIFO3_R          (REGISTER_VAL(0x400380A8))
#define ADC0_SSFSTAT3_R         (REGISTER_VAL(0x400380AC))
#define ADC0_SSOP3_R            (REGISTER_VAL(0x400380B0))
#define ADC0_SSDC3_R            (REGISTER_VAL(0x400380B4))
#define ADC0_DCRIC_R            (REGISTER_VAL(0x40038D00))
#define ADC0_DCCTL0_R           (REGISTER_VAL(0x40038E00))
#define ADC0_DCCTL1_R           (REGISTER_VAL(0x40038E04))
#define ADC0_DCCTL2_R           (REGISTER_VAL(0x40038E08))
#define ADC0_DCCTL3_R           (REGISTER_VAL(0x40038E0C))
#define ADC0_DCCTL4_R           (REGISTER_VAL(0x40038E10))
#define ADC0_DCCTL5_R           (REGISTER_VAL(0x40038E14))
#define ADC0_DCCTL6_R           (REGISTER_VAL(0x40038E18))
#define ADC0_DCCTL7_R           (REGISTER_VAL(0x40038E1C))
#define ADC0_DCCMP0_R           (REGISTER_VAL(0x40038E40))
#define ADC0_DCCMP1_R           (REGISTER_VAL(0x40038E44))
#define ADC0_DCCMP2_R           (REGISTER_VAL(0x40038E48))
#define ADC0_DCCMP3_R           (REGISTER_VAL(0x40038E4C))
#define ADC0_DCCMP4_R           (REGISTER_VAL(0x40038E50))
#define ADC0_DCCMP5_R           (REGISTER_VAL(0x40038E54))
#define ADC0_DCCMP6_R           (REGISTER_VAL(0x40038E58))
#define ADC0_DCCMP7_R           (REGISTER_VAL(0x40038E5C))
#define ADC0_PP_R               (REGISTER_VAL(0x40038FC0))
#define ADC0_PC_R               (REGISTER_VAL(0x40038FC4))
#define ADC0_CC_R               (REGISTER_VAL(0x40038FC8))


#define ADC_REF_VOLTAGE  3.3
#define ADC_MAX_VALUE    4095



/*Initialization FUnction of the ADC*/
void ADC_init(void);


/*Function to start and end conversion of adc value*/
uint32 ADC_readValue(int channel);


#endif /* MCAL_ADC_ADC_H_ */

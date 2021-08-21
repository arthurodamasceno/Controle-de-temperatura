#ifndef FIR_FILTER_H
#define FIR_FILTER_H

#include <stdint.h>

#define FIR_FILTER_LENGTH 5

typedef struct{
	float buf[FIR_FILTER_LENGTH];
	uint8_t bufindex;
	
	float out;	
}FIRFilter;

void FIRFilter_Init(FIRFilter *fir);
float FIRFilter_Update(FIRFilter *fir, float inp);

#endif

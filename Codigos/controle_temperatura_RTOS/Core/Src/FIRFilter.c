#include "FIRFilter.h"

/* Designed filter coefficients*/
static float FIR_IMPULSE_RESPONSE[FIR_FILTER_LENGTH] = {0.02840647f, 0.23700821f, 0.46917063f, 0.23700821f, 0.02840647f};

void FIRFilter_Init(FIRFilter *fir){
	
	/* Clear filter buffer */
	for(uint8_t n=0; n<FIR_FILTER_LENGTH;n++){
		
		fir->buf[n] = 0.0f;
	
	}
	
	/* Reset buffer index */
	fir->bufindex = 0;
	
	/* Clear filter output */
	fir->out = 0.0f;
}

float FIRFilter_Update(FIRFilter *fir, float inp){
	
	/* Store latest sample in buffer */
	fir->buf[fir->bufindex] = inp;
	
	/* Increment buffer index and wrap around if necessary */
	fir->bufindex++;
	
	if(fir->bufindex == FIR_FILTER_LENGTH){
		
		fir->bufindex = 0;
		
	}
	
	/* Compute new output sample (via convolution) */
	fir->out = 0.0f;
	
	uint8_t sumIndex = fir->bufindex;
	
	for(uint8_t n=0; n<FIR_FILTER_LENGTH;n++){
		
		/* Decrement index and wrap if necessary */
		if(sumIndex>0){
			
			sumIndex--;
			
		}else{
			
			sumIndex = FIR_FILTER_LENGTH -1;
			
		}
		
		/* Multiply impulse response with shifted input sample and add to output */
		fir->out += FIR_IMPULSE_RESPONSE[n] * fir->buf[sumIndex];
	}
	
	/* Return filtered output */
	return fir->out;
	
}

/**
 ******************************************************************************
 * @file            FIRFilter.c
 * @brief           implmentação de um filtro digital tipo FIR
 * @author          Arthur Damasceno
 ******************************************************************************
 * @attention
 *
 * <h2><center>Controlador de temperatura</center></h2>
 *
 * Este código apresenta a implementação de um filtro digital passa baixas FIR
 *
 ******************************************************************************
 */
#include "FIRFilter.h"

/* Coeficientes projetados de filtro */
static float FIR_IMPULSE_RESPONSE[FIR_FILTER_LENGTH] = {-0.01238356f, 0.10332170f, 0.81812371f, 0.10332170f, -0.01238356f};

/**
 * @brief Função de inicialização Filtro FIR
 * @param FIRFilter *fir: ponteiro de estrutura do filtro
 * @retval None
 */
void FIRFilter_Init(FIRFilter *fir){
	
	/* Limpa buffer do filtro */
	for(uint8_t n=0; n<FIR_FILTER_LENGTH;n++){
		
		fir->buf[n] = 0.0f;
	
	}
	
	/* Reseta o index do buffer */
	fir->bufindex = 0;
	
	/* Limpa saída do filtro */
	fir->out = 0.0f;
}

/**
 * @brief Função de atualização do filtro FIR
 * @param FIRFilter *fir: ponteiro de estrutura do filtro
 * @param float inp: variável a ser filtrada
 * @retval float fir->out: variável filtrada
 */
float FIRFilter_Update(FIRFilter *fir, float inp){
	
	/* Salva ultima amostra no buffer */
	fir->buf[fir->bufindex] = inp;
	
	/* incrementa o index e "da a volta" no buffer circular caso necessário */
	fir->bufindex++;
	
	if(fir->bufindex == FIR_FILTER_LENGTH){
		
		fir->bufindex = 0;
		
	}
	
	/* Calcula nova saída via convolução */
	fir->out = 0.0f;
	
	uint8_t sumIndex = fir->bufindex;
	
	for(uint8_t n=0; n<FIR_FILTER_LENGTH;n++){
		
		/* Decrementa o index e "da a vola" no buffer circular caso necessário */
		if(sumIndex>0){
			
			sumIndex--;
			
		}else{
			
			sumIndex = FIR_FILTER_LENGTH -1;
			
		}
		
		/* Multiplica a resposta ao impulso com a amostra deslocada e soma a saída */
		fir->out += FIR_IMPULSE_RESPONSE[n] * fir->buf[sumIndex];
	}
	
	/* Retorna a saída filtrada */
	return fir->out;
	
}

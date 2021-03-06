/**
 ******************************************************************************
 * @file            main.c
 * @brief           Corpo principal do programa
 * @author          Arthur Damasceno
 * @author			Mateus Piccinin
 ******************************************************************************
 * @attention
 *
 * <h2><center>Controlador de temperatura</center></h2>
 *
 * Este código apresenta a implementação de um controlador de temperatura
 * utilizando a interface FreeRTOS.
 *
 ******************************************************************************
 */

#include "main.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include "FIRFilter.h"

#define r1 0.00021012f
#define p1 1.00000000f
#define k 0.06382217f
#define ksat 0.10000000f

#define CSen HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
#define CSdis HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);

#define SCK_H HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
#define SCK_L HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);

#define DWT_CTRL (*(volatile uint32_t*) 0XE0001000)

ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart1;

TaskHandle_t Temp_Task;
TaskHandle_t Filter_Task;
TaskHandle_t Control_Task;
TaskHandle_t Display_Task;

QueueHandle_t tempQueue;
QueueHandle_t filteredTempQueue;

FIRFilter tempFilter;

float filteredTemp = 0;
float ref = 0;
float dutyCycle = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART1_UART_Init(void);

void Temp_taskF(void *pvParameters);
void Filter_taskF(void *pvParameters);
void Control_taskF(void *pvParameters);
void Display_taskF(void *pvParameters);

/**
 * @brief  ponto de entrada da aplicação.
 * @retval int
 */
int main(void) {
	/* Reinicia todos os periféricos, inicializa a interface flash e o systick */
	HAL_Init();

	/* Configura o clock do sistema */
	SystemClock_Config();

	/* Inicializa todos os periféricos configurados */
	MX_GPIO_Init();
	MX_ADC1_Init();
	MX_TIM1_Init();
	MX_USART1_UART_Init();

	DWT_CTRL |= (1<<0);

	/* Inicializa o conversor AD */
	if (HAL_ADC_Start(&hadc1) != HAL_OK) {
		Error_Handler();
	}

	/* Inicializa o timer1 em modo PWM */
	if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) {
		Error_Handler();
	}

	/* Inicializa o filtro FIR */
	FIRFilter_Init(&tempFilter);

	SEGGER_SYSVIEW_Conf();

	/* Cria a fila de leituras de temperatura bruta */
	tempQueue = xQueueCreate(1, sizeof(float));
	if (tempQueue == 0) {
		Error_Handler();
	}

	/* Cria a fila de leituras de temperatura filtrada */
	filteredTempQueue = xQueueCreate(1, sizeof(float));
	if (tempQueue == 0) {
		Error_Handler();
	}

	/* Cria tasks na pilha do sistema */
	xTaskCreate(Temp_taskF, "TempTask", 128, NULL, 3, &Temp_Task);
	xTaskCreate(Filter_taskF, "FilterTask", 128, NULL, 2, &Filter_Task);
	xTaskCreate(Control_taskF, "ControlTask", 128, NULL, 4, &Control_Task);
	xTaskCreate(Display_taskF, "DisplayTask", 128, NULL, 1, &Display_Task);

	/* Inicializa o escalonador */
	vTaskStartScheduler();

	/* Loop infinito */
	while (1) {
	}
}

/**
 * @brief Configuração do clock do sistema
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
	PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief Função de inicialização do conversor AD
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void) {

	ADC_ChannelConfTypeDef sConfig = { 0 };

	hadc1.Instance = ADC1;
	hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc1.Init.ContinuousConvMode = ENABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 1;
	if (HAL_ADC_Init(&hadc1) != HAL_OK) {
		Error_Handler();
	}

	sConfig.Channel = ADC_CHANNEL_0;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES_5;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief Função de inicialização do timer 1
 * @param None
 * @retval None
 */
static void MX_TIM1_Init(void) {

	TIM_MasterConfigTypeDef sMasterConfig = { 0 };
	TIM_OC_InitTypeDef sConfigOC = { 0 };
	TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = { 0 };

	htim1.Instance = TIM1;
	htim1.Init.Prescaler = 1;
	htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim1.Init.Period = 18000 - 1;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.RepetitionCounter = 0;
	htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 0;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
	if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1)
			!= HAL_OK) {
		Error_Handler();
	}
	sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
	sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
	sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
	sBreakDeadTimeConfig.DeadTime = 0;
	sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
	sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
	sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
	if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig)
			!= HAL_OK) {
		Error_Handler();
	}

	HAL_TIM_MspPostInit(&htim1);
}

/**
 * @brief Função de inicialização UART1
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {

	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief Função de inicialização das portas gerais
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);

	GPIO_InitStruct.Pin = GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_3;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_4;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 * @brief Tarefa de leitura da temperatura
 * @note  Esta tarefa executa a leitura da temperatura armazenada na memória do módulo
 * 	      MAX6675 atravez de um bitbanging do protocolo SPI, ao fim da conversão o valor
 * 	      é adicionado a fila tempQueue
 * @param *pvParameters: (não utilizado) permite iniciar a função com valor inical
 * @retval None
 */
void Temp_taskF(void *pvParameters) {
	while (1) {
		uint8_t tempdata[16];
		uint16_t temp16 = 0;

		/* bitbanging protocolo SPI */
		CSen
		for (int i = 0; i < 16; i++) {
			SCK_H
			tempdata[i] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4);
			SCK_L
		}
		CSdis

		/* Conversão temperatura */
		if (tempdata[13] == 0) {

			for (int n = 1; n < 13; n++) {
				temp16 += tempdata[n] * (2048 / (1 << (n - 1)));
			}

		}

		float temp = (float) temp16 / 4;

		/* Adiciona a fila tempQueue */
		if (xQueueSend(tempQueue, &temp, 10) == pdPASS) {
		}

		/* Atraso para definição do período da tarefa */
		vTaskDelay(200); /*5Hz frequency*/
	}
}

/**
 * @brief Tarefa de filtro da variável temperatura
 * @note  Esta tarefa executa a chamada para o filtro FIR,
 * 	      após 5 atualizações o valor é adicionado a fila filteredTempQueue
 *
 * @param *pvParameters: (não utilizado) permite iniciar a função com valor inical
 * @retval None
 */
void Filter_taskF(void *pvParameters) {
	uint8_t aux = 0,aux2=1;
	while (1) {
		float rx_temp;
		/* Recebe da fila filteredTempQueue */
		if (xQueueReceive(tempQueue, &rx_temp, 10)) {
			aux++;
			/* Chamada do filtro FIR */
			filteredTemp = FIRFilter_Update(&tempFilter, rx_temp);

		}
		if(aux==4 && aux2==1){
			aux2=0;
			SEGGER_SYSVIEW_Start();
		}
		if (aux == 5) {

			aux = 0;
			/* Adiciona a fila filteredTempQueue */
			if (xQueueSend(filteredTempQueue, &filteredTemp, 10) == pdPASS) {
			}
		}
	}
}

/**
 * @brief Tarefa de atualização da referencia, calculo e execução da lei de controle
 * @note  Esta tarefa executa o calculo da lei de controle, atualizando o valor de referencia
 * 	      setando a razão cíclica da chave de saída ou ativando a ventoinha de resfrianmento
 *
 * @param *pvParameters: (não utilizado) permite iniciar a função com valor inical
 * @retval None
 */
void Control_taskF(void *pvParameters) {
	while (1) {
		float rx_filteredTemp;
		/* Recebe da fila filteredTempQueue */
		if (xQueueReceive(filteredTempQueue, &rx_filteredTemp, 10)) {
			/* Leitura da entrada analógica para calculo de referencia */
			HAL_ADC_PollForConversion(&hadc1, 10);
			ref = (float) HAL_ADC_GetValue(&hadc1) / 27.3; // leitura do potenciometro convertido em ref até 150°C

			/* Lei de controle */
			float u;
			static float up, uint;
			int flag_sat;
			float ek = ref - rx_filteredTemp;

			/* Controlador bang-bang ventoinha */
			if (ek < -15.0) {
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
			} else {
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
			}

			/* Anti-windup integrador */
			if (!flag_sat) {
				uint = uint * p1 + r1 * ek;

			} else if (flag_sat) {
				uint = (uint * p1 + r1 * ek) * ksat;
			}

			/* Proporcional */
			up = k * ek;

			/* Ação de controle */
			u = up + uint;

			/* Conversão período PWM */
			u = u * 4500.0;

			/* Limites de saturação de PWM */
			if (u > 18000.0) {
				u = 18000.0;
				flag_sat = 1;
			} else if (u < 0.0) {
				u = 0.0;
				flag_sat = 1;
			} else {
				u = u;
				flag_sat = 0;
			}

			/* Converte periodo do timer em razão cíclica */
			dutyCycle = u / 180.0;

			/* Seta periférico PWM */
			__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint32_t ) u);

		}

	}
}

/**
 * @brief Tarefa de atualização das variáveis no display
 * @note  Esta tarefa envia ao display TFT via UART os valores atualizados de referência,
 * 		  variável manipulada(razão cíclica) e variável de processo (temperatura)
 *
 * @param *pvParameters: (não utilizado) permite iniciar a função com valor inical
 * @retval None
 */
void Display_taskF(void *pvParameters) {
	while (1) {
		char str[100];
		/* Fim de comando definido pela API do display */
		uint8_t Cmd_End[3] = { 0xFF, 0xFF, 0xFF };

		/* Atualiza valor do setpoint */
		int32_t number = ref * 100;
		sprintf(str, "setPoint.val=%ld", number);
		HAL_UART_Transmit(&huart1, (uint8_t*) str, strlen(str), 10);
		HAL_UART_Transmit(&huart1, Cmd_End, 3, 10);

		/* Atualiza valor da variável de processo */
		number = filteredTemp * 100;
		sprintf(str, "filteredTemp.val=%ld", number);
		HAL_UART_Transmit(&huart1, (uint8_t*) str, strlen(str), 10);
		HAL_UART_Transmit(&huart1, Cmd_End, 3, 10);

		/* Atualiza valor da variável manipulada */
		number = dutyCycle * 100;
		sprintf(str, "dutyCycle.val=%ld", number);
		HAL_UART_Transmit(&huart1, (uint8_t*) str, strlen(str), 10);
		HAL_UART_Transmit(&huart1, Cmd_End, 3, 10);

		/* Atraso para definição do período da tarefa */
		vTaskDelay(1000); /*1Hz frequency*/
	}
}

/**
 * @brief  chamada da função de período
 * @note   Esta função atualiza o valor de "uwTick" utilizado como base de tempo do sistema
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM4) {
		HAL_IncTick();
	}
}

/**
 * @brief  Função executada em caso de erro na aplicação
 * @retval None
 */
void Error_Handler(void) {
	__disable_irq();
	while (1) {
	}
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reporta o arquivo e linha de erro
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif

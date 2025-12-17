#include "main.h"
#include <stdbool.h>
void SystemClock_Config(void);

#define BUFFER_SIZE 128 //Size of circular buffers
#define BUFFER_OVERFLOW_CONTROL 100 //Used to begin the overflow control
#define BUFFER_SAFE_RANGE 50//Used to stop the overflow control


struct cicrularBuffer{
	uint32_t getIndex;
	uint32_t putIndex;
	uint32_t nChars;
	char data[BUFFER_SIZE];


};

volatile struct cicrularBuffer TXbuffer = {0,0,0,{}};
volatile struct cicrularBuffer RXbuffer = {0,0,0,{}};
UART_HandleTypeDef uartHandle;


//-------LED Initialization-------
void LED2_Init(){
	LD2_GPIO_CLK_ENABLE();

	GPIO_InitTypeDef LD2_Init;
	LD2_Init.Pin = LD2_Pin;
	LD2_Init.Mode = GPIO_MODE_OUTPUT_PP;
	LD2_Init.Pull = GPIO_PULLUP;
	LD2_Init.Speed = GPIO_SPEED_FREQ_LOW;

	HAL_GPIO_Init(LD2_GPIO_Port, &LD2_Init);


}


//-----UART Initialization-----
void UART2_Init(){

	uartHandle.Instance = USART2;
	uartHandle.Init.Mode = UART_MODE_TX_RX;
	uartHandle.Init.WordLength = UART_WORDLENGTH_8B;
	uartHandle.Init.BaudRate = 9600;
	uartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
	uartHandle.Init.Parity = UART_PARITY_NONE;
	uartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	uartHandle.Init.StopBits = UART_STOPBITS_1;
	
	
	//if UART fail to initialize, signal an error
	if(HAL_UART_Init(&uartHandle) != HAL_OK) Error_Handler();


}


//-------Signals an error by LED blinking--------
void Error_Handler(){
	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
	while(true){
	HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
	HAL_Delay(500);
	}

}


//-------UART Interrupt Service Routine-----------
void USART2_IRQHandler(){

	//Reception
	if (((uartHandle.Instance -> SR & USART_SR_RXNE) !=0 ) && (uartHandle.Instance -> CR1 & USART_CR1_RXNEIE) !=0){
		RXbuffer.data[RXbuffer.putIndex] = uartHandle.Instance -> DR; 
		RXbuffer.putIndex++;
		
		
		//buffer wrap around
		if (RXbuffer.putIndex == BUFFER_SIZE){
			RXbuffer.putIndex = 0;
		}

		RXbuffer.nChars++;

	}


	//Transmission
	if (((uartHandle.Instance -> SR & USART_SR_TXE) != 0) && ((uartHandle.Instance -> CR1 & USART_CR1_TXEIE) !=0 )){
		
		//Check if any data is present in the buffer
		if (TXbuffer.nChars == 0){
			uartHandle.Instance -> CR1 &= ~(USART_CR1_TXEIE);
			return;
		}

		uartHandle.Instance -> DR = TXbuffer.data[TXbuffer.getIndex];

		TXbuffer.getIndex++;
		
		//buffer wrap around
		if (TXbuffer.getIndex == BUFFER_SIZE){
			TXbuffer.getIndex = 0;
		}


		TXbuffer.nChars--;

	}


}

//transfers the data between buffers buffers
void TransferChar(){

	char ch = RXbuffer.data[RXbuffer.getIndex];


	if(TXbuffer.nChars > BUFFER_OVERFLOW_CONTROL) return;
	TXbuffer.data[TXbuffer.putIndex] = ch;

	__disable_irq();
	TXbuffer.putIndex++;
	RXbuffer.getIndex++;
	TXbuffer.nChars++;
	RXbuffer.nChars--;
	__enable_irq();


	if(RXbuffer.getIndex == BUFFER_SIZE){
		RXbuffer.getIndex = 0;
	}

	if (TXbuffer.putIndex == BUFFER_SIZE){
		TXbuffer.putIndex = 0;
	}

	if((uartHandle.Instance -> CR1 & USART_CR1_TXEIE) == 0){
		uartHandle.Instance -> CR1 |= USART_CR1_TXEIE;
	}

}


int main(void)
{

	HAL_Init();
	SystemClock_Config();
	UART2_Init();
	NVIC_EnableIRQ(USART2_IRQn);

	while (1)
	{

		//It the Txbuffer is not empty, enable interrupts
		if (TXbuffer.nChars > 0){
			if((uartHandle.Instance -> CR1 & USART_CR1_TXEIE)==0){
				uartHandle.Instance -> CR1 |= USART_CR1_TXEIE;
			}
		}

		if(RXbuffer.nChars ==0){
			if((uartHandle.Instance -> CR1 & USART_CR1_RXNEIE) == 0){
				uartHandle.Instance -> CR1 |= USART_CR1_RXNEIE;
			}


		}else{

			//Handle the RXbuffer overflow,
			if(RXbuffer.nChars == BUFFER_OVERFLOW_CONTROL){


				//If the buffer overflow, wait for the TX register to empty and send a signal to stop the tramismission
				uartHandle.Instance -> CR1 &= ~(USART_CR1_RXNEIE);
				uartHandle.Instance -> CR1 &= ~(USART_CR1_TXEIE);
				while((uartHandle.Instance -> SR & USART_SR_TXE) ==0 ) continue;
				uartHandle.Instance -> DR = 0x13;

				uartHandle.Instance -> CR1 |= USART_CR1_TXEIE;

				//If TX buffer overflows, wait for it to empty
				if(TXbuffer.nChars > BUFFER_OVERFLOW_CONTROL){

					while(TXbuffer.nChars > BUFFER_SAFE_RANGE) continue;

				}


				//Wait fot the data to transfeer between buffers, to empty the RXbuffer
				while(RXbuffer.nChars > BUFFER_SAFE_RANGE){
					TransferChar();
				}

				uartHandle.Instance -> CR1 &= ~(USART_CR1_TXEIE);
				
				//After both buffers are ready to operate normally, sent the signal to renew the transmission
				while((uartHandle.Instance -> SR & USART_SR_TXE) == 0) continue;
				uartHandle.Instance -> DR = 0x11;
				uartHandle.Instance -> CR1 |= USART_CR1_TXEIE;
				uartHandle.Instance -> CR1 |= USART_CR1_RXNEIE;

			}

			TransferChar();
		}
	}
}


void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}


#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

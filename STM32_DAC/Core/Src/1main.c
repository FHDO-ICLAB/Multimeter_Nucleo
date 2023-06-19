/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdbool.h>
#include <ctype.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
DAC_HandleTypeDef hdac1;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_DAC1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define RxBuf_SIZE   64						//!< Größe des Rx-Puffers, der zum Empfang von Daten über UART verwendet wird.
#define MainBuf_SIZE 64                     //!< Größe des Hauptpuffers, der zum Speichern der über UART empfangenen Daten verwendet wird.
#define commandBuf_SIZE 64

uint8_t RxBuf[RxBuf_SIZE];                  //!< Rx-Puffer für den Empfang von Daten über UART.
uint8_t MainBuf[MainBuf_SIZE];				//!< Hauptpuffer, der zum Speichern der über UART empfangenen Daten verwendet wird.
//float value  = 0.0;                       //!< Float-Variable zum Speichern des Output wertes des DAC-Kanals.
float value1 = 0.0;                         //!< Float-Variable zum Speichern des Output wertes des DAC-Kanals.
float value2 = 0.0;                         //!< Float-Variable zum Speichern des Output wertes des DAC-Kanals.

uint32_t VAR = 0;	                        //!< VAR zur Speicherung des jeweiligen Digitalwertes für den Output des DAC.
static uint16_t MainBufCounter ;            //!< Statische Variable, die den aktuellen Eintrag des Main-Buffers festhält
char str[20]={'0'};
char ch = '\n' ;

uint8_t commandBuf[commandBuf_SIZE];                        //Dieser Puffer wird verwendet, um den empfangenen Befehl zu speichern.
uint8_t activeChannel = 0;                                  //Dieses Flag wird verwendet, um den aktiven Kanalwert zu speichern
bool newCommandReceived = false;                            //Dieses Flag wird verwendet, um zu prüfen, ob der neue Befehl empfangen wurde.

bool isChannel1Active = false;                              //Diese Flaggen zeigen den Status des Kanals an
bool isChannel2Active = false;

char *retCommand1;                                          //Diese werden zur Überprüfung des empfangenen Befehls verwendet.
char *retCommand2;
char *retCommand3;
char *retCommand4;
char *retCommand5;


/* Diese Funktion ist die Callback-Funktion des UART-Empfangsinterrupts.
   Immer wenn Daten am RX-Pin von UART verfügbar sind, wird diese Funktion ausgeführt.
*/
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	//Variable, die zum Kopieren von Daten aus dem RX-Puffer in den Hauptpuffer verwendet wird.
	uint8_t Counter = 0 ;

	//Prüfen, ob der Interrupt vom USART2 kommt. Da wir derzeit USART 2 verwenden
	if(huart -> Instance == USART2 )
	{
		//Schleife zum Kopieren der Daten vom Rx-Puffer in den Hauptpuffer.
		while(Counter <= Size)
		{
			//Kopieren von Daten aus dem Rx-Puffer in den Hauptpuffer.
			MainBuf[MainBufCounter] = RxBuf[Counter++] ;

			//Prüfen Sie auf das Zeichen \n, das angibt, dass der vollständige Befehl empfangen worden ist.
			if(MainBuf[MainBufCounter] == '\n')
			{
				for(int i = 0 ; i < (MainBufCounter); i++)  //den empfangenen Befehl in den Befehlspuffer einfügen.
				{
					commandBuf[i] =MainBuf[i];

				}

				newCommandReceived = true;  //Flagge, die anzeigt, dass der neue Befehl empfangen wurde.
			}

			//Prüfen Sie, dass am Ende jeder Datenkopie keine Null hinzugefügt wird.
			if(Counter - 1 != Size)
			{
				//Inkrementieren des Hauptpufferzählers
				MainBufCounter++;
			}

			//Prüfen, ob der Hauptpuffer voll mit Daten ist. In diesem Fall werden die Daten von Anfang an ersetzt.
			if(MainBufCounter > 64 )
		    {
				//Zurücksetzen des Hauptpufferzählers
		    	MainBufCounter = 0;
		    }
		}

		//Diese Funktion wird verwendet, um Daten von UART mit DMA zu empfangen, bis der Datenpuffer voll ist oder die IDLE Line erkannt wird.
		HAL_UARTEx_ReceiveToIdle_DMA(&huart2, RxBuf, RxBuf_SIZE);
		//Deaktivieren Sie den Interrupt für die halb übertragenen Daten.
		__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
	}
}
/*
 * Benutzerdefinierte Funktion zur Übertragung von String-Daten über UART.
 * Nimmt einen String vom Benutzer.

 * */

void Uprintf(char *str)
{
	//Funktion zur Übertragung von Daten auf UART.
	HAL_UART_Transmit(&huart2 ,(uint8_t*) str, strlen(str),1000);
}
//Diese Funktion wird verwendet, um den Float-Wert aus einer String zu erhalten.
double get_double(const unsigned char *str)
{
	/* Erstes Überspringen nicht-ziffriger Zeichen */
	    /* Sonderfall zur Behandlung negativer Zahlen */
    while (*str && !(isdigit(*str) || ((*str == '-' || *str == '+') && isdigit(*(str + 1)))))
        str++;

    /* Das Parsen in ein Double */
    return strtod((const char *)str, NULL);
}
//Diese Funktion wird bei der Umwandlung von Float in String verwendet.
int n_tu(int number, int count)
{
    int result = 1;
    while(count-- > 0)
        result *= number;

    return result;
}
//Diese Funktion wird verwendet, um Float-Werte in Strings umzuwandeln.
void float_to_string(float f, char r[])
{
    long long int length, length2, i, number, position, sign;
    float number2;

    sign = -1;    // -1 == positive Zahl
    if (f < 0)
    {
        sign = '-';
        f *= -1;
    }

    number2 = f;
    number = f;
    length = 0;  // Größe des Dezimalteils
    length2 = 0; // Größe des Zehntels

    /* Berechnung des zehnten Teils der Länge2 */
    while( (number2 - (float)number) != 0.0 && !((number2 - (float)number) < 0.0) )
    {
         number2 = f * (n_tu(10.0, length2 + 1));
         number = number2;

         length2++;
    }

    /* Berechnung der Länge des Dezimalteils */
    for (length = (f > 1) ? 0 : 1; f > 1; length++)
        f /= 10;

    position = length;
    length = length + 1 + length2;
    number = number2;
    if (sign == '-')
    {
        length++;
        position++;
    }

    for (i = length; i >= 0 ; i--)
    {
        if (i == (length))
            r[i] = '\0';
        else if(i == (position))
            r[i] = '.';
        else if(sign == '-' && i == 0)
            r[i] = '-';
        else
        {
            r[i] = (number % 10) + '0';
            number /=10;
        }
    }
}
char* lowercase(char* s) {
  for(char *p=s; *p; p++) *p=tolower(*p);
  return s;
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  HAL_Init();

  SystemClock_Config();

  /* Initialisierung aller konfigurierten Peripheriegeräte */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DAC1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  //Diese Funktion wird verwendet, um Daten von UART mit DMA zu empfangen, bis der Datenpuffer voll ist oder die idle line erkannt wird.
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, RxBuf, RxBuf_SIZE);
  //Deaktivieren des Interrupts "Halbe Daten übertragen"..
  __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
  //Aktivieren Sie den Output von DAC-Kanal 1;
 HAL_DAC_Start(&hdac1 , DAC_CHANNEL_1);
 isChannel1Active = true;
 activeChannel = 1;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  //Infinite loop to keep MCU busy.


  while (1)
  {

   if(newCommandReceived == true)                   //Prüfen, ob der neue Befehl empfangen wurde.
	    {


	  	retCommand1 = strstr(lowercase((char *)commandBuf), "instrument:select?");//Prüfen, ob der "INSTrument:SELect?" Befehl empfangen wurde.
        if ((retCommand1==0)) {retCommand1 = strstr(lowercase((char *)commandBuf), "inst:sel?");}

        retCommand2 = strstr(lowercase((char *)commandBuf), "source:voltage:level?");      //Prüfen, ob der "SOURce:VOLTage:LEVel?" Befehl empfangen wird.
	  	if ((retCommand2==0)) {retCommand2 = strstr(lowercase((char *)commandBuf), "sour:volt:lev?");}

	  	retCommand3 = strstr(lowercase((char *)commandBuf), "instrument:select:output1"); //Prüfen, ob der Befehl "INSTrument:SELect:OUTPut1" empfangen wird.
	  	if ((retCommand3==0)) {retCommand3 = strstr(lowercase((char *)commandBuf), "inst:sel:outp1");}

	  	retCommand4 = strstr(lowercase((char *)commandBuf), "instrument:select:output2");  //Prüfen, ob der Befehl "INSTrument:SELect:OUTPut2" empfangen wird.
	  	if ((retCommand4==0)) {retCommand4 = strstr(lowercase((char *)commandBuf), "inst:sel:outp2");}

	  	retCommand5 = strstr(lowercase((char *)commandBuf), "source:voltage:level:");    //Prüfen, ob der "SOURce:VOLTage:LEVel:" Befehl empfangen wird.
	  	if ((retCommand5==0)) {retCommand5 = strstr(lowercase((char *)commandBuf), "sour:volt:lev:");}

	   if(retCommand1)
	      {
	         if(activeChannel == 1u)     //Prüfung auf den aktiven Kanal 1
	         {
	      	   Uprintf("1\n");   //Senden Sie den aktiven Kanal-String zurück
	         }
	         else
	         if(activeChannel == 2u)     //Prüfung auf den aktiven Kanal 2
	         {
	      	   Uprintf("2\n");   //Senden Sie den aktiven Kanal-String zurück
	         }

	         newCommandReceived = false;            //Rücksetzen des commandBuf und des MainBuf sowie des Flags für den neuen empfangenen Befehl
	         memset(commandBuf , 0 ,commandBuf_SIZE*(sizeof(commandBuf[0])));
	         MainBufCounter = 0 ;
	         memset(MainBuf , 0 ,MainBuf_SIZE*(sizeof(MainBuf[0])));

	      }
	      if(retCommand2)
	      {
	    	if(activeChannel == 1u)
	    	{
	    	   float_to_string(value1,str);               //den Spannungswert in einen Stringwert umwandeln.
	    	   Uprintf(strncat((char *)str,&ch,1));   //Senden des Spannungswertes über UART
	    	}
	    	else
	        {
	    		if(activeChannel == 2u)
				{
	    			float_to_string(value2,str);               //den Spannungswert in einen Stringwert umwandeln.
	    			Uprintf(strncat((char *)str,&ch,2));   //Senden des Spannungswertes über UART
				}
	    	}
			newCommandReceived = false;            //Rücksetzen des commandBuf und des MainBuf sowie des Flags für den neuen empfangenen Befehl
	      	memset(commandBuf , 0 ,commandBuf_SIZE*(sizeof(commandBuf[0])));
	      	MainBufCounter = 0 ;
	      	memset(MainBuf , 0 ,MainBuf_SIZE*(sizeof(MainBuf[0])));
	      }
	      if(retCommand3)
	      {
	          if(isChannel1Active == false)
	          {
	        	//Aktivieren Sie den Output von DAC-Kanal 1;
	            HAL_DAC_Start(&hdac1 , DAC_CHANNEL_1);
	            isChannel1Active = true;
	            //Digitale Wertberechnung zum Schreiben auf den DAC.
	            VAR = value1 * (0xfff + 1) / 3.3;
	            //Diese Funktion schreibt den berechneten digitalen Wert in den DAC-Kanal..
	            HAL_DAC_SetValue(&hdac1 , DAC_CHANNEL_1 , DAC_ALIGN_12B_R , VAR);

	          }

	          activeChannel = 1u;
	          //Rücksetzen des commandBuf und des MainBuf und des empfangenen neuen Befehls
	      	newCommandReceived = false;
	      	memset(commandBuf , 0 ,commandBuf_SIZE*(sizeof(commandBuf[0])));
	      	MainBufCounter = 0 ;
	      	memset(MainBuf , 0 ,MainBuf_SIZE*(sizeof(MainBuf[0])));
	      }

	      if(retCommand4)
	      {
	      	if(isChannel2Active == false)
	      	{
	      		//Aktivieren Sie den Output von DAC-Kanal 2;
	      	    HAL_DAC_Start(&hdac1 , DAC_CHANNEL_2);
	      	    isChannel2Active = true;
	      	    //Digitale Wertberechnung zum Schreiben auf den DAC.
	      	  	VAR = value2 * (0xfff + 1) / 3.3;
	      	  	//Diese Funktion schreibt den berechneten digitalen Wert in den DAC-Kanal.
	      	  	HAL_DAC_SetValue(&hdac1 , DAC_CHANNEL_2 , DAC_ALIGN_12B_R , VAR);
	      	}

	      	activeChannel = 2u;
	      	//Rücksetzen des commandBuf und des MainBuf sowie des Flags für den neuen empfangenen Befehl
	      	newCommandReceived = false;
	      	memset(commandBuf , 0 ,commandBuf_SIZE*(sizeof(commandBuf[0])));
	      	MainBufCounter = 0 ;
	      	memset(MainBuf , 0 ,MainBuf_SIZE*(sizeof(MainBuf[0])));
	      }
	      if(retCommand5)

	      {
	         if(activeChannel == 1u)
	         {
	        	//Extrahieren Sie den Spannungswert aus dem Befehl.
	        	value1 = get_double(commandBuf);
	            //Digitale Wertberechnung zum Schreiben auf den DAC.
	        	VAR = value1 * (0xfff + 1) / 3.3;
	        	//Diese Funktion schreibt den berechneten digitalen Wert in den DAC-Kanal..
	      	    HAL_DAC_SetValue(&hdac1 , DAC_CHANNEL_1 , DAC_ALIGN_12B_R , VAR);
	         }
	         else
	         if(activeChannel == 2u)
	  	     {
	        	 //Extrahieren Sie den Spannungswert aus dem Befehl.
	             value2 = get_double(commandBuf);
	        	 //Digitale Wertberechnung zum Schreiben auf den DAC.
	             VAR = value2 * (0xfff + 1) / 3.3;
	        	 //Diese Funktion schreibt den berechneten digitalen Wert in den DAC-Kanal.
	      	     HAL_DAC_SetValue(&hdac1 , DAC_CHANNEL_2 , DAC_ALIGN_12B_R , VAR);
	  	    }

	         //Rücksetzen des commandBuf und des MainBuf sowie des Flags für den neuen empfangenen Befehl
	         newCommandReceived = false;
	         memset(commandBuf , 0 ,commandBuf_SIZE*(sizeof(commandBuf[0])));
	         MainBufCounter = 0 ;
	         memset(MainBuf , 0 ,MainBuf_SIZE*(sizeof(MainBuf[0])));
	      }
	    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief DAC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC1_Init(void)
{

  /* USER CODE BEGIN DAC1_Init 0 */

  /* USER CODE END DAC1_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC1_Init 1 */

  /* USER CODE END DAC1_Init 1 */
  /** DAC Initialization
  */
  hdac1.Instance = DAC1;
  if (HAL_DAC_Init(&hdac1) != HAL_OK)
  {
    Error_Handler();
  }
  /** DAC channel OUT1 config
  */
  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE;
  sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /** DAC channel OUT2 config
  */
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC1_Init 2 */

  /* USER CODE END DAC1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

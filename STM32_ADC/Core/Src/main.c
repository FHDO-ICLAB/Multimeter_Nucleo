/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_adc.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>




/* Private variables ---------------------------------------------------------*/
#define RxBuf_SIZE   64						//!< Größe des Rx-Puffers, der zum Empfang von Daten über UART verwendet wird.
#define MainBuf_SIZE 64                     //!< Größe des Hauptpuffers, der zum Speichern der über UART empfangenen Daten verwendet wird.
#define commandBuf_SIZE 64

uint8_t RxBuf[RxBuf_SIZE];                  //!< Rx-Puffer für den Empfang von Daten über UART.
uint8_t MainBuf[MainBuf_SIZE];				//!< Hauptpuffer, der zum Speichern der über UART empfangenen Daten verwendet wird.
uint16_t adc_value;
uint8_t maxRes=12;
uint8_t minRes=6;

int maxAp=24.5*256;  //Vordefinierte Maximalapertur, hier bezogen auf eine Samplingzeit von 24,5 Zyklen und maximale Ratio
int minAp=24.5*2;   //Vordefinierte Minimalperture
int currentAp;
int division=4095;   //Initialisierung der Division für einen 12-Bit-ADC, die zur Umrechnung des ADC-Wertes in Volt verwendet wird.

ADC_ChannelConfTypeDef sConfig = {0};
//Instanz der Struktur ADC_ChannelConfTypeDef für die Konfiguration von ADC-Kanalparametern erstellen

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;


uint8_t commandBuf[commandBuf_SIZE];                        //In diesem Puffer wird der empfangene Befehl gespeichert.
bool newCommandReceived = false;                            //Dieses Flag wird verwendet, um zu prüfen, ob der neue Befehl empfangen wurde.
static uint16_t MainBufCounter ;            //!< Statische Variable, die den aktuellen Eintrag des Main-Buffers enthält.



/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	uint8_t Counter = 0 ;  	// Hilfsvariable zum Umkopieren von Daten aus dem RX-Puffer in den zentralen Puffer

	if(huart -> Instance == USART2 )   // Überprüfung, ob der Interrupt von USART2 stammt, da wir aktuell USART 2 einsetzen
	{

		while(Counter <= Size)  // Durchlauf zur Übertragung der Daten vom Rx-Puffer in den zentralen Puffer
		{
			//Prüfen, ob der Hauptpuffer voll mit Daten ist. In diesem Fall werden die Daten von Anfang an ersetzt.
			if(MainBufCounter > 64 )
		    {
				//Zurücksetzen des Hauptpufferzählers
		    	MainBufCounter = 0;
		    }
			MainBuf[MainBufCounter] = RxBuf[Counter++] ; // Übertragung der Daten vom Rx-Puffer in den zentralen Puffer
			if(MainBuf[MainBufCounter] == '\n')     // Überprüfung auf das Zeichen \n, das signalisiert, dass der vollständige Befehl eingegangen ist
			{for(int i = 0 ; i < (MainBufCounter); i++)  // Einfügen des empfangenen Befehls in den Befehlspuffer
				{
					commandBuf[i] =MainBuf[i];
				}
				newCommandReceived = true;  //Statusflag, das signalisiert, dass ein neuer Befehl eingegangen ist
			}

			if(Counter - 1 != Size)    // Sicherstellen, dass nach jeder Datenkopie kein Nullzeichen hinzugefügt wird
			{
				//Inkrementieren des Hauptpufferzählers
				MainBufCounter++;
			}
		}
		//Diese Funktion wird verwendet, um Daten von UART mit DMA zu empfangen, bis der Datenpuffer voll ist oder die IDLE Line erkannt wird.
		HAL_UARTEx_ReceiveToIdle_DMA(&huart2, RxBuf, RxBuf_SIZE);
		//Deaktivieren Sie den Interrupt für die halb übertragenen Daten.
		__HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
	}
}
/*

 * */


void Uprintf(char *str)
{
	//Funktion zur Übertragung von Daten auf UART.
	HAL_UART_Transmit(&huart2 ,(uint8_t*) str, strlen(str),HAL_MAX_DELAY);
}

char* lowercase(char* s)   // Diese Funktion wandelt alle Zeichen in einem String in Kleinbuchstaben um
{
  for(char *p=s; *p; p++) *p=tolower(*p);   // Durchläuft jeden Charakter im String und wandelt ihn in Kleinbuchstaben um
  return s;
}

const char* get_adc_channel_string(uint32_t channel) // Diese Funktion gibt einen String zurück, der dem übergebenen ADC-Kanal entspricht
{
      switch(channel) {    // Switch-Case-Struktur zur Auswahl des korrekten Kanalstrings basierend auf dem eingegebenen channel-Wert
          case ADC_CHANNEL_1:
              return "CHANNEL_1";
          case ADC_CHANNEL_2:
              return "CHANNEL_2";
          case ADC_CHANNEL_3:
              return "CHANNEL_3";
          case ADC_CHANNEL_4:
              return "CHANNEL_4";
          case ADC_CHANNEL_5:
              return "CHANNEL_5";
          case ADC_CHANNEL_11:
              return "CHANNEL_11";
          case ADC_CHANNEL_12:
              return "CHANNEL_12";
          case ADC_CHANNEL_13:
              return "CHANNEL_13";
          case ADC_CHANNEL_VREFINT:
              return "CHANNEL_VREFINT";
          default:
              return "Channel not selected";
      }
  }

void convertStringToADCChannel( char* str)   // Diese Funktion konvertiert einen String zu einem ADC-Kanal
{

      if (strcmp(str, "ADC_CHANNEL_1") == 0) {  // Verwendet eine if-else-if-Struktur, um den String
    	                                         //zu überprüfen und den entsprechenden ADC-Kanal zuzuweisen
    	  sConfig.Channel  = ADC_CHANNEL_1;
      } else if (strcmp(str, "ADC_CHANNEL_2") == 0) {
    	  sConfig.Channel  = ADC_CHANNEL_2;
      } else if (strcmp(str, "ADC_CHANNEL_3") == 0) {
    	  sConfig.Channel  = ADC_CHANNEL_3;
      } else if (strcmp(str, "ADC_CHANNEL_4") == 0) {
    	  sConfig.Channel  = ADC_CHANNEL_4;
      } else if (strcmp(str, "ADC_CHANNEL_5") == 0) {
    	  sConfig.Channel  = ADC_CHANNEL_5;
      } else if (strcmp(str, "ADC_CHANNEL_11") == 0) {
    	  sConfig.Channel  = ADC_CHANNEL_11;
      } else if (strcmp(str, "ADC_CHANNEL_12") == 0) {
    	  sConfig.Channel  = ADC_CHANNEL_12;
      } else if (strcmp(str, "ADC_CHANNEL_13") == 0) {
    	  sConfig.Channel  = ADC_CHANNEL_13;
      } else if (strcmp(str, "ADC_CHANNEL_VREFINT") == 0) {
    	  sConfig.Channel  = ADC_CHANNEL_VREFINT;
      }

 	  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
 	  	  {
 	  	    Error_Handler();
 	  	  }
 	 HAL_Delay(10);
  }


uint16_t extract_integer_from_command(const char* commandBuf, const char* command) {
	// Extrahieren der ganzzahligen Teilzeichenkette aus dem Befehl
	char integer_str[commandBuf_SIZE];
	strlcpy(integer_str, lowercase((char *)commandBuf) + strlen(command), commandBuf_SIZE);
	// Umwandlung der String in eine Ganzzahl
	uint16_t value = atoi(integer_str);
    return value;
}

const char* get_adc_resolution_string(uint32_t resolution)  // Diese Funktion gibt einen String zurück, der der übergebenen ADC-Auflösung entspricht
{
      switch(resolution) {   // Switch-Case-Struktur zur Auswahl des korrekten Auflösungsstrings basierend auf dem eingegebenen resolution-Wert
          case ADC_RESOLUTION_6B:
              return "6 Bits";
          case ADC_RESOLUTION_8B:
              return "8 Bits";
          case ADC_RESOLUTION_10B:
              return "10 Bits";
          case ADC_RESOLUTION_12B:
              return "12 Bits";


          default:
              return "Unbekannt";
      }
  }
void setADCResoltuion( uint8_t resolution)  // Diese Funktion stellt die adc-Auflösung ein, abhängig von dem angegebenen Parameter
{

    switch(resolution) {   // Verwendet eine Switch-Case-Struktur, um die eingegebene Auflösung zu überprüfen und die entsprechende ADC-Auflösung zuzuweisen
        case 6:
      	  hadc1.Init.Resolution  = ADC_RESOLUTION_6B;
       	 division=63;   //(2^6)-1 Division ist der Parameter, um den ADC-Wert als Volt in Abhängigkeit von der Auflösung zu erhalten
       	break;

        case 8:
      	  hadc1.Init.Resolution  = ADC_RESOLUTION_8B;
       	 division=255;
       	break;

        case 10:
      	  hadc1.Init.Resolution  = ADC_RESOLUTION_10B;
       	 division=1023;
       	break;

        case 12:
      	  hadc1.Init.Resolution  = ADC_RESOLUTION_12B;
       	 division=4095;
       	break;
    }

	  if (HAL_ADC_Init(&hadc1) != HAL_OK)
	   {
	     Error_Handler();
	   }

	  HAL_Delay(10);
}
int getRatio(uint32_t ratio)  // Diese Funktion gibt das Verhältnis einer gegebenen ADC-Überabtastungsrate als Zahl zurück
{
      switch(ratio) {  // Switch-Case-Struktur zur Auswahl des korrekten Verhältnisses basierend auf dem eingegebenen ratio-Wert
          case ADC_OVERSAMPLING_RATIO_2:
              return 2;
          case ADC_OVERSAMPLING_RATIO_4:
              return 4;
          case ADC_OVERSAMPLING_RATIO_8:
              return 8;
          case ADC_OVERSAMPLING_RATIO_16:
              return 16;
          case ADC_OVERSAMPLING_RATIO_32:
              return 32;
          case ADC_OVERSAMPLING_RATIO_64:
              return 64;
          case ADC_OVERSAMPLING_RATIO_128:
              return 128;
          case ADC_OVERSAMPLING_RATIO_256:
              return 256;

          default:
              return 0;
      }
  }

void setADCAperture( uint16_t aperture)  // Diese Funktion setzt die Apertur des ADC basierend auf einer gegebenen Apertur
{

    switch(aperture) {// Verwendet eine Switch-Case-Struktur, um die eingegebene Apertur zu überprüfen und die entsprechende ADC-Apertur zuzuweisen
 // Aperture basierend auf Oversampling Time von 24.4 Cycles
        case 49:
      	  hadc1.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_2;
      	  hadc1.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_1;
       	break;

        case 98:
      	  hadc1.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_4;
      	  hadc1.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_2;
       	break;

        case 196:
      	  hadc1.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_8;
      	  hadc1.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_3;
       	break;

        case 392:
      	  hadc1.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_16;
      	  hadc1.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_4;
       	break;

        case 784:
      	  hadc1.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_32;
      	  hadc1.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_5;
       	break;

        case 1568:
      	  hadc1.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_64;
      	  hadc1.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_6;
       	break;

        case 3136:
      	  hadc1.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_128;
      	  hadc1.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_7;
       	break;

        case 6272:
      	  hadc1.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_256;
      	  hadc1.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_8;
       	break;
    }

	  if (HAL_ADC_Init(&hadc1) != HAL_OK)
	   {
	     Error_Handler();
	   }

	  HAL_Delay(10);
}
int checkCommandType(char* commandBuf)  // Diese Funktion überprüft den Befehlstyp eines gegebenen Befehls
{

	// Verwendet eine Switch-Case-Struktur, um das erste Zeichen des Befehls zu prüfen und dann den einzelnen Befehl zu bestimmen.

    switch (commandBuf[0]) {
    case 'm':
        if (strncmp(commandBuf, "measure:voltage?", 16) == 0 || strncmp(commandBuf, "meas:volt?", 10) == 0) {
            return 1;
        }
        break;
    case 's':
        if (strncmp(commandBuf, "sense:channel?", 14) == 0 || strncmp(commandBuf, "sens:chan?", 10) == 0) {
            return 2;

        } else if (strncmp(commandBuf, "sense:channel ", 14) == 0|| strncmp(commandBuf, "sens:chan ", 10) == 0) {
            return 3;

        } else if (strncmp(commandBuf, "sense:resolution? max", 21) == 0 || strncmp(commandBuf, "sens:reso? max", 14) == 0) {
            return 4;

        } else if (strncmp(commandBuf, "sense:resolution? min", 21) == 0 || strncmp(commandBuf, "sens:reso? min", 14) == 0) {
            return 5;

        } else if (strncmp(commandBuf, "sense:resolution?", 17) == 0 || strncmp(commandBuf, "sens:reso?", 10) == 0) {
            return 6;

        } else if (strncmp(commandBuf, "sense:resolution ", 17) == 0|| strncmp(commandBuf, "sens:reso ", 10) == 0) {
            return 7;

        } else if (strncmp(commandBuf, "sense:aperture? max", 19) == 0 || strncmp(commandBuf, "sens:aper? max", 14) == 0) {
            return 8;

        } else if (strncmp(commandBuf, "sense:aperture? min", 19) == 0 || strncmp(commandBuf, "sens:aper? min", 14) == 0) {
            return 9;

        } else if (strncmp(commandBuf, "sense:aperture?", 15) == 0 || strncmp(commandBuf, "sens:aper?", 10) == 0) {
            return 10;

        } else if (strncmp(commandBuf, "sense:aperture ", 15) == 0 || strncmp(commandBuf, "sens:aper ", 10)== 0){
            return 11;
        }
        break;
        case 'e':
            if (strncmp(commandBuf, "enable_oversampling", 19) == 0) {
                return 12;
            }
            break;
        case 'd':
            if (strncmp(commandBuf, "disable_oversampling", 20) == 0) {
                return 13;
            }
            break;
    }


    return 0;

}
int main(void)
{

  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();


  hadc1.Init.Resolution = ADC_RESOLUTION_12B;    //HAL-Funktion zur Initialisierung der ADC-Auflösung,
  hadc1.Init.OversamplingMode = DISABLE;          // Oversampling deaktiviert als Startpunkt
  hadc1.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_256;   // Oversampling Ratio
  hadc1.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_8;   //Oversampling RightBitshift, abhängig von Ratio

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2, RxBuf, RxBuf_SIZE);
  //Diese Funktion wird zum Empfangen von Daten vom UART mit DMA verwendet, bis der Datenpuffer voll ist oder eine Leerzeile erkannt wird.
  __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
  //Deaktivieren des Interrupts



  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  if(newCommandReceived == true)                   //Prüfen, ob der neue Befehl empfangen wurde.
	 	  		   	    {

		  char uart_buffer[50]; // Puffer für UART-Übertragung

		  switch (checkCommandType(lowercase((char *)commandBuf))) // Befehlstyp prüfen
		  {


		      case 1: // measure:voltage?


		    	  HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);  // ADC-Kalibrierung starten
		    	  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&adc_value, 1);  //Startet den ADC-DMA-Modus für den ADC "hadc1"
		    	  HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);   //Wartet, bis die ADC-Konvertierung abgeschlossen ist.
		    	  HAL_Delay(100);  //Eine Pause von 100 Millisekunden, um sicherzustellen, dass der ADC-Wert stabil ist.

	  			    float voltage = (3.3f * adc_value) / (division) ; // ADC-Wert in Spannung umrechnen


	  			    sprintf(uart_buffer, "Voltage: %.3f V", voltage); //Schreibt den Spannungswert formatiert in den uart_buffer, um ihn später über UART zu übertragen.



		          break;
		      case 2:
		 	  		sprintf(uart_buffer, "Current Channel: %s", get_adc_channel_string(sConfig.Channel));

		 	  		//Schreibt den formatierten Kanalnamen in den uart_buffer zur späteren Übertragung über UART.
		          break;
		      case 3:


		    	  	 	  		if(strcmp(lowercase((char *)commandBuf), "sense:channel vrefint")==0||strcmp(lowercase((char *)commandBuf), "sens:chan vrefint")==0)
		    	  	 	  		{
		    	  	 	  			//Überprüft, ob der empfangene Befehl "sense:channel vrefint" oder "sens:chan vrefint" ist.

		    	  	 	  		convertStringToADCChannel("ADC_CHANNEL_VREFINT"); //ADC-Kanal auf "ADC_CHANNEL_VREFINT" setzen.

		    	  	 	  		}

		    	  	 	  		else if(strncmp(lowercase((char *)commandBuf), "sens:chan ", 10) == 0)
		    	  	 	  		{
		    	  	 	  		uint16_t channel= extract_integer_from_command(((char *)commandBuf),"sens:chan ");
		    	  	 	  		// Extrahiert die Kanalnummer aus dem Befehl "sense:channel x" und speichert sie in der Variable channel.
		    			    	char adc_channel_temp[20];
		    	  	 	  	    snprintf(adc_channel_temp, sizeof(adc_channel_temp), "ADC_CHANNEL_%u", channel);
		    	  	 	  	    //Erstellt einen temporären Puffer adc_channel_temp und formatiert den Kanalwert darin.
		    	  	 	  	    convertStringToADCChannel(adc_channel_temp); //ADC-Kanal entsprechend adc_channel_temp  setzen.
		    	  	 	  		}
		    	  	 	  		else {

		    	  	 	  		uint16_t channel= extract_integer_from_command(((char *)commandBuf),"sense:channel ");
                                // Extrahiert die Kanalnummer aus dem Befehl "sense:channel x" und speichert sie in der Variable channel.
		    			    	char adc_channel_temp[20];
		    	  	 	  	    snprintf(adc_channel_temp, sizeof(adc_channel_temp), "ADC_CHANNEL_%u", channel);
		    	  	 	  	    //Erstellt einen temporären Puffer adc_channel_temp und formatiert den Kanalwert darin.
		    	  	 	  	    convertStringToADCChannel(adc_channel_temp); //ADC-Kanal entsprechend adc_channel_temp  setzen.

		    	  	 	  		}

		    	  	 	  		sprintf(uart_buffer, "Selected Channel: %s", get_adc_channel_string(sConfig.Channel));
		    	  	 	  		// Schreibt den ausgewählten ADC-Kanal in den uart_buffer für die spätere Übertragung über UART.


		    	  	 	  	break;

		      case 4: // Maximale Auflösung erhalten
		              sprintf(uart_buffer, "Max ADC resolution: %d bits", maxRes); //Schreibt die maximale Auflösung des ADCs in den uart_buffer.


		          break;
		      case 5: // Mindestauflösung erhalten

		              sprintf(uart_buffer, "Min ADC resolution: %d bits", minRes); //Schreibt die minimale Auflösung des ADCs in den uart_buffer.

		          break;


		      case 6: // aktuelle Auflösung erhalten
		      {

		          sprintf(uart_buffer, "Current resolution: %s ", get_adc_resolution_string(hadc1.Init.Resolution));
		          //Schreibt die aktuelle Auflösung des ADCs in den uart_buffer unter Verwendung der Funktion get_adc_resolution_string.


		          break;
		      }

		      case 7:  //Set Resolution
		      {
		    	  if(strncmp(lowercase((char *)commandBuf), "sense:resolution min",20)==0 || strncmp(lowercase((char *)commandBuf), "sens:reso min",13)==0 ) // Befeh Überprüfung
		    	  {
	 	  	          setADCResoltuion(minRes);  //ADC-Auflösung entsprechend der angegebenen Auflösung setzen, hier Min Res

		    	  }
		    	  else if(strncmp(lowercase((char *)commandBuf), "sense:resolution max",20)==0 || strncmp(lowercase((char *)commandBuf), "sens:reso max",13)==0)
				  {
	 	  	          setADCResoltuion(maxRes); // ADC-Auflösung entsprechend der angegebenen Auflösung setzen, hier Max Res

				 		    	  }
		    	  else if (strncmp(lowercase((char *)commandBuf), "sens:reso ",10)==0){

		    		  uint16_t resolution = extract_integer_from_command(((char *)commandBuf),"sens:reso ");
		    		  //Extrahiert die Auflösungszahl aus dem Befehl "sens:reso x" und speichert sie in der Variable resolution.
	 	  	          setADCResoltuion(resolution);
		    	  }
		    	  else{

		    		  uint16_t resolution = extract_integer_from_command(((char *)commandBuf),"sense:resolution ");
	 	  	          setADCResoltuion(resolution);
		    	  }


		          sprintf(uart_buffer, "Selected resolution: %s ", get_adc_resolution_string(hadc1.Init.Resolution));
		          break;
		      }

		      case 8: // maximale Apertur erhalten

		              sprintf(uart_buffer, "Max Aperture: %d Cycles", maxAp);

		          break;
		      case 9: // minimale Apertur erhalten

		              sprintf(uart_buffer, "Min Aperture: %d Cycles", minAp);
		          break;

		      case 10: // aktuelle Apertur ermitteln

		    	      currentAp=24.5*getRatio(hadc1.Init.Oversampling.Ratio); // Berechnet die aktuelle Apertur des ADCs basierend auf dem Oversampling-Verhältnis
		              sprintf(uart_buffer, "Current Aperture: %d Cycles", currentAp);
		          break;

		      case 11: // Apertur einstellen


		    	  if(strncmp(lowercase((char *)commandBuf), "sense:aperture min",18)==0 || strncmp(lowercase((char *)commandBuf), "sens:aper min",13)==0)
		    	  {
		    		  setADCAperture(minAp);

		    	  }
		    	  else if(strncmp(lowercase((char *)commandBuf), "sense:aperture max",18)==0 || strncmp(lowercase((char *)commandBuf), "sens:aper max",13)==0)
				  {
		    		  setADCAperture(maxAp);


				  }else if(strncmp(lowercase((char *)commandBuf), "sens:aper ",10)==0) {

					  uint16_t aperture = extract_integer_from_command(((char *)commandBuf),"sens:aper ");
					  //Extrahiert den Aperturwert aus dem Befehl "sens:aper x" und speichert ihn in der Variable aperture.
		    		  setADCAperture(aperture);
		    		  //die Apertur des ADCs entsprechend dem angegebenen Wert setzen
	 	  	                                            }
		    	  else{

		    		  uint16_t aperture = extract_integer_from_command(((char *)commandBuf),"sense:aperture ");

		    		  setADCAperture(aperture);

	 	  	                                            }

		    	  int aperture=24.5*getRatio(hadc1.Init.Oversampling.Ratio);

		    	  sprintf(uart_buffer, "Selected aperture: %d Cycles",aperture);


		          break;



		      case 12: // Enable Oversampling


		              hadc1.Init.OversamplingMode = ENABLE; //Aktiviert das Oversampling für den ADC.
		              HAL_ADC_Init(&hadc1);  //nitialisiert den ADC nach der Aktivierung des Oversamplings.
		              sprintf(uart_buffer, "Oversampling enabled");

		          break;

		      case 13: // Disable Oversampling

		    	      hadc1.Init.OversamplingMode = DISABLE; ////Deaktiviert das Oversampling für den ADC.
		    	      HAL_ADC_Init(&hadc1);
		    	  	  sprintf(uart_buffer, "Oversampling disabled");
		          break;



		  }//switch end



	  	  Uprintf(uart_buffer);  // Überträgt den Inhalt von uart_buffer über UART.
		  memset(uart_buffer, 0, sizeof(uart_buffer)); //Löscht den Inhalt von uart_buffer, indem alle Bytes auf den Wert 0 gesetzt werden.
          newCommandReceived = false;            //Setzt das Flag newCommandReceived zurück, um anzuzeigen, dass kein neuer Befehl empfangen wurde.
          memset(commandBuf , 0 ,commandBuf_SIZE*(sizeof(commandBuf[0]))); //Löscht den Inhalt von commandBuf, indem alle Bytes auf den Wert 0 gesetzt werden. Dadurch wird der Befehlspuffer zurückgesetzt.
          MainBufCounter = 0 ; // Setzt den Zähler MainBufCounter auf 0, um den Hauptspeicherzähler zurückzusetzen.
          memset(MainBuf , 0 ,MainBuf_SIZE*(sizeof(MainBuf[0]))); //Löscht den Inhalt von MainBuf, indem alle Bytes auf den Wert 0 gesetzt werden.
}
}
  }
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

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
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};


  /* USER CODE BEGIN ADC1_Init 1 */
  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;

  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
  hadc1.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_RESUMED_MODE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure Regular Channel*/

  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_24CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
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

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

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

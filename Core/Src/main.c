/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "eeF4.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

#define ADC_BUF_SIZE 3
#define Channel_2_BUF_SIZE 10
#define Channel_3_BUF_SIZE 10
#define Channel_4_BUF_SIZE 10

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
static bool parse_kv_int(const char *line, const char *key,  uint32_t *outVal);
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
bool DMA_Channel_2_Was_Filled = false;
bool DMA_Channel_3_Was_Filled = false;
bool DMA_Channel_4_Was_Filled = false;

int Buffer2_TableCount = 0;
int Buffer3_TableCount = 0;
int Buffer4_TableCount = 0;

//volatile bool adc_ready = false;
volatile uint16_t ADC_MoistureSensor_Dev_1[Channel_2_BUF_SIZE];
volatile uint16_t ADC_MoistureSensor_Dev_2[Channel_3_BUF_SIZE];
volatile uint16_t ADC_LightSensor_Dev[Channel_4_BUF_SIZE];
volatile uint16_t adc_buffer[ADC_BUF_SIZE];

#define FLASH_MAGIC      0xA55A1235
typedef struct {
	uint32_t magic;
	uint32_t Sensor1DryValue;
	uint32_t Sensor1WetValue;
	uint32_t Sensor2DryValue;
	uint32_t Sensor2WetValue;
	uint32_t GetMoistureAvg_TimeLenghtDefault; // 20 sek
	uint32_t GetMoistureAvg_TimeLenghtFast; // 1sek
	uint32_t StartPumpWhenMoistureLower;
	uint32_t StopPumpWhenMoistureHigher;
	uint32_t lightTime;
} DataFlash;

DataFlash dane;
//uint32_t flash_addr = 0x000100;
uint32_t flash_address = EE_FLASH_BASE; // jesli offset + flash_addr;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
HAL_StatusTypeDef Flash_WriteStruct(uint32_t address, void *data, uint32_t size);
void Flash_ReadStruct(uint32_t address, void *data, uint32_t size);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	typedef enum {
		DARK =20,
		NIGHT_OR_MORNING = 90,
		BRIGHT = 99
	} Light_State;

  uint32_t TimeNOW = 0;

//  int GetMoistureAvg_TimeLenghtDefault = 20000; // 20 sek
//  int GetMoistureAvg_TimeLenghtFast = 1000; // 1sek
  int GetMoistureAvg_TimeLenght = 0;
  uint32_t GetSoilMoisture_FromTime = 0;

  int MoistureHumidityPercent_Dev_1 = 0;
  int MoistureHumidityPercent_Dev_2 = 0;
  int MoistureGeneral = 0;

  bool PumpIsRunnign = false;
//  int StartPumpWhenMoistureLower = 40;
//  int StopPumpWhenMoistureHigher = 75;

  bool waitFordark = false;
  bool LightTimerEnabled = false;
//  uint32_t lightTime = 3600 * 1000 * 10;
  uint32_t GetLightTime_FromTime = 0;

  int GetLightMesurment_TimeLenghtDefault = 60000; //
  uint32_t GetLightMesurment_FromTime = 0;

  int MixWater_TimeLenghtDefault = 10800000; // 3h
  uint32_t MixWater_FromTime = 0;

  int LightSensorPercent = 0;
  Light_State LightSensor = DARK;
  bool LightIsON = false;

  GPIO_PinState WaterSensor1State = GPIO_PIN_RESET;

//  int Sensor1DryValue = 2700; // przeniesione do EEPROM
//  int Sensor1WetValue = 2020;
//  int Sensor2DryValue = 2700;
//  int Sensor2WetValue = 2020;

  uint32_t LedSecond_FromTime=0; // nie dziala Led na balkonie
  uint32_t LedSecond=1000;

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  InitUSART();

  Send("Welcome waweroIT\r\n");
  HAL_Delay(1000);


	Send("Read from flash on start...\r\n");
	memcpy(&dane, (void*)flash_address, sizeof(DataFlash));

	if  (dane.magic != FLASH_MAGIC)
	{
	    // To pierwsze uruchomienie lub dane niewazne
		dane.magic = FLASH_MAGIC;
		dane.Sensor1DryValue = 2700;
		dane.Sensor1WetValue = 2020;
		dane.Sensor2DryValue = 2700;
		dane.Sensor2WetValue = 2020;
		dane.GetMoistureAvg_TimeLenghtDefault = 20000; // 20 sek
		dane.GetMoistureAvg_TimeLenghtFast = 1000; // 1sek
		dane.StartPumpWhenMoistureLower = 40;
		dane.StopPumpWhenMoistureHigher = 75;
		dane.lightTime = 3600 * 1000 * 10; // 10H
	    // Skasuj i zapisz domyslne wartosci do flash
		memoryPageErase(0);
		        /* Zapisz defaulty */
		Send("Write basic Configuration\r\n");
		if (Flash_WriteStruct(flash_address, &dane, sizeof(DataFlash)) != HAL_OK)
		{
		    Send("Saving error!\r\n");
		}

	}
	else
	{
		Send("Magic data ok.\r\n");

	    GetMoistureAvg_TimeLenght = dane.GetMoistureAvg_TimeLenghtDefault;
	}


  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, ADC_BUF_SIZE);
  HAL_TIM_Base_Start_IT(&htim3);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if (isRecivingComplete) //S1WValue:1900\r\n
	  {
	      char line[RX_BUFFER_SIZE];
	      // pobierz wszystkie dostępne linie (gdyby przyszło naraz kilka)
	      while (USART_TakeLine(line, sizeof(line))) {

	          bool matched = false;

	          matched |= parse_kv_int(line, "S1DValue", &dane.Sensor1DryValue);
	          matched |= parse_kv_int(line, "S1WValue", &dane.Sensor1WetValue);
	          matched |= parse_kv_int(line, "S2DValue", &dane.Sensor2DryValue);
	          matched |= parse_kv_int(line, "S2WValue", &dane.Sensor2WetValue);

	          matched |= parse_kv_int(line, "MoistureAvgTimeDefault", &dane.GetMoistureAvg_TimeLenghtDefault);
	          matched |= parse_kv_int(line, "MoistureAvgTimeFast", &dane.GetMoistureAvg_TimeLenghtFast);
	          matched |= parse_kv_int(line, "StartPumpWhenMoistureLower", &dane.StartPumpWhenMoistureLower);
	          matched |= parse_kv_int(line, "StopPumpWhenMoistureHigher", &dane.StopPumpWhenMoistureHigher);

	          matched |= parse_kv_int(line, "lightTime", &dane.lightTime);



	          uint8_t temp = 0;

	          if (parse_kv_int(line, "waterLvl", (uint32_t*) &temp)) {
					GPIO_PinState ss = HAL_GPIO_ReadPin(
							WaterSensor1_GPIO_Port, WaterSensor1_Pin);
					if (ss == GPIO_PIN_SET) {
						Sendf("Water level: LOW\r\n");
					}
					else
						Sendf("Water level: HIGH\r\n");
				}
	          else if (parse_kv_int(line, "runPump",(uint32_t*) &temp)) {
					if (temp == 1)
						HAL_GPIO_WritePin(PIN_PB10_Pump_GPIO_Port,
						PIN_PB10_Pump_Pin, GPIO_PIN_SET);
					else
						HAL_GPIO_WritePin(PIN_PB10_Pump_GPIO_Port,
						PIN_PB10_Pump_Pin, GPIO_PIN_RESET);
				}
	          else if (parse_kv_int(line, "lightOn", (uint32_t*)&temp)) {
					if (temp == 1)
						HAL_GPIO_WritePin(PIN_PB11_Light_GPIO_Port,
						PIN_PB11_Light_Pin, GPIO_PIN_SET);
					else
						HAL_GPIO_WritePin(PIN_PB11_Light_GPIO_Port,
						PIN_PB11_Light_Pin, GPIO_PIN_RESET);
				}
	          else if(parse_kv_int(line, "help", (uint32_t*)&temp))
	          {
	              Sendf("AvailConfig: S1DValue:%d\r\n", dane.Sensor1DryValue);
	              Sendf("AvailConfig: S1WValue:%d\r\n", dane.Sensor1WetValue);
	              Sendf("AvailConfig: S2DValue:%d\r\n", dane.Sensor2DryValue);
	              Sendf("AvailConfig: S2WValue:%d\r\n", dane.Sensor2WetValue);

	              Sendf("AvailConfig: MoistureAvgTimeDefault:%d\r\n", dane.GetMoistureAvg_TimeLenghtDefault);
	              Sendf("AvailConfig: MoistureAvgTimeFast:%d\r\n", dane.GetMoistureAvg_TimeLenghtFast);
	              Sendf("AvailConfig: StartPumpWhenMoistureLower:%d\r\n", dane.StartPumpWhenMoistureLower);
	              Sendf("AvailConfig: StopPumpWhenMoistureHigher:%d\r\n", dane.StopPumpWhenMoistureHigher);

	              Sendf("AvailConfig: lightTime:%d\r\n", dane.lightTime);
	              Sendf("other: waterLvl ,lightOn, runPump \r\n");

	          }
	          else if (matched)
	          {
	            Sendf("OK: %s\r\n", line);
	            GetMoistureAvg_TimeLenght = dane.GetMoistureAvg_TimeLenghtDefault;

	            Sendf("Save new config to flash..\r\n", line);
	      		memoryPageErase(0);
	      		if (Flash_WriteStruct(flash_address, &dane, sizeof(DataFlash)) != HAL_OK)
	      	    {
	      	    	Send("Saving error!\r\n");
	      	    }

	          } else {
	              Sendf("ERR: unknown cmd \"%s\"\r\n", line);
	          }
	      }
	      isRecivingComplete = false;
	  }

		 //if(adc_ready == true)
//		 if (hadc1.Instance == ADC1)
//		 {
				if(Buffer2_TableCount < Channel_2_BUF_SIZE && DMA_Channel_2_Was_Filled == false)
				{
					ADC_MoistureSensor_Dev_1[Buffer2_TableCount]= adc_buffer[0];
					Buffer2_TableCount++;
				}
				else
				{
					DMA_Channel_2_Was_Filled = true;
					Buffer2_TableCount = 0;
				}

				if(Buffer3_TableCount < Channel_3_BUF_SIZE && DMA_Channel_3_Was_Filled == false)
				{
					ADC_MoistureSensor_Dev_2[Buffer3_TableCount]= adc_buffer[1];
					Buffer3_TableCount++;
				}
				else
				{
					DMA_Channel_3_Was_Filled = true;
					Buffer3_TableCount = 0;
				}

				if(Buffer4_TableCount < Channel_4_BUF_SIZE && DMA_Channel_4_Was_Filled == false)
				{
					ADC_LightSensor_Dev[Buffer4_TableCount]= adc_buffer[2];
					Buffer4_TableCount++;
				}
				else
				{
					DMA_Channel_4_Was_Filled = true;
					Buffer4_TableCount = 0;
				}

				// adc_ready = false;
				 //HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, ADC_BUF_SIZE);
//		 }

	    WaterSensor1State = HAL_GPIO_ReadPin( WaterSensor1_GPIO_Port, WaterSensor1_Pin);

		TimeNOW = HAL_GetTick();

		if(((uint32_t)TimeNOW - GetSoilMoisture_FromTime) >= GetMoistureAvg_TimeLenght)
		{
			GetSoilMoisture_FromTime = TimeNOW;

			 if(DMA_Channel_2_Was_Filled && DMA_Channel_3_Was_Filled)
			 {

	//			Moisture soil 1.2 -5 Volt
	//			 MoistureHumidityPercent_Dev_1 = ScaleADC_To_Percent_Inverted_Ranged(ADC_MoistureSensor_Dev_1, Channel_2_BUF_SIZE, 4095, 2300);
	//			 MoistureHumidityPercent_Dev_2 = ScaleADC_To_Percent_Inverted_Ranged(ADC_MoistureSensor_Dev_2, Channel_3_BUF_SIZE, 4095, 2300);

				 //MakerSoil MoistureSensor 5 Volt
//				 MoistureHumidityPercent_Dev_1 = ScaleADC_To_Percent_Inverted_Ranged(ADC_MoistureSensor_Dev_1, Channel_2_BUF_SIZE, 4095, 2910);
//				 MoistureHumidityPercent_Dev_2 = ScaleADC_To_Percent_Inverted_Ranged(ADC_MoistureSensor_Dev_2, Channel_3_BUF_SIZE, 4095, 2840);

				 //MakerSoil MoistureSensor 3.3 Volt


	  			 MoistureHumidityPercent_Dev_1 = ScaleADC_To_Percent_Inverted_Ranged((uint16_t*)ADC_MoistureSensor_Dev_1, Channel_2_BUF_SIZE, dane.Sensor1DryValue, dane.Sensor1WetValue);
				 MoistureHumidityPercent_Dev_2 = ScaleADC_To_Percent_Inverted_Ranged((uint16_t*)ADC_MoistureSensor_Dev_2, Channel_3_BUF_SIZE, dane.Sensor2DryValue, dane.Sensor2WetValue);

//				 MoistureHumidityPercent_Dev_1 = ScaleADC_To_Percent_Inverted_Ranged(ADC_MoistureSensor_Dev_1, Channel_2_BUF_SIZE, 2600, 1830);
//				 MoistureHumidityPercent_Dev_2 = ScaleADC_To_Percent_Inverted_Ranged(ADC_MoistureSensor_Dev_2, Channel_3_BUF_SIZE, 2600, 1830);

				 uint32_t avg = 0;
					for(int i = 0; i < Channel_2_BUF_SIZE; i++)
					{
						avg += ADC_MoistureSensor_Dev_1[i];
					}
					avg = ((avg) / Channel_2_BUF_SIZE);

				 Sendf("Moisture sensor Dev 1  ADC:     %d \r\n", avg);
				 Sendf("Moisture sensor Dev 1: S1DValue:%d S1WValue:%d\r\n", dane.Sensor1DryValue,dane.Sensor1WetValue);
				 Sendf("Moisture sensor Dev 1: %d %%\r\n\r\n", MoistureHumidityPercent_Dev_1);

				 	avg = 0;
					for(int i = 0; i < Channel_3_BUF_SIZE; i++)
					{
						avg += ADC_MoistureSensor_Dev_2[i];
					}
					avg = ((avg) / Channel_3_BUF_SIZE);

				 Sendf("Moisture sensor Dev 2  ADC:     %d \r\n", avg);
				 Sendf("Moisture sensor Dev 2: S2DValue:%d S2WValue:%d\r\n", dane.Sensor2DryValue,dane.Sensor2WetValue);
				 Sendf("Moisture sensor Dev 2: %d %%\r\n\r\n", MoistureHumidityPercent_Dev_2);

				 MoistureGeneral = (MoistureHumidityPercent_Dev_1 + MoistureHumidityPercent_Dev_2)/2;

				 Sendf("Soil moisture avg: %d %%\r\n\r\n", MoistureGeneral);


				 DMA_Channel_2_Was_Filled = false;
				 DMA_Channel_3_Was_Filled = false;


				 if (WaterSensor1State != GPIO_PIN_SET)
				 {
					 if(MoistureGeneral <= dane.StartPumpWhenMoistureLower && PumpIsRunnign == false)
					 {
						 Sendf("Soil moisture avg: %d is <= %d \r\n", MoistureGeneral, dane.StartPumpWhenMoistureLower);

						 GetMoistureAvg_TimeLenght = dane.GetMoistureAvg_TimeLenghtFast;
						 HAL_GPIO_WritePin(PIN_PB10_Pump_GPIO_Port, PIN_PB10_Pump_Pin, GPIO_PIN_SET);
						 PumpIsRunnign = true;
					 }
				 }
				 else
				 {
					 Sendf("Soil moisture avg: %d\r\nWater level: LOW\r\n", MoistureGeneral, dane.StartPumpWhenMoistureLower);
				 }



				 if(PumpIsRunnign == true && WaterSensor1State == GPIO_PIN_SET)
				 {
					 Sendf("Water level: LOW\r\n");
					 HAL_GPIO_WritePin(PIN_PB10_Pump_GPIO_Port, PIN_PB10_Pump_Pin, GPIO_PIN_RESET);
					 GetMoistureAvg_TimeLenght = dane.GetMoistureAvg_TimeLenghtDefault;
					 PumpIsRunnign = false;
				 }

				 if(MoistureGeneral >= dane.StopPumpWhenMoistureHigher && PumpIsRunnign == true)
				 {
					 Sendf("Soil moisture avg: %d is >= %d \r\n", MoistureGeneral, dane.StopPumpWhenMoistureHigher);
					 HAL_GPIO_WritePin(PIN_PB10_Pump_GPIO_Port, PIN_PB10_Pump_Pin, GPIO_PIN_RESET);
					 GetMoistureAvg_TimeLenght = dane.GetMoistureAvg_TimeLenghtDefault;
					 PumpIsRunnign = false;
				 }

			 }


		}

		TimeNOW = HAL_GetTick();

		if(((uint32_t)TimeNOW - GetLightMesurment_FromTime) >= (uint32_t)GetLightMesurment_TimeLenghtDefault)
		{
		    GetLightMesurment_FromTime = TimeNOW;

		    if(DMA_Channel_4_Was_Filled)
		    {
		        DMA_Channel_4_Was_Filled = false;

		        //jasno → niska rezystancja LDR → wysokie napięcie → wysokie ADC (np. 3500)
		        //ciemno → wysoka rezystancja LDR → niskie napięcie → niskie ADC (np. 100)
		        LightSensorPercent = ScaleADC_Light_To_Percentage((uint16_t*)ADC_LightSensor_Dev, Channel_4_BUF_SIZE, 100, 3700);

		        Sendf("Light sensor: %d %% light\r\n", LightSensorPercent);

		        if(LightSensorPercent <= DARK)
		            LightSensor = DARK;
		        else if(LightSensorPercent > DARK && LightSensorPercent < BRIGHT)
		            LightSensor = NIGHT_OR_MORNING;
		        else if(LightSensorPercent > BRIGHT)
		            LightSensor = BRIGHT;

		        memset(txBuffer, 0, sizeof(txBuffer));
		        switch (LightSensor)
		        {
		            case DARK:
		                // Jeśli okno 10h jest aktywne -> DARK nie może wyłączać LED
		                if(LightTimerEnabled == true)
		                {
		                    if(LightIsON == false)
		                    {
		                        LightIsON = true;
		                        HAL_GPIO_WritePin(PIN_PB11_Light_GPIO_Port, PIN_PB11_Light_Pin, GPIO_PIN_SET);
		                        Sendf("Light sensor DARK enable LED (timer active)\r\n");
		                    }
		                }
		                else
		                {
		                    if(LightIsON == true)
		                    {
		                        HAL_GPIO_WritePin(PIN_PB11_Light_GPIO_Port, PIN_PB11_Light_Pin, GPIO_PIN_RESET);
		                        Sendf("Light sensor DARK disable LED\r\n");
		                        LightIsON = false;
		                    }
		                    waitFordark = false;
		                }
		            break;

		            case NIGHT_OR_MORNING:
		                // Start okna 10h tylko przy pierwszym sensownym "porannym" warunku
		                if(LightTimerEnabled == false && waitFordark == false)
		                {
		                    GetLightTime_FromTime = TimeNOW; // od tej pory zliczamy 10H okna świecenia
		                    LightTimerEnabled = true;
		                }

		                if(LightTimerEnabled == true)
		                {
		                    if(LightIsON == false && waitFordark == false)
		                    {
		                        LightIsON = true;
		                        HAL_GPIO_WritePin(PIN_PB11_Light_GPIO_Port, PIN_PB11_Light_Pin, GPIO_PIN_SET);
		                        Sendf("Light sensor NIGHT_OR_MORNING enable LED\r\n");
		                    }
		                }
		            break;

		            case BRIGHT:
		                // BRIGHT wyłącza LED, ale NIE resetuje okna czasowego (okno dalej leci)
		                if(LightIsON == true)
		                {
		                    HAL_GPIO_WritePin(PIN_PB11_Light_GPIO_Port, PIN_PB11_Light_Pin, GPIO_PIN_RESET);
		                    Sendf("Light sensor too BRIGHT disable LED\r\n");
		                    LightIsON = false;
		                }
		            break;

		            default:
		            break;
		        }
		    }

		    // Zamykanie okna 10h (zabezpieczenie na overflow ticków przez rzutowanie na uint32_t)
		    if(((uint32_t)(TimeNOW - GetLightTime_FromTime) >= dane.lightTime) && (LightTimerEnabled == true))
		    {
		        LightTimerEnabled = false;

		        HAL_GPIO_WritePin(PIN_PB11_Light_GPIO_Port, PIN_PB11_Light_Pin, GPIO_PIN_RESET);
		        LightIsON = false; // spójność stanu
		        waitFordark = true;
		        Sendf("Light time elapsed - disable LED\r\n");
		    }
		}

		TimeNOW = HAL_GetTick();

		if(((uint32_t)TimeNOW - MixWater_FromTime) >= MixWater_TimeLenghtDefault)
		{
			MixWater_FromTime = TimeNOW;
			if (WaterSensor1State != GPIO_PIN_SET)
			 if(MoistureGeneral < dane.StopPumpWhenMoistureHigher && PumpIsRunnign == false)
			 {
				 Sendf("Mixing water..\r\n");
				 HAL_GPIO_WritePin(PIN_PB10_Pump_GPIO_Port, PIN_PB10_Pump_Pin, GPIO_PIN_SET);
				 HAL_Delay(5000);
				 HAL_GPIO_WritePin(PIN_PB10_Pump_GPIO_Port, PIN_PB10_Pump_Pin, GPIO_PIN_RESET);
				 HAL_Delay(5000);
				 HAL_GPIO_WritePin(PIN_PB10_Pump_GPIO_Port, PIN_PB10_Pump_Pin, GPIO_PIN_SET);
				 HAL_Delay(5000);
				 HAL_GPIO_WritePin(PIN_PB10_Pump_GPIO_Port, PIN_PB10_Pump_Pin, GPIO_PIN_RESET);
			 }
		}

		if((TimeNOW - LedSecond_FromTime) >= LedSecond)
		{
			LedSecond_FromTime = TimeNOW;
			HAL_GPIO_TogglePin(LED_01_GPIO_Port, LED_01_Pin);
		}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
//void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
//{
//	HAL_ADC_Stop_DMA(&hadc1); // jesli chcial bym wylaczyc timer
//	adc_ready = true;
//}

HAL_StatusTypeDef Flash_WriteStruct(uint32_t address, void *data, uint32_t size)
{
    HAL_StatusTypeDef status;

    if ((address & 0x3u) != 0u)
        return HAL_ERROR;

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK)
        return status;

    flash_clear_all_flags();

    uint32_t *src = (uint32_t*)data;
    uint32_t words = (size + 3) / 4;   // ilość WORD

    for (uint32_t i = 0; i < words; i++)
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                                   address + (i * 4),
                                   src[i]);
        if (status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return status;
        }
    }

    HAL_FLASH_Lock();
    return HAL_OK;
}

void Flash_ReadStruct(uint32_t address, void *data, uint32_t size)
{
    memcpy(data, (void*)address, size);
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM3)
	{
		// HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, 1);  1 jako ON 0 jako OFF
		//HAL_GPIO_TogglePin(LED_01_GPIO_Port, LED_01_Pin);
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        USART_OnByteReceived(rxByte);
        USART_StartReceiveIT();
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) // Sprawdź, czy to Twój UART
    {
    	isTransmissionComplete = true;
    }
}


static bool parse_kv_int(const char *line, const char *key, uint32_t *outVal)
{
    size_t klen = strlen(key);
    if (strncmp(line, key, klen) == 0 && line[klen] == ':') {
        int v = 0;
        if (sscanf(line + klen + 1, "%d", &v) == 1) {
            *outVal = v;
            return true;
        }
    }
    return false;
}



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

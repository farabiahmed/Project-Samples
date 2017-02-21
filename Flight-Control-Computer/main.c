/**
 ******************************************************************************
 * @file    main.c
 * @author  Farabi Ahmed Tarhan
 * @version V1.0.0
 * @date    09-05-2013
 * @brief   Main program body
 ******************************************************************************
 * @attention
 *
 * Coding Standards
 * https://users.ece.cmu.edu/~eno/coding/CCodingStandard.html
 *
 * Documentation
 * http://fnch.users.sourceforge.net/doxygen_c.html
 *
 * Some Useful Articles About Interrupt Handling
 * http://blog.atollic.com/device-driver-development-the-ultimate-guide-for-embedded-system-developers
 *
 * Understanding startup file
 * http://charleskorn.com/2016/04/17/a-deeper-look-at-the-stm32f4-project-template-getting-things-started/
 *
 * Understanding the stack
 * https://www.cs.umd.edu/class/sum2003/cmsc311/Notes/Mips/stack.html
 *
 ******************************************************************************
 */

/*
 * Application Flash / Ram Size
 *
 * text - shows the code and read-only data in your application (in decimal)
 * data - shows the read-write data in your application (in decimal)
 * bss - show the zero initialized ('bss' and 'common') data in your application (in decimal)
 * dec - total of 'text' + 'data' + 'bss' (in decimal)
 * hex - hexidecimal equivalent of 'dec'
 *
 * Typically,
 * the flash consumption of your application will then be text + data
 * the RAM consumption of your application will then be data + bss.
 *
 * Resource: http://www.stf12.org/developers/freerots_ec-linker_script.html
 *
 * STM32F407VGT6
 *
 * Maximum Clock Frequency:	168 MHz
 * Program Memory Size:	1 MB
 * Data RAM Size:	192 kB
 *
 */

/*
 * REMOTE DEBUG
 * 1. Connect RPI from "Remote System Explorer"
 * 2. Then start openocd;
 * 		sudo openocd -f board/stm32f4discovery.cfg
 * 		sudo openocd -f interface/sysfsgpio-raspberrypi.cfg -f target/stm32f4x.cfg
 * 3. Then start debug session by;
 * 		FlightController Remote
 *
 */

/*
 * Flight Management Computer
 *
 * sudo ./FlightManager/FlightManager.elf
 *
 *
 */

//#define MODE_AIRCRAFT
/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "ADSB.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
void Delay(__IO uint32_t nCount);
/* Private functions ---------------------------------------------------------*/

//uint32_t i=0,j=0;
uint8_t FlightPermission = 0;
uint32_t CPULoadCalculator = 0;
int ADIS1640xLedState = 0;
uint16_t ret = 0;
uint8_t HMC5883L_Connected;
uint8_t MPU6050_Connected;
uint8_t currentAttempt=0;
#define MAXATTEMPT 5
//For DJI
CanTxMsg CanTxMessage;

//Navigation
uint8_t NavigationFirstFix=0;

/*
 struct platform_data_s {
 signed char orientation[9];
 };
 static struct platform_data_s gyro_pdata = {
 .orientation = { 1, 0, 0,
 0, 1, 0,
 0, 0, 1}

 };
 */
int main(void) {
	/*
	 At this stage the microcontroller clock setting is already configured,
	 this is done through SystemInit() function which is called from startup
	 file (startup_stm32f4xx.s) before to branch to application main.
	 To reconfigure the default setting of SystemInit() function, refer to
	 system_stm32f4xx.c file
	 */

	// Initialize Peripherals

	Peripherals_Init();

	//Wait Naza (can bus) To Initialize
	WaitMs(1000);

	//Some Delay for Sensor Configuration and Let The Operator to Hear Initial Sound.
	for (int i = 0; i < 20; i++) {
		BUZZER_BLINK;
		WaitMs(20);
	}
	BUZZER_OFF;

	//Some More Delay For Sensors and Let the Operator see the leds on the avionic.
	LEDGS_ON;
	WaitMs(200);
	LEDIMU_ON;
	WaitMs(200);
	LEDRPI_ON;
	WaitMs(200);
	LEDRC_ON;
	WaitMs(200);

	if (ADIS1640x_Initialize() != 0) {
		for (int i = 0; i < 20; i++) {
			BUZZER_BLINK;
			WaitMs(50);
		}
	}

	BUZZER_OFF;

	currentAttempt = 0;
	while(HMC5883L_Connected==0 && currentAttempt<MAXATTEMPT)
	{
		HMC5883L_Connected = HMC5883L_Configuration();

		if(HMC5883L_Connected==0)
			I2C_Configuration();

		currentAttempt++;
	}

	//MPU6050_Connected = MPU6050_WhoIAm();
	//MPU6050_Connected = MPU6050_Configuration();


	//Load Flight Parameters
	FlightPermission = Parameters_Load();

	ADIS1640x_Read_SensorData(&ADIS1640x);

	Kalman_Init(&KalmanRoll,	Config.KalmanRoll.Q1,	Config.KalmanRoll.Q2,	Config.KalmanRoll.R); // 0.0001, 0.0001, 0.9
	Kalman_Init(&KalmanPitch,	Config.KalmanPitch.Q1,	Config.KalmanPitch.Q2,	Config.KalmanPitch.R);

	Kalman_Init(&(KalmanVelocity[0]), Config.KalmanVelocity[0].Q1,Config.KalmanVelocity[0].Q2,Config.KalmanVelocity[0].R);
	Kalman_Init(&(KalmanVelocity[1]), Config.KalmanVelocity[1].Q1,Config.KalmanVelocity[1].Q2,Config.KalmanVelocity[1].R);
	Kalman_Init(&(KalmanVelocity[2]), Config.KalmanVelocity[2].Q1,Config.KalmanVelocity[2].Q2,Config.KalmanVelocity[2].R);

	Kalman_Init(&(KalmanPosition[0]), Config.KalmanPosition[0].Q1,Config.KalmanPosition[0].Q2,Config.KalmanPosition[0].R);
	Kalman_Init(&(KalmanPosition[1]), Config.KalmanPosition[0].Q1,Config.KalmanPosition[0].Q2,Config.KalmanPosition[0].R);
	Kalman_Init(&(KalmanPosition[2]), Config.KalmanPosition[0].Q1,Config.KalmanPosition[0].Q2,Config.KalmanPosition[0].R);
	Kalman_Init(&(KalmanAltitude), Config.KalmanAltitude.Q1,Config.KalmanAltitude.Q2,Config.KalmanAltitude.R);

	//Configure Communication Packets
	TelemetryTx.Port = UART4;
	MissionTx.Port = USART3;
	HILPacketTx.Port = USART3;

	//Initialize The Sensorkit
	XsensMti.Port = USART2;
	SensorKitHIL.HILEnabled = 0;
	//VectorNav.Port= UART4;


	//SensorKit 			= &SensorKitHIL;
	SensorKit 				= &ADIS1640x;
	//SensorKit 		= &XsensMti;
	//SensorKit 		= &MPU6050;
	//SensorKit 		= &DJINaza;
	//SensorKit 		= &VectorNav;
	//SensorKitBackUp	= &ADIS1640x;
	//SensorKitBackUp	= &DJINaza;
	SensorKitBackUp		= &MPU6050;
	//SensorKitBackUp 	= &XsensMti;
	//SensorKitBackUp 	= &VectorNav;

	// Configure Xsense
	Xsens_ChangeMode(&XsensMti, XSENS_MODE_CONFIG);
	WaitMs(200);
	Xsens_Configuration(&XsensMti);
	WaitMs(200);
	Xsens_ChangeMode(&XsensMti, XSENS_MODE_MEASUREMENT);

	//Current and Voltage Reading.
	ADC_SoftwareStartConv(ADC1);

	//Ublox GPS Start
#ifdef HW_VERSION_3V0
	uBlox.Port = USART1;
	USARTx_Configuration(uBlox.Port, 9600, USART_WordLength_8b, USART_StopBits_1, USART_Parity_No);
#else
	uBlox.Port = UART5;
	USARTx_Configuration(uBlox.Port, 38400, USART_WordLength_8b, USART_StopBits_1, USART_Parity_No);
#endif

	uBlox_Init(&uBlox);
	USARTx_Configuration(uBlox.Port, 230400, USART_WordLength_8b,
	USART_StopBits_1, USART_Parity_No);
	uBlox_Configuration(&uBlox);

	LEDIMU_OFF;
	LEDRC_OFF;
	LEDRPI_OFF;
	LEDGS_OFF;

	CanTxMessage.StdId = 0x0108;
	CanTxMessage.ExtId = 0x00;
	CanTxMessage.RTR = CAN_RTR_DATA;
	CanTxMessage.IDE = CAN_ID_STD;
	CanTxMessage.DLC = 8;

	CanTxMessage.Data[0] = 0x55;
	CanTxMessage.Data[1] = 0xAA;
	CanTxMessage.Data[2] = 0x55;
	CanTxMessage.Data[3] = 0xAA;
	CanTxMessage.Data[4] = 0x07;
	CanTxMessage.Data[5] = 0x10;
	CanTxMessage.Data[6] = 0x00;
	CanTxMessage.Data[7] = 0x00;

	uint8_t TransmitMailbox = CAN_Transmit(CAN1, &CanTxMessage);

	//while(CAN_TransmitStatus(CAN1,TransmitMailbox)!=CANTXOK);//1Hz ile yollayýnca Kilitlendi
	WaitMs(10);

	CanTxMessage.Data[0] = 0x66;
	CanTxMessage.Data[1] = 0xCC;
	CanTxMessage.Data[2] = 0x66;
	CanTxMessage.Data[3] = 0xCC;
	CanTxMessage.Data[4] = 0x00;
	CanTxMessage.Data[5] = 0x00;
	CanTxMessage.Data[6] = 0x00;
	CanTxMessage.Data[7] = 0x00;

	CAN_Transmit(CAN1, &CanTxMessage);

	while (1) {
		CPULoadCalculator++;

		if (!Counter1sec) {
			Counter1sec = 1000;
			UpTime++;

			CalCanBusExpander_GetBattery();


			ADIS1640x_Set_Output(1, ADIS1640xLedState);
			if (ADIS1640xLedState == 0)
				ADIS1640xLedState = 1;
			else
				ADIS1640xLedState = 0;

			CpuUsage = 100 - (uint8_t) (CPULoadCalculator * 100 / 8809227);
			if (CpuUsage > 100)
				CpuUsage = 100;
			CPULoadCalculator = 0;

			uBlox.PacketRate = uBlox.PacketCounter;
			uBlox.PacketCounter = 0;

			ControlSticks.PacketRate = ControlSticks.PacketCounter;
			ControlSticks.PacketCounter = 0;


			ControlSticksSlave.PacketRate = ControlSticksSlave.PacketCounter;
			ControlSticksSlave.PacketCounter = 0;

			ControlSticks.BadPacketRate = ControlSticks.BadPacketCounter;
			ControlSticks.BadPacketCounter = 0;

			ControlSticksSlave.BadPacketRate =
					ControlSticksSlave.BadPacketCounter;
			ControlSticksSlave.BadPacketCounter = 0;

			MissionRx.Rate = MissionRx.Counter;
			MissionRx.Counter = 0;

			if (ControlSticks.BadPacketRate > 2)
				ControlSticks.Connected = 1;
			else
				ControlSticks.Connected = 0;

			ADCRate = ADCCounter;
			ADCCounter = 0;

			HPA200TempRate = HPA200TempCounter;
			HPA200TempCounter = 0;

			HPA200PressRate = HPA200PressCounter;
			HPA200PressCounter = 0;

			SensorKit->PacketRate = SensorKit->PacketCounter;
			SensorKit->PacketCounter = 0;

			SensorKit->IMUPacketRate = SensorKit->IMUPacketCounter;
			SensorKit->IMUPacketCounter = 0;

			SensorKit->INSPacketRate = SensorKit->INSPacketCounter;
			SensorKit->INSPacketCounter = 0;

			//(SensorKit->IMUPacketRate > 10 && SensorKit->INSPacketRate > 10) -> Xsens (xsens doesn't give some information about accuracy and satellites that is connected.)
			// (SensorKit->Satellites>5 && SensorKit->HAccuracy<25) -> CalINS
			if ( (SensorKit->IMUPacketRate > 10 && SensorKit->INSPacketRate > 10) || (SensorKit->Satellites>3 && SensorKit->HAccuracy<25) )
			{
				SensorKit->IsNavigationValid = 1;

				//Initialize Home Location
				if(Home.Position.Lat==0 || Home.Position.Lon==0)
				{
					Home.Position.Lat 		= GPSDevice.Latitude;
					Home.Position.Lon 		= GPSDevice.Longitude;
					Home.Position.Altitude 	= GPSDevice.MeanSeaLevel;
				}
			}
			else
			{
				SensorKit->IsNavigationValid = 0;
				CounterHomeAcquire = 5000;
			}

			CanPacketRate = CanPacketCounter;
			CanPacketCounter = 0;

			//Reset DMA
			if (SensorKit->PacketRate == 0) {
				USART_ITConfig(SensorKit->Port, USART_IT_RXNE, ENABLE);
				USART_DMACmd(SensorKit->Port, USART_DMAReq_Rx, DISABLE);
			}

			SensorKitBackUp->IMUPacketRate = SensorKitBackUp->IMUPacketCounter;
			SensorKitBackUp->IMUPacketCounter = 0;

			SensorKitBackUp->PacketRate = SensorKitBackUp->PacketCounter;
			SensorKitBackUp->PacketCounter = 0;

			SensorKitBackUp->INSPacketRate = SensorKitBackUp->INSPacketCounter;
			SensorKitBackUp->INSPacketCounter = 0;

			//Send ADS-B Packet To GroundStation and Other Vehicles.
			//GroundStation_SendRawADSB(&TelemetryTx);

			PortQueue.Rate = PortQueue.Counter;
			PortQueue.Counter = 0;

			if (ControlSticks.PacketRate == 0) {
				Sticks_Init(&ControlSticks);
			}
			if (ControlSticksSlave.PacketRate == 0) {
				Sticks_Init(&ControlSticksSlave);
			}

		}

		if (!CounterWarnings) {

			CounterWarnings = 50; //20Hz

			//if (SensorKit->PacketRate > 75)
			if(HMC5883L_Connected)
				LEDMAG_BLINK;
			else
				LEDMAG_OFF;

			if (ControlSticks.PacketRate > 25)
			{
				LEDRC_BLINK;

				//Reset Fail Safe Counter in order to avoid entering fail situation
				FailSafe_TimeOut_Counter[FAILSAFE_ID_REMOTE] = 0;
			}
			else
				LEDRC_OFF;

			if (MissionRx.Rate > 1)
				LEDRPI_BLINK;
			else
				LEDRPI_OFF;

			if(uBlox.PacketCounter>3)
				LEDGPS_BLINK;
			else
				LEDGPS_OFF;
		}

		if (ControlSticks.DataReady) {
			ControlSticks.DataReady = 0;
			if (ParseControlSticks(&ControlSticks))
			{
				ControlSticks.PacketCounter++;
			}
			else
			{
				ControlSticks.BadPacketCounter++;
				ControlSticks.BadPacketTotal++;
			}
		}
		if (ControlSticksSlave.DataReady) {
			ControlSticksSlave.DataReady = 0;
			if (ParseControlSticks(&ControlSticksSlave))
				ControlSticksSlave.PacketCounter++;
			else
			{
				ControlSticksSlave.BadPacketCounter++;
				ControlSticksSlave.BadPacketTotal++;
			}
		}

		if (ADCReady) {
			ADCReady = 0;
			ADCCounter++;

			/*
			for (int i = 0; i < 4; i++) {
				ADCVoltages[i] = (float) (ADCFiltered[i]); //(float)(ADCFiltered[i])*0.000732421875;
			}
			BatteryVoltage = ADCFiltered[2] * 0.00619 + 0.0636; //V
			BatteryCurrent = ADCFiltered[0] * 0.08238 - 30.116; //A

			if (BatteryCurrent < 0.1)
				BatteryCurrent = 0.1;

			if (ADCRate > 0)
				BatteryConsumption += BatteryCurrent * 1000 / (3600 * ADCRate); //mA
			*/
		}
		if (uBlox.DataReady) {
			uBlox.DataReady = 0;
			ParseUBXSentence(&GPSDevice, uBlox.Buffer);

			uBlox.PacketCounter++;

			if(ADIS1640x.IsNavigationValid)
			{
				if(NavigationFirstFix==0)
				{
					NavigationFirstFix=1;

					//KalmanPosition[2].x_bias = GPSDevice.MeanSeaLevel - SensorKit->BaroAltitude;
					//KalmanPosition[2].x_pos = GPSDevice.MeanSeaLevel;
				}

				//Convert Lat/Lon to Meter
				lla2nwu(&(GPSDevice.DistanceNorth), &(GPSDevice.DistanceWest), &(GPSDevice.DistanceUp),
						Home.Position.Lat, Home.Position.Lon,0,
						GPSDevice.Latitude, GPSDevice.Longitude, GPSDevice.MeanSeaLevel);

				//Kalman Correction for Velocities
				Kalman_Update(&(KalmanVelocity[0]),	GPSDevice.VelocityNorth);
				Kalman_Update(&(KalmanVelocity[1]),	-GPSDevice.VelocityEast);
				Kalman_Update(&(KalmanVelocity[2]),	-GPSDevice.VelocityDown);

				//Kalman Correction for Positions
				Kalman_Update(&(KalmanAltitude),	GPSDevice.MeanSeaLevel);
				Kalman_Update(&(KalmanPosition[0]),	GPSDevice.DistanceNorth);
				Kalman_Update(&(KalmanPosition[1]), GPSDevice.DistanceWest);
				//Kalman_Update(&(KalmanPosition[2]),	GPSDevice.MeanSeaLevel);
			}

			ADIS1640x.Satellites = GPSDevice.Satellites;
			ADIS1640x.HAccuracy = GPSDevice.HorizontalAccuracy;
		}

		if (Garmin15H.DataReady) {
			Garmin15H.DataReady = 0;
			memcpy(Garmin15H.ParseBuffer, Garmin15H.Buffer,
					Garmin15H.RxCounter);
			memset(Garmin15H.Buffer, 0, 256);

			ParseNMEASentence(&Garmin15H, Garmin15H.ParseBuffer);
		}

		if (VectorNav.DataReady) {
			VectorNav.DataReady = 0;
			VectorNav_Parse(&VectorNav, VectorNav.Buffer);
		}

		if (XsensMti.DataReady) {
			XsensMti.DataReady = 0;

			if (ParseXsensMtiSentence(&XsensMti)) {
				XsensMti.PacketCounter++;
			}

		}

		if (TelemetryRx.DataReady) {
			TelemetryRx.DataReady = 0;
			GroundStation_Parse(&TelemetryRx, 1);
		}

		if (MissionRx.DataReady) {
			MissionRx.DataReady = 0;
			GroundStation_Parse(&MissionRx, 1);
		}

		if (!CounterGroundStation) {
			CounterGroundStation = 25; //40 Hz

			//Read HMC5883L Raw Values
			HMC5883L_Connected = HMC5883L_Read(&ADIS1640x);

			//If reading fails, then restart the I2C
			if(HMC5883L_Connected==0)
				I2C_Configuration();

			//Barometer Altitudue Update
			Kalman_Predict_Altitude(&(KalmanAltitude), ADIS1640x.BaroAltitude, dT);   // Kalman predict
			Kalman_Update(&(KalmanPosition[2]),	KalmanAltitude.x_pos);


			//If ACK is requested send it.
			if (MissionRx.ACK)
			{
				GroundStation_SendOKData(&MissionTx);
				MissionRx.ACK = 0;

				//If not flying save parameters if needed.
				if (SaveConfiguration && ControlSticks.PwmDutyCycle[2] < 1200) {
					SaveConfiguration = 0;
					BUZZER_ON;
					Parameters_Save();
					BUZZER_OFF;
				}
			}
			else if(ReSendMissionSteps==1)
			{
				ReSendMissionSteps=0;
				Send_FCPRM(PARAMETERTYPE_MISSION,&MissionTx);
			}
			else
			{
				static uint8_t Counter1Hz = 0;
				static uint8_t Counter5Hz = 0;
				static uint8_t Counter20Hz = 0;

				Counter1Hz++;
				Counter5Hz++;
				Counter20Hz++;

				if (Counter1Hz >= (1000 / CounterGroundStation))
				{
					Counter1Hz = 0;

					//Send Data to LowRateRadio
					Send_FGSTA(&TelemetryTx,SensorKit);

					//GroundStation_SendADSBInfo(&MissionTx);
				}
				else if (Counter5Hz >= (200 / CounterGroundStation))
				{
					Counter5Hz = 0;

					//Send Data to HighRateRadio
					Send_FGSTA(&MissionTx,SensorKit);
					//Send_FGINS(&MissionTx);

					//Send Data to LowRateRadio
					Send_FGRCS(&TelemetryTx, SensorKit);
				}
				else if (Counter20Hz >= (50 / CounterGroundStation))
				{
					Counter20Hz = 0;

					//Send Data to HighRateRadio
					Send_FGRCS(&MissionTx, SensorKit);

					//SendALL(&MissionTx,SensorKit);
				}

				//If HIL Enabled, send HIL Packet to Simulator
				if (SensorKit->HILEnabled) {
					Send_HIL_Control(&HILPacketTx, ControlSignals, 8);
				}
			}

		}
		if (!CounterLogSender) {
			CounterLogSender = 10;

			if (ControlSticks.PwmDutyCycle[5] > 1500) {
				Send_FLLOG(&MissionTx);
				//Send_FGDSN(&MissionTx);
			}
		}

		if (!CounterStateEstimation) {
			CounterStateEstimation = 1;
			Toc(&(TimeSpan[4]));
			Tic(&(TimeSpan[4]));

			ADIS1640x_Read_SensorData(&ADIS1640x);

			//MPU6050_Connected = MPU6050_Read(&MPU6050);

			Attitude_Transform_pqr2eulerdot(&ADIS1640x);

			Attitude_ComplimentaryFilter(&ADIS1640x,0.001);
		}

		if (!CounterControl) {

			CounterControl = dT * 1000;
			LoopCounter++;

			MEASPIN_BLINK;

			Tic(&(TimeSpan[1]));

			//For Autonomous TakeOff
			if(TimeOutMotorWarmUp==0 && Actuator.Armed==1)
			{
				ControllerPositionRateZ.Integral = (1500-900)/(ControllerPositionRateZ.I);
				TimeOutMotorWarmUp=-1;
			}

			if (ControllerSettlingTimeOut)
			{
				Controller_Init(&ControllerPositionX);
				Controller_Init(&ControllerPositionY);
				Controller_Init(&ControllerPositionZ);

				Controller_Init(&ControllerPositionRateX);
				Controller_Init(&ControllerPositionRateY);

				Controller_Init(&ControllerPitch);
				Controller_Init(&ControllerRoll);
				Controller_Init(&ControllerYaw);

				Controller_Init(&ControllerPitchRate);
				Controller_Init(&ControllerRollRate);
				Controller_Init(&ControllerYawRate);
			}

			/*
			 * INS Loops
			 */
			Tic(&(TimeSpan[5]));

			//Attitude_KalmanFilter(&ADIS1640x);
			Attitude_Heading_Estimation(&ADIS1640x);
			//Attitude_AccelerometerAngle(&ADIS1640x);
			//Attitude_ComplimentaryFilter(&XsensMti,0.01);
			//Attitude_ComplimentaryFilter(&ADIS1640x,0.01);

			//Magnetometer Hard&Soft Iron Calibration
			Attitude_Mag_Calibration(&ADIS1640x);

			//Magnetometer Tilt Compensation
			Attitude_Heading_TiltCompensated(&ADIS1640x);

			//Body To NWU Acceleration Rotation
			Attitude_TrueAcceleration(&ADIS1640x);

			//Pressure to Altitude Conversation
			ADIS1640x.BaroAltitude_1 = ADIS1640x.BaroAltitude;
			ADIS1640x.BaroAltitude = Attitude_Altitude_FromPressure(ADIS1640x.BaroPressure, ADIS1640x.Temperature);
			ADIS1640x.BaroSpeed = ADIS1640x.BaroSpeed * 0.99+ ((ADIS1640x.BaroAltitude - ADIS1640x.BaroAltitude_1)/ 0.001) * 0.01;

			//Integrate Inertial Navigation System if GPS Valid
			if(ADIS1640x.IsNavigationValid)
			{
				if(isnan(KalmanVelocity[0].x_pos)) KalmanVelocity[0].x_pos=0;
				if(isnan(KalmanVelocity[1].x_pos)) KalmanVelocity[1].x_pos=0;
				if(isnan(KalmanVelocity[2].x_pos)) KalmanVelocity[2].x_pos=0;

				if(isnan(KalmanPosition[0].x_pos)) KalmanPosition[0].x_pos=0;
				if(isnan(KalmanPosition[1].x_pos)) KalmanPosition[1].x_pos=0;
				if(isnan(KalmanPosition[2].x_pos)) KalmanPosition[2].x_pos=0;

				//Acceleration to Velocity
				Kalman_Predict_Integrate(&(KalmanVelocity[0]), ADIS1640x.TrueInertial[0], dT);    // Kalman predict
				Kalman_Predict_Integrate(&(KalmanVelocity[1]), ADIS1640x.TrueInertial[1], dT);    // Kalman predict
				Kalman_Predict_Integrate(&(KalmanVelocity[2]), ADIS1640x.TrueInertial[2], dT);    // Kalman predict

				// Transfer Kalman Output To Sensorkit Structure
				ADIS1640x.Velocity[0] = KalmanVelocity[0].x_pos;
				ADIS1640x.Velocity[1] = KalmanVelocity[1].x_pos;
				ADIS1640x.Velocity[2] = KalmanVelocity[2].x_pos;

				//Velocity To Position
				Kalman_Predict_Integrate(&(KalmanPosition[0]), ADIS1640x.Velocity[0], dT);    // Kalman predict
				Kalman_Predict_Integrate(&(KalmanPosition[1]), ADIS1640x.Velocity[1], dT);    // Kalman predict
				Kalman_Predict_Integrate(&(KalmanPosition[2]), ADIS1640x.Velocity[2], dT);   // Kalman predict

				//Convert Lat/Lon to Meter
				double lat,lon,alti;
				nwu2lla(&lat,&lon,&alti,Home.Position.Lat,Home.Position.Lon, 0, KalmanPosition[0].x_pos, KalmanPosition[1].x_pos, KalmanPosition[2].x_pos);

				// Transfer Kalman Output To Sensorkit Structure
				ADIS1640x.Position[0] = (float)lat;
				ADIS1640x.Position[1] = (float)lon;
				ADIS1640x.Position[2] = (float)alti;
			}

			Toc(&(TimeSpan[5]));

			/*
			 * Fail Safe Procedures
			 * It alters the controls if needed.
			 */
			FailSafe_Run();

			Tic(&(TimeSpan[2]));
			/*
			 * Control Loops
			 */
			FlightControl(SensorKit);
			Toc(&(TimeSpan[2]));

			Tic(&(TimeSpan[3]));
			Transformation_ToActuators();
			Toc(&(TimeSpan[3]));

			//Send_FGDSN(&MissionTx); //TODO: Remove after logging.

			/*
			 * Check If Motors needs close down or open.
			 */

			Actuator_Arm_Check(&Actuator, &ControlSticks,
					((SensorKit->IMUPacketRate > 10)
							&& (ControlSticks.PacketRate > 5)
							&& (FlightPermission)), FlightMode);

#ifdef MODE_AIRCRAFT
			Actuator.Armed = 1;
#endif

			if (Actuator.Armed) {
				Actuator_Refresh(&Actuator);
			} else {
				Actuator_OFF(&Actuator);

				ControlSticks.CommandYawHold = SensorKit->Angle[2];

				Controller_Init(&ControllerPitch);
				Controller_Init(&ControllerRoll);
				Controller_Init(&ControllerYaw);

				Controller_Init(&ControllerPitchRate);
				Controller_Init(&ControllerRollRate);
				Controller_Init(&ControllerYawRate);
			}

			//DMA Control Reset Uart1
			if (!DMATimeOut[1]) {
				USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
				USART_DMACmd(USART1, USART_DMAReq_Rx, DISABLE);
				DMATimeOut[1] = 5000;
			}
			//DMA Control Reset Uart2
			if (!DMATimeOut[2]) {
				USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
				USART_DMACmd(USART2, USART_DMAReq_Rx, DISABLE);
				DMATimeOut[2] = 5000;
			}
			//DMA Control Reset Uart3
			if (!DMATimeOut[3]) {
				USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
				USART_DMACmd(USART3, USART_DMAReq_Rx, DISABLE);
				DMATimeOut[3] = 5000;
			}
			//DMA Control Reset Uart4
			if (!DMATimeOut[4]) {
				USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);
				USART_DMACmd(UART4, USART_DMAReq_Rx, DISABLE);
				DMATimeOut[4] = 5000;
			}

			//Calculation Time
			Toc(&(TimeSpan[1]));

			//Control Period
			Toc(&(TimeSpan[0]));
			Tic(&(TimeSpan[0]));
		}

	}
}

void Delay(__IO uint32_t nCount) {
	while (nCount--) {
	}
}

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

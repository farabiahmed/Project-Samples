/*
 * Flight Management Computer Main Routine
 * main.cpp
 *
 *  Created on: 28 Eyl 2013
 *      Author: Farabi Ahmed Tarhan
 */

/*
 * sudo killall -v FlightManager.elf  
 * g++ -std=c++0x -o FlightManager main.cpp drivers/UDP.cpp drivers/GPIOClass.cpp drivers/SocketServer.cpp drivers/Uart.cpp Frameworks/HILService.cpp Frameworks/LiveData.cpp Frameworks/ProtocolParser.cpp Frameworks/Telemetry.cpp Frameworks/Mission.cpp -lpthread
 */
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <pthread.h>
#include "drivers/Uart.h"
#include "Frameworks/HILService.h"
#include "Frameworks/ProtocolParser.h"
#include "Frameworks/LiveData.hpp"
#include "Frameworks/Telemetry.h"
#include "Frameworks/Mission.h"
#include "drivers/SocketServer.h"
#include "drivers/UDP.h"
#include "drivers/GPIOClass.h"

#pragma GCC diagnostic ignored "-Wwrite-strings" //Suppress warning: deprecated conversion from string constant to �char*�


using namespace std;

int udpPort = 5001;
int tcpPort = 5000;
int missionPort = 5003;
int udphilreceiverport=49005;
int udphiltransmitterport=49000;

/*
 * Class Instances
 */
Uart SerialPort;
Socket Server;
Socket MissionServer;
UDP UDPSocket;

UDP UDPHILSender;
UDP UDPHILReceiver;

Telemetry SerialPacketRx;
Telemetry SerialPacketTx;

Telemetry TCPPacketRx;
Telemetry TCPPacketTx;

Telemetry MissionPacketRx;
Telemetry MissionPacketTx;

Telemetry HILPacketToSim;
Telemetry HILPacketTx;

Telemetry UDPPacketTx;
Telemetry UDPPacketRx;

/*
 * Prototypes
 */
void* Timer(void *ptr);
void* UartReceiver(void *ptr);
void* UartSender(void *ptr);
void* ServerReceiver(void *ptr);
void* ServerSender(void *ptr);
void* ServerListener(void *ptr);
void* MissionReceiver(void *ptr);
void* MissionSender(void *ptr);
void* MissionListener(void *ptr);
void* UDPReceiver(void *ptr);
void* UDPSender(void *ptr);
void* UDPHILReceiverRoutine(void *ptr);
void* UDPHILSenderRoutine(void *ptr);
void* ApplicationRoutine(void *ptr);

bool GetStringFromFile(char *ffile, char *value);

/*
 * Globals
 */
int GroundStationResponse=RESPONSE_NONE;

/*
 * Packet Counters
 */
int SerialPacketRate=0;
int SerialPacketCounter=0;
int NetworkPacketRate=0;
int NetworkPacketCounter=0;
int MissionPacketRate=0;
int MissionPacketCounter=0;
int HILStatePacketRate=0;
int HILStatePacketCounter=0;
int HILControlPacketRate=0;
int HILControlPacketCounter=0;
int UDPPacketCounter=0;
int UDPPacketRate=0;
int FGALLRate=0;
int FGALLCounter=0;
int HILCNRate=0;
int HILCNCounter=0;

//Log
FILE* pFile;
int LogPermission=0;
char cmd[1024];
int filenumber;

//GPIO Variables
GPIOClass* LedPin;
char LedStatus[1];

int main(void)
{
	cout << "###############################" 	<< endl << flush;
	cout << "Mission Controller Starting!" 		<< endl << flush;
	cout << "###############################" 	<< endl << flush;


	/*
	 * Thread Instances
	 */
	pthread_t threadTimer;
	pthread_t threadSerialReceiver;
	pthread_t threadServerReceiver;
	pthread_t threadServerSender;
	pthread_t threadServerListener;
	pthread_t threadSerialSender;
	pthread_t threadUDPReceiver;
	pthread_t threadUDPSender;
	pthread_t threadMissionTCPReceiver;
	pthread_t threadMissionTCPSender;
	pthread_t threadMissionTCPListener;
	pthread_t thHILReceiver;
	pthread_t thHILTransmitter;
	pthread_t thApplication;

	cout.precision(12);
	
	/*
	 * GPI Init
	 */
	LedPin = new GPIOClass("18"); //create new GPIO object to be attached to  GPIO4
	LedPin->export_gpio(); //export
    cout << " GPIO18 pin exported" << endl;
    LedPin->setdir_gpio("out"); //output

	/*
	 * SerialPort Initialization
	 */
	SerialPort.Start("/dev/ttyAMA0",B921600);

	/*
	 * Server Initialization
	 */

	MissionServer.CreateServer(missionPort);

	Server.CreateServer(tcpPort);

	UDPSocket.CreateBroadcastServer(udpPort);

	UDPHILReceiver.Listen(udphilreceiverport);
	UDPHILSender.Listen(udphiltransmitterport);

	cout << "UDPPort" 	<< udpPort << endl << flush;
	cout << "TCPPort" 	<< tcpPort << endl << flush;


	//Start Log File:
	sprintf(cmd,"ls -l /home/pi/FlightManager/Log/*.log | wc -l > /home/pi/FlightManager/Log/lognumber");
	if(system(cmd)!=0)
	{
		cout << "Log File Name Identification Error..." 	<< endl << flush;
	}
	else
	{
		if(GetStringFromFile("/home/pi/FlightManager/Log/lognumber", cmd))
		{
			filenumber = atoi(cmd)+1;
			sprintf(cmd,"/home/pi/FlightManager/Log/%d.log",filenumber);

			pFile = fopen(cmd, "wb"); //write binary mode

			if(pFile!=NULL)
			{
				LogPermission=1;
				cout << "Log File OK: "<< filenumber << endl << flush;
				fclose(pFile);
			}
			else
			{
				cout << "Log File Open Error..." 	<< endl << flush;
			}
		}
		else
		{
			cout << "Log File Name Error..." 	<< endl << flush;
		}
	}

	LiveData.ACK = 0;

	/*
	 * Threads Starting...
	 */
	//SerialPort
	pthread_create(&threadTimer, NULL, Timer, NULL);
	pthread_create(&threadSerialReceiver, NULL, UartReceiver, NULL);
	pthread_create(&threadSerialSender, NULL, UartSender, NULL);

	//Ground Station TCP
	pthread_create(&threadServerReceiver, NULL, ServerReceiver, NULL);
	pthread_create(&threadServerSender, NULL, ServerSender, NULL);
	pthread_create(&threadServerListener, NULL, ServerListener, NULL);

	//Ground Station UDP
	pthread_create(&threadUDPReceiver, NULL, UDPReceiver, NULL);
	pthread_create(&threadUDPSender, NULL, UDPSender, NULL);

	//Mission Processor
	pthread_create(&threadMissionTCPReceiver, NULL, MissionReceiver, NULL);
	pthread_create(&threadMissionTCPSender, NULL, MissionSender, NULL);
	pthread_create(&threadMissionTCPListener, NULL, MissionListener, NULL);

	//HIL UDP
	pthread_create(&thHILReceiver, NULL, UDPHILReceiverRoutine, NULL);
	pthread_create(&thHILTransmitter, NULL, UDPHILSenderRoutine, NULL);

	//Application
	pthread_create(&thApplication, NULL, ApplicationRoutine, NULL);

	MissionPacketTx.RxBuffer[0] = '$';
	MissionPacketTx.RxBuffer[1] = 0;
	MissionPacketTx.RxBuffer[2] = 10;
	MissionPacketTx.RxBuffer[3] = 'G';
	MissionPacketTx.RxBuffer[4] = 'S';
	MissionPacketTx.RxBuffer[5] = 'C';
	MissionPacketTx.RxBuffer[6] = 'P';
	MissionPacketTx.RxBuffer[7] = 'T';
	MissionPacketTx.RxBuffer[8] = 12;
	MissionPacketTx.RxBuffer[9] = 248;
	MissionPacketTx.RxBuffer[10] = 77;
	MissionPacketTx.RxBuffer[11] = 5;
	MissionPacketTx.RxBuffer[12] = 215;

	MissionPacketTx.Length = 10;



	// Wait for the thread to finish, then exit
	pthread_join(threadTimer, NULL);


	Server.CloseServer();


	cout << "###############################" 	<< endl << flush;
	cout << "Mission Controller Closing!" 		<< endl << flush;
	cout << "###############################" 	<< endl << flush;
	sleep(3);


}


void* Timer(void *ptr)
{
	while (1)
	{
		SerialPacketRate = SerialPacketCounter;
		SerialPacketCounter=0;

		NetworkPacketRate=NetworkPacketCounter;
		NetworkPacketCounter=0;

		UDPPacketRate = UDPPacketCounter;
		UDPPacketCounter=0;

		MissionPacketRate = MissionPacketCounter;
		MissionPacketCounter=0;

		HILStatePacketRate = HILStatePacketCounter;
		HILStatePacketCounter=0;

		HILControlPacketRate = HILControlPacketCounter;
		HILControlPacketCounter=0;


		FGALLRate = FGALLCounter;
		FGALLCounter=0;

		HILCNRate=HILCNCounter;
		HILCNCounter=0;

		printf("[Timer 1Hz] PacketRates TCPRx:%3d <-> UDPRx:%3d <-> SerRx:%3d <-> MisRx:%3d <-> HIL:%3d\n",NetworkPacketRate,UDPPacketRate,SerialPacketRate,MissionPacketRate,HILStatePacketRate);
		//printf("FGALL: %d HILCN: %d\n",FGALLRate,HILCNRate);
		//printf("Ele: %3.2f Ail:%3.2f Vect:%3.2f Rud:%3.2f \n",Vehicle[0].ControlSignals[0],Vehicle[0].ControlSignals[1],Vehicle[0].ControlSignals[2],Vehicle[0].ControlSignals[3]);

		/*
		for(int i=0;i<8;i++)
		{
			printf("%2.2f ",Vehicle[0].ControlSignals[i]);
		}
		printf("\n");
		*/
		
		//printf("Lat/Lon/Alti: %lf %lf %lf\n",LiveData.Position[0],LiveData.Position[1],LiveData.Position[2]);

		//Blink Led
		if(!strcmp(LedStatus,"1"))
		{
			strcpy(LedStatus,"0");
			LedPin->setval_gpio("1");
		}
		else
		{
			strcpy(LedStatus,"1");
			LedPin->setval_gpio("0");
		}

		sleep(1);
	}

	return 0;
}

//
//APPLICATION THREAD
//
void* ApplicationRoutine(void *ptr)
{
	char message[50][1024];
	char response[1024];
	char buffer[1024];
	uint32_t taskCounter=0;
	int PathLength = 0;
	double Path[128][3];
	
	while (1)
	{
		if(LiveData.ApplicationType == APPLICATION_NORMAL)
		{
			sleep(1);
		}
		else if(LiveData.ApplicationType == APPLICATION_WEBCONTROL)
		{
			static int CallId=-1;
			
			//printf("Application: WebCheck...\n");
			sleep(1);
						
			sprintf(cmd,"sudo curl --silent -m 10  'http://160.75.17.220/drone/services/service_dronepoll.php?apikey=70y4X41Z5f0dKJxCFra88x6527u6g5gS&type=setstatus&uavid=%s&message=%f,%f,%f,%f'  > /dev/null ","19",LiveData.Position[0],LiveData.Position[1],LiveData.Position[2],LiveData.Angle[2]);
			system(cmd);
		
			if(GroundStationResponse==RESPONSE_AUTONOMOUSPATH_ACCEPT)
			{
				//Send Accept Message To Server
				cout<<"Send Accept Message To Server"<<endl;
				sprintf(cmd,"sudo curl --silent -m 10  'http://160.75.17.220/drone/services/service_dronepoll.php?apikey=70y4X41Z5f0dKJxCFra88x6527u6g5gS&type=setcall&callid=%d&newstat=accept'  > /dev/null ",CallId);
				system(cmd);
				GroundStationResponse = RESPONSE_NONE;
				
				//Send New Waypoints To Autopilot
				if(PathLength>0)
				{
					Missions.clear();
					for(int i=0; i<PathLength; i++)
					{
						Struct_MissionStep step;
						
						if(i==0)
							step.Type = MISSIONTYPE_VTOL_TAKEOFF;
						else
							step.Type = MISSIONTYPE_VTOL_WAYPOINT;
						
						step.Latitude = Path[i][0];
						step.Longitude = Path[i][1];
						step.Altitude = Path[i][2];
						step.WaitTime=0;
						step.Accuracy=5;
						step.MaxSpeedVertical=2;
						step.MaxSpeedHorizontal=5;
						step.LandingHeight=0;		

						Missions.push_back(step);
					}					
					
					cout<<"Missions Size:"<<Missions.size()<<endl;
					SerialPacketTx.PayloadLength = Mission_PackForAutopilot(SerialPacketTx.Payload,Missions);		
					cout<<"Payload Length:"<<SerialPacketTx.PayloadLength<<endl;
					SerialPacketTx.CreatePacket((uint8_t*)"PCPRM",SerialPacketTx.Payload,SerialPacketTx.PayloadLength);					
				}
				
				cout<<"[MISSIONS...]"<<endl;
				for(int i=0;i<Missions.size();i++)
				{
					cout<<i<<":"<<Missions[i].Latitude<<"-"<<Missions[i].Longitude<<endl;
				}
			}
			else if(GroundStationResponse==RESPONSE_AUTONOMOUSPATH_DISCARD)
			{
				//Send Discard Message To Server
				cout<<"Send Discard Message To Server"<<endl;
				sprintf(cmd,"sudo curl --silent -m 10  'http://160.75.17.220/drone/services/service_dronepoll.php?apikey=70y4X41Z5f0dKJxCFra88x6527u6g5gS&type=setcall&callid=%d&newstat=discard' > /dev/null",CallId);
				system(cmd);
				GroundStationResponse = RESPONSE_NONE;
			}
			
			if((taskCounter%10)==0)
			{
				cout<<"Checking ITU Drone Servers..."<<endl;

				system("sudo curl --silent -m 10  'http://160.75.17.220/drone/services/service_dronepoll.php?apikey=70y4X41Z5f0dKJxCFra88x6527u6g5gS&type=getlast' > /home/pi/FlightManager/response");

				if(GetStringFromFile("/home/pi/FlightManager/response", response))
				{
					cout<<"Response: " << response <<endl;

					int i=0;
					char* tokens = strtok (response,",");
					while (tokens != NULL)
					{
						memset(message[i], '\0', sizeof(message[i]));
						strcpy(message[i++],tokens);
						tokens = strtok (NULL, ",");
					}

					if(!strcmp(message[0],"getlast"))
					{			
						CallId = atoi(message[1]);
						double TargetLat = atof(message[4]);
						double TargetLon = atof(message[5]);
						
						cout<<"Target Location: (" << message[4] << "," << message[5] << ")" << endl;
						cout<<"Current Location: (" << LiveData.Position[0] << "," << LiveData.Position[1] << ")" << endl;
						
						cout<<"Running Path Finder..."<<endl;
						sprintf(response,"cd /home/pi/PathFinder/ && /home/pi/PathFinder/PathFinder 1 %.10f %.10f %.10f %.10f", LiveData.Position[0], LiveData.Position[1], TargetLat, TargetLon); //exec terminates shell in order to avoid memory usage
						cout<<"PathFinder Script Call: " << response << endl;
						system(response);
						
						if(GetStringFromFile("/home/pi/PathFinder/GeneratedPath.dat", response))
						{
							system("sudo rm /home/pi/PathFinder/GeneratedPath.dat");
							cout<<"Path Finder Response: " << response << endl;
							
							//Parse PathFinder Output
							i=0;
							char* tokens = strtok (response,",");
							while (tokens != NULL)
							{
								memset(message[i], '\0', sizeof(message[i]));
								strcpy(message[i++],tokens);
								tokens = strtok (NULL, ",");
							}
							
							if(!strcmp(message[0],"Success"))
							{
								//If path finder successfully finished its mission.							
								PathLength = atoi(message[1]);								
								int index=2;
														
								cout<<"Path Finder Path: "<<PathLength<<" Nodes" << endl;
															
								for(int i=0; i<PathLength; i++)
								{
									Path[i][0] = atof(message[index++]);
									Path[i][1] = atof(message[index++]);
									Path[i][2] = atof(message[index++]);
																	
									//cout<<message[i+2]<<" "<<message[i+3]<<endl;
									cout<<"("<<  Path[i][0] << " - " << Path[i][1] << " - " << Path[i][2] << ")" << endl;
								}
								
								//Sending Groundstation
								cout<<"Sending Path Nodes to Ground Station..."<<endl;
								
								//Load Path To Buffer to Send
								memset(buffer, '\0', sizeof(buffer));
								index=0;
								buffer[index++]=PathLength;
								
								for(int i=0; i<PathLength; i++)
								{
									DataConverter.f = Path[i][0];
									buffer[index++] = DataConverter.b[0];
									buffer[index++] = DataConverter.b[1];
									buffer[index++] = DataConverter.b[2];
									buffer[index++] = DataConverter.b[3];
									
									DataConverter.f = Path[i][1];
									buffer[index++] = DataConverter.b[0];
									buffer[index++] = DataConverter.b[1];
									buffer[index++] = DataConverter.b[2];
									buffer[index++] = DataConverter.b[3];		

									DataConverter.f = Path[i][2];
									buffer[index++] = DataConverter.b[0];
									buffer[index++] = DataConverter.b[1];
									buffer[index++] = DataConverter.b[2];
									buffer[index++] = DataConverter.b[3];	
								}
								
								while(UDPPacketTx.NewPacket){usleep(1000);}
								
								memcpy(UDPPacketTx.Payload,buffer,index);
								UDPPacketTx.CreatePacket((uint8_t*)"MGPTH",(uint8_t*)UDPPacketTx.Payload,index); //Fligh(M)anager to (G)round Station (PATH) Information
								UDPPacketTx.NewPacket = true;										
							}

						}									
					}
				}
			}		

			taskCounter++;
		}
		else
		{
			sleep(1);
		}
	}
	return 0;
}

//
//SERIAL PORT RECEIVER
//
void* UartReceiver(void *ptr)
{
	while (1)
	{
		bool result;
		unsigned char byte;
		int RxLength=0;

		//result = SerialPort.Read(&byte,1);
		result = SerialPort.Read(SerialPort.Buffer,&RxLength);
		if(result)
		{
			for(int i=0; i< RxLength; i++)
			{
				if(SerialPacketRx.Process(SerialPort.Buffer[i]))
				{
					SerialPacketCounter++;
					
					ProtocolParser_Parse(SerialPacketRx.RxBuffer);

					//Gelen Paketin ilk harfi kimden geldi�i(F:FlightController), ikinci Harfi Kime Gidece�ini Belirtir. (L: LOG, G:GroundStation)
					//if(SerialPacketRx.RxBuffer[3]=='F' && SerialPacketRx.RxBuffer[4]=='L')//FlightController To Log
					if(!memcmp((const char*)(SerialPacketRx.RxBuffer+3),"FLLOG",5))
					{
						//Log Dosyas� Haz�rm� Diye Kontrol edilir:
						if(LogPermission)
						{
							sprintf(cmd,"/home/pi/FlightManager/Log/%d.log",filenumber);
							pFile = fopen(cmd, "ab+"); //append binary mode
							fwrite(SerialPacketRx.RxBuffer,sizeof(char),SerialPacketRx.Length+3,pFile); //write packet
							fprintf(pFile, "\r\n");//??Binary File Oldugundan ihtiyac yok. C�nk� bir ise yaramayacak, arada data olarakta /r ve /n  (10,13) gelebiliyor.
							fclose(pFile);
						}
					}
					else if(SerialPacketRx.RxBuffer[4]=='G')//TELEMETRY Sended By UDP //FGALL
					{
						if(UDPPacketTx.NewPacket==false)
						{
							memcpy(UDPPacketTx.RxBuffer,SerialPacketRx.RxBuffer,SerialPacketRx.Length+3);
							UDPPacketTx.Length = SerialPacketRx.Length;
							UDPPacketTx.NewPacket = true;
						}
						FGALLCounter++;
					}
					//else if(SerialPacketRx.RxBuffer[3]=='H' && SerialPacketRx.RxBuffer[4]=='I' && SerialPacketRx.RxBuffer[5]=='L' && SerialPacketRx.RxBuffer[5]=='C' && SerialPacketRx.RxBuffer[6]=='N')
					else if(!memcmp((const char*)(SerialPacketRx.RxBuffer+3),"HILCN",5))
					{
						HILCNCounter++;
					}
					else//Other Packets Sended By TCP
					{
						memcpy(TCPPacketTx.RxBuffer,SerialPacketRx.RxBuffer,SerialPacketRx.Length+3);
						TCPPacketTx.Length = SerialPacketRx.Length;
						TCPPacketTx.NewPacket = true;
					}
				}
			}
		}
		else
		{
			printf("Serial Read Error\n");
			usleep(10000);
		}
	}
	return 0;
}

//SERIALPORT TRANSMITTER
void* UartSender(void *ptr)
{
	while (1)
	{
		if(SerialPacketTx.NewPacket)
		{
			SerialPort.Write(SerialPacketTx.RxBuffer,SerialPacketTx.Length+3);
			SerialPacketTx.NewPacket =false;
		}
		else if(HILPacketTx.NewPacket)
		{
			SerialPort.Write(HILPacketTx.RxBuffer,HILPacketTx.Length+3);
			HILPacketTx.NewPacket =false;
		}

		usleep(1000);
	}
	return 0;
}

//TCP RECEIVER
void* ServerReceiver(void *ptr)
{
	while (1)
	{
		bool result;
		unsigned char byte;

		if(Server.isClientConnected)
		{
			result = Server.ReadSocket(&byte,1);
			if(result)
			{
				//printf("Network: %c\n",byte);
				if(TCPPacketRx.Process(byte))
				{
					NetworkPacketCounter++;

					//GroundStation to Mission Computer Packets starts with GM
					if(!memcmp((const char*)(TCPPacketRx.RxBuffer+3),"GM",2))
					{
						ProtocolParser_Parse(TCPPacketRx.RxBuffer);
					}
					else
					{
						memcpy(SerialPacketTx.RxBuffer,TCPPacketRx.RxBuffer,TCPPacketRx.Length+3);
						SerialPacketTx.Length = TCPPacketRx.Length;
						SerialPacketTx.NewPacket = true;
					}
				}
			}
			else
			{
				usleep(1000);
			}
		}
		else
		{
			sleep(1);
		}
	}
	return 0;
}

//TCP CONNECTION LISTENER
void* ServerListener(void *ptr)
{
	char response[1024];
	
	while (1)
	{
		//if(!Server.isClientConnected)
		//{
			//printf("[ServerListener] ReStarting Server\n");
			Server.StartServer();
			UDPSocket.SetTargetIp(Server.connected_ip);
			
			cout<<"Restarting Stream Server for: " << Server.connected_ip<<endl;
			system("sudo killall raspivid &");		
			system("sudo killall gst-launch-1.0 &");			
			sprintf(response,"sudo /home/pi/stream.sh %s &", Server.connected_ip);
			system(response);			
			
		//}
		sleep(1);
	}
	return 0;
}

//TCP TRANSMITTER
void* ServerSender(void *ptr)
{
	while (1)
	{
		if(Server.isClientConnected && TCPPacketTx.NewPacket)
		{
			Server.WriteSocket(TCPPacketTx.RxBuffer,TCPPacketTx.Length+3);
			TCPPacketTx.NewPacket=false;
		}
		else
		{
			usleep(1000);
		}
	}
	return 0;
}

//UDP RECEIVER
void* UDPReceiver(void *ptr)
{
	while (1)
	{
		int result;
		unsigned char byte[65535];

		result = UDPSocket.ReadSocket(byte);

		if(result)
		{
			for(int i=0;i<result;i++)
			{
				if(UDPPacketRx.Process(byte[i]))
				{
					printf("UDP Packet Received, directing to SerialPort...\n");

					UDPPacketCounter++;

					memcpy(SerialPacketTx.RxBuffer,UDPPacketRx.RxBuffer,UDPPacketRx.Length+3);
					SerialPacketTx.Length = UDPPacketRx.Length;
					SerialPacketTx.NewPacket = true;

				}
			}
		}
		else
		{
			usleep(100000);
		}
	}
	return 0;
}

//UDP TRANSMITTER
void* UDPSender(void *ptr)
{
	while (1)
	{
		if(UDPSocket.isConnected && UDPPacketTx.NewPacket)
		{
			UDPSocket.WriteSocket(UDPPacketTx.RxBuffer,UDPPacketTx.Length+3);
			UDPPacketTx.NewPacket = false;
			//cout<<"Udp Tx Size: "<< UDPPacketTx.Length+3 <<endl;
			//cout<<UDPPacketTx.RxBuffer[3]<<UDPPacketTx.RxBuffer[4]<<UDPPacketTx.RxBuffer[5]<<UDPPacketTx.RxBuffer[6]<<UDPPacketTx.RxBuffer[7]<<endl;
		}
		else
			usleep(1000);
	}
	return 0;
}


//UDP HIL RECEIVER
void* UDPHILReceiverRoutine(void *ptr)
{
	while (1)
	{
		int result;
		unsigned char byte[65535] = {0};

		result = UDPHILReceiver.ReadSocket(byte);

		if(result>0)
		{
			result = HILService_ParseHILPacket(byte,result);
			if(result)
			{
				UDPHILSender.SetTargetIp(UDPHILReceiver.ip);

				HILPacketTx.CreatePacket((uint8_t*)"HILST",(uint8_t*)HILPacketTx.Payload,HILService_HILStateToAutopilot(&HILState,HILPacketTx.Payload));
				HILStatePacketCounter++;

				//printf("Orientation: (%3.2f) (%3.2f) (%3.2f)\n",HILState.Orientation[0],HILState.Orientation[1],HILState.Orientation[2]);
				//printf("Position: (%3.2f) (%3.2f) (%3.2f)\n",HILState.Position[0],HILState.Position[1],HILState.Position[2]);
				//printf("Angular: (%3.2f) (%3.2f) (%3.2f)\n",HILState.AngularSpeed[0],HILState.AngularSpeed[1],HILState.AngularSpeed[2]);
			}
			else
				printf("Parse Error Xplane\n");

			//printf("UDP:%c-%c-%d\n",byte[0],byte[1],result);
		}
		else if (result==0)
		{
			usleep(1000);
			printf("UDPHIL Returns 0!\n");
		}
		else if (result<0)
		{
			usleep(1000);
			//printf("UDPHIL ERROR!\n");
		}
	}
	return 0;
}

//UDP HIL TRANSMITTER
void* UDPHILSenderRoutine(void *ptr)
{
	while (1)
	{
		if(HILPacketToSim.NewPacket && HILStatePacketRate>1)
		{
			UDPHILSender.WriteSocket(HILPacketToSim.RxBuffer,HILPacketToSim.Length);
			HILPacketToSim.NewPacket = false;
		}
		else
			usleep(1000);
	}
	return 0;
}


//MissionTCP RECEIVER
void* MissionReceiver(void *ptr)
{
	while (1)
	{
		bool result;
		unsigned char byte;

		if(MissionServer.isClientConnected)
		{
			result = MissionServer.ReadSocket(&byte,1);
			if(result)
			{
				//printf("Network: %c\n",byte);
				if(MissionPacketRx.Process(byte))
				{
					MissionPacketCounter++;

				}
			}
			else
			{
				usleep(1000);
			}
		}
		else
		{
			sleep(1);
		}
	}
	return 0;
}

//TCP CONNECTION LISTENER
void* MissionListener(void *ptr)
{
	while (1)
	{
		if(!MissionServer.isClientConnected)
		{
			MissionServer.StartServer();
		}
		sleep(1);
	}
	return 0;
}

//TCP TRANSMITTER
void* MissionSender(void *ptr)
{
	while (1)
	{
		if(MissionServer.isClientConnected && MissionPacketTx.NewPacket)
		{
			MissionServer.WriteSocket(MissionPacketTx.RxBuffer,MissionPacketTx.Length+3);
			MissionPacketTx.NewPacket=false;
		}
		else
		{
			usleep(1000);
		}
	}
	return 0;
}


bool GetStringFromFile(char *ffile, char *value)
{
	FILE * fptr;
	char c;
	fptr = fopen(ffile, "r");  // open said file for reading.
    if(!fptr)
	{
		return false;
	}
	else
	{
		while(!feof(fptr))
		{
			fgets(value,1024,fptr);
		}
		fclose(fptr);                   // don't call this is fptr is NULL.
		//remove(ffile);             // clean up
		return true;
	}
}

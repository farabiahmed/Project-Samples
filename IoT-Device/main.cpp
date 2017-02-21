/**
 ******************************************************************************
 * @file    main.cpp
 * @author  Farabi Ahmed Tarhan
 * @version 
 * @date    06-03-2014
 * @brief   Main program body
 ******************************************************************************
*/

#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <cstdlib>
#include <iostream>
#include <cctype>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>
#include <sys/time.h> 
#include "cMysql.h"
#include "cTime.h"
#include "system.h"
#include "Uart.h"
#include "Telemetry.h"
#include <arpa/inet.h>
#include "GPIOClass.h"
#include <string>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h> 
#include <signal.h>
#include <errno.h>
#include "mysql/mysql.h"
#include "mutechProtocol.h"
#include "heatmap.h"

#define BUFLEN 512  //Max length of buffer
#define PORT 8888   //The port on which to listen for incoming data

#define MAXATTEMPTS 5
using namespace std;

//References
//UDP: http://www.binarytides.com/programming-udp-sockets-c-linux/


/*
sudo killall -v xCounter
sudo g++ -std=c++0x -o xCounter main.cpp cMysql.cpp GPIOClass.cpp cTime.cpp Uart.cpp Telemetry.cpp -lpthread `mysql_config --cflags --libs` -Wno-write-strings

sudo svn commit -m `cat build-version.txt`
*/

//Thread Prototypes
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
void* TestA(void *ptr);
void* TestB(void *ptr);
void* SystemChecker(void *ptr);
void* Refresher(void *ptr);
void* UartReceiver(void *ptr);
void* UserInputReceiver(void *ptr);

int AliveRefresher=0;
int AliveUartReceiver=0;
int AliveUserInputReceiver=0;
int AliveUdpListener=0;

//
int RandomTimeDelayForMainTask=0;
int TimeOutForMainTask = -1;

//Class Variables
cMysql oMysql;				//Mysql Service for LocalHost
cMysql oMysqlMainService;	//Mysql Service for MuTechnologies.net
cTime oTime;
CONFIG Config;


Uart SerialPort;
Telemetry SerialPacketRx;
Telemetry SerialPacketTx;

Sensor_Struct Sensor[8];
int LastHour=0,LastDay=0,LastMonth=0,LastYear=0,LastMinute=0;


//Global Variables
int status = 0;
//char Version[20] = "2.5.2a";

//Detect Whether Time is Synched before.
int TimeSynched = 0;

char Version_MCU[20] = "";
char StartTime[30] = "";
char PacketRevision[50]="";
char cmd[1024];
bool internetStatus=false;
int watchdogDeviceHandle;
int watchdogTimeout = 15;
int second=0,minute=0,hour=0,day=0,month=0,year=0;

char cpufrequency[20] ="489";
char cpuload[20] ="168";
char temperature[20] ="-10";
char diskusage[20] ="999";
char vpnip[50] = "0.0.0.0";


int openingHour=9;
int closingHour=23;

char MainServiceHost[50] = "www.mutechnologies.net";
char MainServiceUser[15] = "xcounter";
char MainServicePass[15] = "mutech";   
char MainServiceDB[15] = "xcounter";  

//Database
char dbhost[50] = "localhost";
char dbuser[15] = "root";
char dbpassword[15] = "mutech";   
char database[15] = "xCounterDB";   
int dbport = 3306;

int CurrentAttempt=0;

//UPS
int UPSState=-1;
int UPSConnected=0;
int UPSStateCounter=0;
GPIOClass* gpioPsu;
GPIOClass* gpioBlueLed;
char blueLedStatus[1];

//UDP 
struct sockaddr_in si_me, si_other;
int s, i, slen = sizeof(si_other) , recv_len;
char buf[BUFLEN];
char message[BUFLEN];

// Define the function to be called when ctrl-c (SIGINT) signal is sent to process
void
signal_callback_handler(int signum)
{
   printf("Caught signal %d\n",signum);
   // Cleanup and close up stuff here

   // Terminate program
   exit(signum);
}


int main(int argc, const char* argv[])
{
	cout << "##################################" << endl << flush;
	cout << "xCounter C/C++ Service Started.   " << endl << flush;
	cout << "##################################" << endl << flush;
	/**Brief: initialization
	* 
	*/
	// Register signal and signal handler
    signal(SIGINT, signal_callback_handler);
	
	////////////////////////////////////////////////////////////////////////////////
	//Watchdog
	////////////////////////////////////////////////////////////////////////////////	
	// open watchdog device on /dev/watchdog
	if ((watchdogDeviceHandle = open("/dev/watchdog", O_RDWR | O_NOCTTY)) < 0) {
		printf("[WATCHDOG] Error: Couldn't open watchdog device! %d\n", watchdogDeviceHandle);		
	} 	
	ioctl(watchdogDeviceHandle, WDIOC_SETTIMEOUT, &watchdogTimeout);
	ioctl(watchdogDeviceHandle, WDIOC_GETTIMEOUT, &watchdogTimeout);
	printf("[WATCHDOG] The watchdog timeout is %d seconds.\n\n", watchdogTimeout);	
	
	////////////////////////////////////////////////////////////////////////////////
	//LOCAL VARIABLES
	////////////////////////////////////////////////////////////////////////////////
	pthread_t threadRefresher;
	pthread_t threadSerialReceiver;
	pthread_t threadUserInput;
	pthread_t threadUdpListener;
	pthread_t threadTestA;
	pthread_t threadTestB;
	pthread_t threadSystemChecker;
	
	// baslangicta false olarak kaliyor. egere cihaz DB ye baglanip kayitli olup olmadigina bakamaz ise 
	// xCounter yazlimi bazi ozellikeriyerine getiremeyecektir. kopyalamanin onune gecmek icin yapilmistir.
	// ftp gonderme fonksiyonu calismaz
	// Bulut sistemine (Main Servis) B827EB966CC1 ornegin (MACID_DB) yazim yapmasinda bir sikinti olmaz 
	Config.Registration=false;
	
	//initialize random seed:
	srand (time(NULL));
	
	printf("SoftwareVersion: %s\n",VERSION);
	
	////////////////////////////////////////////////////////////////////////////////
	//UDP CONFIGURATION
	////////////////////////////////////////////////////////////////////////////////

	//create a UDP socket
    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        //die("socket");
		cout<<"[Socket Error]"<<endl;
    }
	
    int reuse = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

#ifdef SO_REUSEPORT
    if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0) 
        perror("setsockopt(SO_REUSEPORT) failed");
#endif

    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
		cout<<"[Bind Error]"<<endl;
        //die("bind");
    }
	
	////////////////////////////////////////////////////////////////////////////////
	//GET CONFIGURATION
	////////////////////////////////////////////////////////////////////////////////
	oMysql = cMysql("localhost","root","mutech","xCounterDB",3306);
	oMysql.GetConfig(&Config);	
	
	printf("\n[Configuration]	\n");
	printf("Timezone      : %s\n",Config.Timezone);
	printf("ServerIP      : %s\n",Config.serverIP);
	printf("IPType		  : %s\n",Config.IpType);
	printf("DeviceIP      : %s\n",Config.deviceIP);
	printf("DeviceNetMask : %s\n",Config.deviceNetMask);
	printf("DeviceGateway : %s\n",Config.deviceGateway);
	printf("DeviceName    : %s\n",Config.deviceName);
	printf("CheckExIPLink : %s\n",Config.CheckExIPLink);
	//printf("MainServiceIP : %s\n",Config.MainService.IP);
	//printf("MainServiceUI : %s\n",Config.MainService.UI);
	//printf("MainServicePW : %s\n",Config.MainService.PW);
	printf("MainServiceDB : %s\n",Config.MainService.DB);
	printf("NetworkDevice : %s\n",Config.NetworkDevice);
	printf("WifiSSID      : %s\n",Config.WifiSSID);
	printf("WifiPassword  : %s\n",Config.WifiPassword);
	//FTP 
	printf("FtpHost		  : %s\n",Config.ftphost);
	printf("FtpUser		  : %s\n",Config.ftpuser);
	printf("FtpPassword   : %s\n",Config.ftppassword);
	
	//Shop Hours
	printf("Opening Hour  : %s\n",Config.openinghour);
	printf("Closing Hour  : %s\n",Config.closinghour);

	//Operation Mode
	printf("OperationMode : %s\n",Config.operationmode);
	
	//Device Mode
	printf("DeviceMode : %s\n",Config.devicemode);
	
	openingHour = atoi(Config.openinghour);
	closingHour = atoi(Config.closinghour);
	
	GetStringFromFile("version.txt",PacketRevision);
	
	//sprintf(cmd,"sudo echo 'nameserver 8.8.8.8' > /etc/resolv.conf");
	
	////////////////////////////////////////////////////////////////////////////////
	//Get MAC, Extarnal IP and Local IP 
	////////////////////////////////////////////////////////////////////////////////
	printf("[Timezone] Configurating... %s",Config.Timezone);
	//sprintf(cmd,"echo 'export TZ=Europe/Istanbul' > test");
	sprintf(cmd,"sudo cp /usr/share/zoneinfo/%s /etc/localtime",Config.Timezone);
	system(cmd);
	
	////////////////////////////////////////////////////////////////////////////////
	//Get MAC, Extarnal IP and Local IP 
	////////////////////////////////////////////////////////////////////////////////
	
	sprintf(cmd,"sudo curl --silent --connect-timeout 5 %s > ip", Config.CheckExIPLink);
	system(cmd);
	
	if(GetStringFromFile("ip",Config.ExternalIP))
	{
		if(isValidIpAddress(Config.ExternalIP)==false)
			strcpy(Config.ExternalIP,"0.0.0.0");
		
		printf("[External IP] %s\n",Config.ExternalIP);
	}
	/////////////////////////////////////////////////////////////////////////////
	
	sprintf(cmd,"sudo ifconfig %s | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}' > lanip",Config.NetworkDevice);
	system(cmd);
	
	if(GetStringFromFile("lanip",Config.LocalIP))
	{
		RemoveNewLineFromString(Config.LocalIP);
		
		if(isValidIpAddress(Config.LocalIP)==false)
			strcpy(Config.LocalIP,"0.0.0.0");
		
		printf("[Lan IP Address] %s\n",Config.LocalIP);
	}	
	/////////////////////////////////////////////////////////////////////////////
	
	sprintf(cmd,"sudo cat /sys/class/net/eth0/address > mac");
	system(cmd);
	
	if(GetStringFromFile("mac",Config.MACAddress))
	{
		RemoveNewLineFromString(Config.MACAddress);
		mac2char(Config.MACAddress,Config.MACAddress);	
		printf("[Serial Number] %s\n",Config.MACAddress);
	}	
	/////////////////////////////////////////////////////////////////////////////
	//time update icin kullaniliyor
	oTime.UpdateCurrentTime();
	sprintf(StartTime,"%s",oTime.timeStr);
	printf("[Current Time] %s\n",StartTime);	
	
	
	/////////////////////////////////////////////////////////////////////////////
	//Create Note Table if not exist
	/////////////////////////////////////////////////////////////////////////////
	
	if(oMysql.Connect())
	{
		printf("[Creating Note Table]\n");	
		oMysql.CheckTable("Log");
		oMysql.CreateNoteTable("Log");		
		oMysql.Disconnect();
		
		oTime.UpdateCurrentTime();
		oMysql.AddNote("StartTime","","0",oTime.timeStr);
	}
	else
	{
		printf("[Creating Note Table] ERROR!\n");	
	}
	/////////////////////////////////////////////////////////////////////////////
	//Register device to the www.mutechnologies.net servers
	/////////////////////////////////////////////////////////////////////////////	
	//oMysqlMainService = cMysql(MainServiceHost,MainServiceUser,MainServicePass,MainServiceDB,3306);

	/*
	//Old Direct DB Access Registration Routine. Keep it.
	if(oMysqlMainService.MainServiceRegister(Config.MACAddress,&Config.Command,Config.deviceName,Config.ExternalIP,Config.LocalIP,Version,oTime.timeStr,StartTime))
	{
		Config.Registration=true;
		printf("[Device Registration] TRUE Command:%d\n",Config.Command);
	}
	else
	{
		Config.Registration=false;
		printf("[Device Registration] FALSE Command:%d\n",Config.Command);
		
		oMysql.AddNote("InitialDeviceRegistration","Error","0",oTime.timeStr);
	}
	*/
	
	//Update CPUFreq,Temp,CPUUsage vs.
	UpdateSystemProperties();
	
	
	sprintf(cmd,"sudo hamachi | grep 'address' | cut -d: -f2 | awk '{ print $1}' > vpnip");	
	system(cmd);
	if(GetStringFromFile("vpnip",vpnip))
	{
		RemoveNewLineFromString(vpnip);
		if(isValidIpAddress(vpnip)==false)
			strcpy(vpnip,"0.0.0.0");		
		printf("[VPN Ip] %s\n",vpnip);
	}
	
	{
		CurrentAttempt=0;
		while(CurrentAttempt < MAXATTEMPTS)
		{
			//Try To Register Device to Remote Server
			if(MainServiceRegistration())
			{
				//if successfull break...
				break;
			}
			else
			{
				//if failed note it to LOG then re-try.
				oTime.UpdateCurrentTime();
				oMysql.AddNote("MainServiceRegister","Error","1",oTime.timeStr);
			}		

			//Sleep 1 sec in order to reduce network traffic
			sleep(1);
			
			//Increase Current Attempt
			CurrentAttempt++;
		}
	}
	
	
	//Inform User About Registration
	printf("[Device Registration] %x\n",Config.Registration);
	

	/*
	 * SerialPort Initialization
	 */
	cout << "[SerialPort] Initialization..." << endl << flush;
	sprintf(cmd,"sudo chmod a+rw /dev/ttyAMA0");
	system(cmd);	 	 
	SerialPort.Start("/dev/ttyAMA0",B9600);
	cout << "SerialPort Opened!" << endl << flush;
	//MCU_Request_Version();
	
	//UpdateLocalDatabase();
	
	/////////////////////////////////////////////////////////////////////////////
	//Set GPIO's
	/////////////////////////////////////////////////////////////////////////////
	//RPI24 ve RPI25 -> UPS Active Signal
	
	gpioPsu = new GPIOClass("25"); //create new GPIO object to be attached to  GPIO4
    gpioPsu->export_gpio(); //export GPIO4 
    cout << " GPIO pins exported" << endl; 
    gpioPsu->setdir_gpio("in"); //GPIO4 set to output
    cout << " Set GPIO pin directions" << endl;
		
	cout << " UPS Connection Status: " << flush;
	string inputstate;
	for(int i = 0; i < 5; i++)
	{
		gpioPsu->getval_gpio(inputstate);
		if(inputstate == "0")
		{
			UPSConnected = 0;
			break;
		}
		else
			UPSConnected = 1;
		
		cout << "." << flush;
		
		sleep(1);
	}
	
	if(UPSConnected)
		cout << " OK!" << endl;
	else 
		cout << " ERROR!" << endl;
	
	
	cout << "##################################" << endl << flush;
	
	
	/////////////////////////////////////////////////////////////////////////////
	//Set Blue Led
	/////////////////////////////////////////////////////////////////////////////
	//RPI27 -> Blue Led Internet Active Signal
	
	gpioBlueLed = new GPIOClass("27"); //create new GPIO object to be attached to  GPIO4
        gpioBlueLed->export_gpio(); //export 
        cout << " GPIO27 pin exported" << endl; 
        gpioBlueLed->setdir_gpio("out"); //output 
        cout << " Set GPIO pin directions" << endl;
	
	cout << "##################################" << endl << flush;
		
		
	/////////////////////////////////////////////////////////////////////////////
	//Check Internet Status
	/////////////////////////////////////////////////////////////////////////////	
	internetStatus = CheckInternet();

	/////////////////////////////////////////////////////////////////////////////	
	//Threads
	/////////////////////////////////////////////////////////////////////////////	
        if (pthread_mutex_init(&lock, NULL) != 0)
        {
                    oTime.UpdateCurrentTime();
                    oMysql.AddNote("MutexInit","Error","1",oTime.timeStr);		
                                    
            printf("\n *** mutex init failed *** \n");
            return 1;
        }
            
	RandomTimeDelayForMainTask = rand() % 120;
	printf("[RANDOM TIME DELAY] %d \n", RandomTimeDelayForMainTask);
	
	if(strcmp(Config.operationmode,"master") == 0 || strcmp(Config.devicemode,"heatmap") == 0)
	{				
		DataBase_GetMissedMainServerSensors();
	}
		
	//status = pthread_create(&threadUserInput, NULL, UserInputReceiver, NULL);
		/////////////////////////////////////////////////////////////////////////////	
	//Threads
	/////////////////////////////////////////////////////////////////////////////	
	transmitDataPack.payLoadCounter = 0;
	transmitDataPack.payLoad[transmitDataPack.payLoadCounter++] = (MP_PACKTYPE_RPI_ALIVE >> 8);
	transmitDataPack.payLoad[transmitDataPack.payLoadCounter++] = (MP_PACKTYPE_RPI_ALIVE & 0xFF);
	
	PrepareDataToSend(&transmitDataPack);
	
	
	/*
	cout<<"Test Connection"<<endl<<flush;
	char response[2000];
	bool ret=false;
	for(int j=0; j<100000; j++)
	{
		cout << j << " " <<endl<< flush;
		ret = oMysql.ReadCell(response,2015,11,28,10);
		//ret = oMysql.WriteCell(2015,11,28,10,response);
		oTime.UpdateCurrentTime();		
	}
	*/
	status = pthread_create(&threadSystemChecker, NULL, SystemChecker, NULL);
	status = pthread_create(&threadSerialReceiver, NULL, UartReceiver, NULL);
	status = pthread_create(&threadRefresher, NULL, Refresher, NULL);
	status = pthread_create(&threadUdpListener, NULL, UdpListener, NULL);

	
	//status = pthread_create(&threadTestA, NULL, TestA, NULL);
	//status = pthread_create(&threadTestB, NULL, TestB, NULL);	
	

	status = pthread_join(threadSystemChecker, NULL);
	
	
	sleep(2);

	close(watchdogDeviceHandle);
	pthread_mutex_destroy(&lock);
	
	cout << "##################################" << endl << flush;
	cout << "xCounter C/C++ Service Terminated." << endl << flush;
	cout << "##################################" << endl << flush;
	return 0;
}
void* SystemChecker(void *ptr)
{
	cout << "[System Checker] Starting " <<endl<<flush;
	while (1)
	{
		oTime.UpdateCurrentTime();
		cout << "[System Checker] "<< oTime.timeStr <<" Refresher: "<< AliveRefresher << " UdpListener: " << AliveUdpListener << " UartReceiver: " << AliveUartReceiver << " UserInputReceiver: "<< AliveUserInputReceiver << " Internet: "<< internetStatus <<endl<<flush;

		sleep(60);
	}
	cout << "[System Checker] Stopping " <<endl<<flush;
}

void* TestA(void *ptr)
{
	while (1)
	{
		char response[2000];
		cout << "Thread A" <<endl<<flush;
		int mutexresponse=0;
		
		mutexresponse = pthread_mutex_lock(&lock);
		if(mutexresponse==0)
		{
			oMysql.ReadCell(response,2015,11,18,9);	
			pthread_mutex_unlock(&lock);
		}
		else
		{
			cout << "Thread A Mutex ERROR" <<endl<<flush;
		}
		//sleep(1);
		usleep(10000);
	}
}

void* TestB(void *ptr)
{
	while (1)
	{
		char response[2000];
		cout << "Thread B" <<endl<<flush;
		//oMysql.ReadCell(response,2015,11,18,9);	
		int mutexresponse=0;
		
		mutexresponse = pthread_mutex_lock(&lock);
		if(mutexresponse==0)
		{
			oMysql.ReadCell(response,2015,11,18,9);	
			pthread_mutex_unlock(&lock);
		}
		else
		{
			cout << "Thread B Mutex ERROR" <<endl<<flush;
		}
		//sleep(1);
		usleep(1000);
	}
}

void* UdpListener(void *ptr)
{	
	char response[2000];
	char message[50][2000];	
	char str[100];
	char str2[100];
	
	while (1)
	{			
		//cout<<"[xCounter Listening UDP]"<<endl;
        //try to receive some data, this is a blocking call
		memset (buf, 0, BUFLEN); //ToDo: Buf local degisken olarak degistirilsin
        if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, (socklen_t*)&slen)) == -1)
        {
			cout<<"[UDP Read Error]"<<endl;
            //die("recvfrom()");
        }
        //cout<<"[xCounter UDP Received]"<<endl;
		
		int i = 0,in,out,y,m,d,h;

		//cout<< "[UNDETERMINED] " << buf << endl<<flush;
		
		//Parse String
		//LIVE,CH:0,IN:%d,OUT:%d,END
		char* tokens = strtok (buf,",");		
		while (tokens != NULL)
		{
			strcpy(message[i++],tokens);
			tokens = strtok (NULL, ",");
		}
		//cout<<"[xCounter Packet Parsed]"<<endl;
		
		if(!strcmp(message[0],"LIVE"))
		{
			//LIVE,CH:0,IN:%d,OUT:%d,END
			//sscanf(buf,"CH:%d,IN:%d,OUT:%d,END",&i,&in,&out);		
			sscanf(message[1],"CH:%d",&i);
			sscanf(message[2],"IN:%d",&in);
			sscanf(message[3],"OUT:%d",&out);
			
			Sensor[i].In=in;
			Sensor[i].Out=out;

			//Get Sensor Ip
			strcpy(Sensor[i].ip,inet_ntoa(si_other.sin_addr));
			Sensor[i].remoteSock = si_other;

			//print details of the client/peer and the data received
			//printf("UDP (LIVE) %16s Sensor:%d In:%d Out:%d\n", Sensor[i].ip,i,in,out);
			Sensor[i].Connected=1;			
		}
		else if(!strcmp(message[0],"LIVEr2"))
		{

			//LIVEr2,Y:%d,M:%d,D:%d,H:%d,CH:%d,IN:%d,OUT:%d,END
			sscanf(message[1],"Y:%d",&y);
			sscanf(message[2],"M:%d",&m);
			sscanf(message[3],"D:%d",&d);			
			sscanf(message[4],"H:%d",&h);			
			sscanf(message[5],"CH:%d",&i);
			sscanf(message[6],"IN:%d",&in);
			sscanf(message[7],"OUT:%d",&out);
		
			Sensor[i].Last_In = Sensor[i].In;
			Sensor[i].Last_Out = Sensor[i].Out;
			Sensor[i].In=in;
			Sensor[i].Out=out;
			

			if(Sensor[i].Last_In!=Sensor[i].In || Sensor[i].Last_Out!=Sensor[i].Out)
			{
				//print details of the client/peer and the data received
				printf("UDP (LIVEr2) %16s Sensor:%d %04d-%02d-%02d %02dh In:%d Out:%d\n", Sensor[i].ip,i,y,m,d,h,in,out);
			
			
				// Refresher thread ayni DB den veri okuyabilir. cakisirsa sistem patlar
				// mutex Lock cok onemli
				/**ToDo: LocalDB de Belirlenen calisma saatleri disindaki 
				* hucreler yazma yapilmasin
				*/
				//pthread_mutex_lock(&lock); 
				int mutexresponse=0;

				mutexresponse = pthread_mutex_lock(&lock);
				if(mutexresponse==0)
				{
					UpdateLocalDatabase(y,m,d,h,i,in,out);		
					oMysql.DataBase_ChangeLogStatusOfSensorToZero(y,m,d,h); //inorder to resend the updated local data to mainserver.
					pthread_mutex_unlock(&lock);
				}
				else
				{
					oTime.UpdateCurrentTime();
					oMysql.AddNote("MutexError","UdpListener","1",oTime.timeStr);	
					cout << "[Thread UdpListener] Mutex ERROR" <<endl<<flush;
				}
			}
			//Get Sensor Ip
			strcpy(Sensor[i].ip,inet_ntoa(si_other.sin_addr));
			Sensor[i].remoteSock = si_other;

			Sensor[i].Connected=1;			
		}		
		else if(!strcmp(message[0],"HISTORY"))
		{
			int In[8]={0};
			int Out[8]={0};
				
			//HISTORY,5,2015,07,25,16,76,72,END
			printf("[xCounter HISTORY MESSAGE Received] CH:%d %04d-%02d-%02d %02dh Value:%d.%d\n",atoi(message[1]),atoi(message[2]),atoi(message[3]),atoi(message[4]),atoi(message[5]),atoi(message[6]),atoi(message[7]));		
			
			//Get Sensor Ip
			strcpy(Sensor[atoi(message[1])].ip,inet_ntoa(si_other.sin_addr));
			Sensor[atoi(message[1])].remoteSock = si_other;
			
			//UPPDATE CELL			
			//oMysql.WriteCell(LastYear,LastMonth,LastDay,LastHour,UpdateSensorString());
			pthread_mutex_lock(&lock);
			if(oMysql.ReadCell(response,atoi(message[2]),atoi(message[3]),atoi(message[4]),atoi(message[5]))==0)
			{
				//Parse response and change relevant value. 
				printf("[CURRENT VALUE] %s\n",response);
				
				//response: %d.%d/%d.%d/%d.%d/%d.%d/%d.%d/%d.%d/%d.%d/%d.%d

				sscanf(response,"%d.%d/%d.%d/%d.%d/%d.%d/%d.%d/%d.%d/%d.%d/%d.%d",				
				&(In[0]),&(Out[0]),
				&(In[1]),&(Out[1]),
				&(In[2]),&(Out[2]),
				&(In[3]),&(Out[3]),
				&(In[4]),&(Out[4]),
				&(In[5]),&(Out[5]),
				&(In[6]),&(Out[6]),
				&(In[7]),&(Out[7]));							
			}
			//no valid cell value found. recreate it.
			//Message[1] SensorNo
			//Message[6] IN verisi
			//Message[7] OUT verisi
			In[atoi(message[1])] = atoi(message[6]);
			Out[atoi(message[1])] = atoi(message[7]);			
			//Ilgili sensore ait yeni veriler hucrede olmasi gereken yere yazildi
	
			sprintf(response,"%d.%d/%d.%d/%d.%d/%d.%d/%d.%d/%d.%d/%d.%d/%d.%d",
			In[0],Out[0],
			In[1],Out[1],
			In[2],Out[2],
			In[3],Out[3],
			In[4],Out[4],
			In[5],Out[5],
			In[6],Out[6],
			In[7],Out[7]);
			
			cout<<"[WRITING NEW VALUES]: "<<response<<endl;
						
			cout<<message[1]<< "-"<< message[2]<< "-"<< message[3]<< "-"<< message[4]<< "-"<< message[5]<< endl;
			
			/*
			cout<<message[1]<<endl;	// Sensor Channel NO
			cout<<message[2]<<endl; // Year
			cout<<message[3]<<endl; // Mounth
			cout<<message[4]<<endl; // Day
			cout<<message[5]<<endl; // Hour (16h)
			*/
			
			
			//v2.6.1 de eklendi.
			//
			sprintf(str,"%s",response);		
			sprintf(str2,"%04d-%02d-%02d %02d",atoi(message[2]),atoi(message[3]),atoi(message[4]),atoi(message[5]));							
			oMysql.AddNote("RawData",str,"0",str2);		

			//Organize Sensor Datas v2.6.1'de eklendi.
			ReOrganizeSensors(response);

			//Yeni verileri localdb note tablosuna yazalım. 
			//Eğer mutech.net'e gönderebilirsek flagı 0 dan 1 e çevirmeliyiz.
			//0 data yeni demek. 1 olması için Mutech.nete yazmalıyız. 
			sprintf(str,"%s",response);		
			sprintf(str2,"%04d-%02d-%02d %02d",atoi(message[2]),atoi(message[3]),atoi(message[4]),atoi(message[5]));									
			oMysql.AddNote("ReOrganized",str,"0",str2);									
			
			if(oMysql.WriteCell(atoi(message[2]),atoi(message[3]),atoi(message[4]),atoi(message[5]),response))
			{
				//Eger Yeni Veriler Yerine Konabilirse Karsı tarafa haber et.		
				cout<< "[xCounter SENSOR ID] Converting by atoi..."  << endl;								
				i=atoi(message[1]);				
				
				cout<< "[SENDING ACK] : "<< i << endl;
				
				sprintf(response,"ACK,%04d,%02d,%02d,%02d,END",atoi(message[2]),atoi(message[3]),atoi(message[4]),atoi(message[5]));
				
				if (sendto(s, response, strlen(response) , 0 , (struct sockaddr *) &(Sensor[i].remoteSock), sizeof (Sensor[i].remoteSock))==-1)
				{
					cout<<"[UDP Send Error]"<<endl;
				}					
				else
				{
					cout<<"[xCounter UDP Send Reset Message] Successfull"<<endl;
				}
			}
			else
			{
				//Todo: Add Note Log tablosuna giris yapilsin				
				cout<<"[MYSQL WriteCell] Error"<<endl;
				
				oTime.UpdateCurrentTime();
				oMysql.AddNote("HistoryWriteCell","Error","1",oTime.timeStr);				
			}
			pthread_mutex_unlock(&lock);
			cout<<"[xCounter History Routine] End"<<endl;
		}
		else
		{
			cout<<"[xCounter UNDETERMINED Message]"<< message[0] <<endl;
		}
		//sleep(1);
		
		AliveUdpListener++;
		
	}

	return 0;
}

/**Note: Cin icerisindeki bug nedeni belli bir sure sonra surekli veri geliyor hic giris olmadigi halde
* Bu thread devre disi birakilmistir.
*/
void* UserInputReceiver(void *ptr)
{
	while (1)
	{
		char userinput[50]; 		
		cin >> userinput;
		cout<< "[User Input]" << userinput << endl << flush;
		
		if(!strcmp(userinput,"VR"))
		{
			MCU_Request_Version();
		}
		if(!strcmp(userinput,"RR"))
		{
			//MCU_Request_Reset(); Which Hour Problem...
		}
		if(!strcmp(userinput,"GT"))
		{
			MCU_Get_Time();
		}		
		if(!strcmp(userinput,"GD"))
		{
			MCU_Get_Date();
		}		
		
		AliveUserInputReceiver++;
	}
}

void* Refresher(void *ptr)
{

	minute = oTime.GetMinute();
	hour = oTime.GetHour();
	day = oTime.GetDay();
	month = oTime.GetMonth();
	year = oTime.GetYear();
	
	char sensorString[128];
	int status = -1;
	
	char str[100];
	char str2[100];
	
	LastMinute=minute;
	LastHour=hour;
	LastDay=day;
	LastMonth=month;
	LastYear=year;

	while (1)
	{
		AliveRefresher++;
			
		ioctl(watchdogDeviceHandle, WDIOC_KEEPALIVE, 0);
		
		minute = oTime.GetMinute();
		
		//TimeDelayFlag For MainTask
		if(TimeOutForMainTask > 0)TimeOutForMainTask--;
	
		//Dakikada bir kere yapılacak işler:
		if(LastMinute != minute)
		{			
			oTime.UpdateCurrentTime();
			cout << "[thread refresher] "<< oTime.timeStr <<" Fresh Minute..." << endl << flush;			
			
			hour = oTime.GetHour();
			day = oTime.GetDay();
			month = oTime.GetMonth();
			year = oTime.GetYear();
			
			//15 dk'de bir yapılacak işler:
			if((minute % 15) == 0)
			{
				TimeOutForMainTask = RandomTimeDelayForMainTask;
			}
			
			//Check Local Ip Is Up			
			sprintf(cmd,"sudo ifconfig %s | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}' > lanip",Config.NetworkDevice);
			system(cmd);
			
			strcpy(Config.LocalIP,"");
			if(GetStringFromFile("lanip",Config.LocalIP))
			{
				RemoveNewLineFromString(Config.LocalIP);
				
				//If No Ip Gotten then restart network device
				if(isValidIpAddress(Config.LocalIP)==false)
				{
					strcpy(Config.LocalIP,"0.0.0.0");
					
					cout << "[thread refresher] Local Network Error..." << Config.NetworkDevice <<endl << flush;
					
					oTime.UpdateCurrentTime();
					oMysql.AddNote("LocalNetwork","Error",Config.NetworkDevice,oTime.timeStr);
					
					sprintf(cmd,"sudo ifdown %s0",Config.NetworkDevice);
					system(cmd);
					
					sleep(1);
					
					sprintf(cmd,"sudo ifup %s0",Config.NetworkDevice);
					system(cmd);
					
					sleep(1);
					
					printf("[Lan IP Address] %s %s\n",oTime.timeStr,Config.LocalIP);
				}
				//else
				//	printf("[Lan IP Address] Error %s\n",Config.LocalIP);								
			}	
			
			internetStatus = CheckInternet();
			
			if(Version_MCU=="")
			{
				//MCU_Request_Version();
			}
			
			hour = oTime.GetHour();
			LastMinute = minute;
			LastHour=hour;
			LastDay=day;
			LastMonth=month;
			LastYear=year;
		}
	
		//Saniyede 1 yapılacak işler:
		//Blue Led
		//LED status change saniyede bir defa yanip sonme
		if(internetStatus)
		{
			//cout << "Internet Ok..." << endl;
			if(!strcmp(blueLedStatus,"1"))
			{
				strcpy(blueLedStatus,"0");
				gpioBlueLed->setval_gpio("1");
			}
			else
			{
				strcpy(blueLedStatus,"1");
				gpioBlueLed->setval_gpio("0");
			}
		}
		else
		{
			//cout << "Internet Error..." << endl;
			gpioBlueLed->setval_gpio("0");
		}
		
		// Check UPS State
		UPS_Check();
		if(UPSState==0 && UPSConnected==1)
		{ 
			oTime.UpdateCurrentTime();
			oMysql.AddNote("Shutting Down","","0",oTime.timeStr);
		
			cout << "SYSTEM SHUTTING DOWN..." << endl;
			
			//sprintf(cmd,"sudo shutdown -h now");
			//system(cmd);	
			
			//exit(0);
		}
		
		//15 dk'de bir yapılacak işler:		
		if(TimeOutForMainTask==0)
		{
			//Get This Flag To Wait State for another 15 minutes.
			TimeOutForMainTask = -1;
			
			if(!strcmp(Config.devicemode,"heatmap"))
			{
				cout << "[HEATMAP DataBase_GetMissedMainServerSensors] Checking Missed MainService Datas... Maintask" <<endl << flush;
				Heatmap_Synchronise_LocalandCloud(Config.MACAddress);
			}			
			
			oTime.UpdateCurrentTime();
			cout << "[thread refresher] Device Registering... " << oTime.timeStr <<endl << flush;							

			//Update System Properties like CPUFreq, Temp, Disk Usage 
			UpdateSystemProperties();

			//Registration Routine
			{
				CurrentAttempt=0;
				while(CurrentAttempt < MAXATTEMPTS)
				{
					//Try To Register Device to Remote Server
					if(MainServiceRegistration())
					{
						//if successfull break...
						break;
					}
					else
					{
						//if failed note it to LOG then re-try.
						oTime.UpdateCurrentTime();
						oMysql.AddNote("MainServiceRegister","Error","1",oTime.timeStr);
					}		

					//Sleep 1 sec in order to reduce network traffic
					sleep(2);
					
					//Increase Current Attempt
					CurrentAttempt++;
				}
			}
			
			if(strcmp(Config.operationmode,"master") == 0)
			{				
				if(minute < 15)
				{
					cout << "[thread refresher] Sending ACK To Sensors... " << oTime.timeStr <<endl << flush;						
					
					if(hour == 0)
						MCU_Request_Reset(year,month,day,23);
					else
						MCU_Request_Reset(year,month,day,hour-1);
				}
									
				//UPDATE CLOUD DB CELL IF NEEDED
				if( (openingHour <= hour && hour < closingHour) || (hour == closingHour && minute < 15) )
				{
					char response[2000];
					int dummyHour=0;
					
					//Combine All sensors to a string.
					//sprintf(sensorString,"%s",UpdateSensorString());
					
					//saat başı olduğunda güncel saati değil bir önceki saatin versini göndermeliyiz. 
					if(minute < 15) dummyHour = hour - 1; else dummyHour = hour;
					if(dummyHour == -1) dummyHour = 23;
					
					//localdb'deki veriyi oku.
					oTime.UpdateCurrentTime();
					cout<<"[Refresher] "<< oTime.timeStr <<" Mutex Locking..."<<endl;
					
					//pthread_mutex_lock(&lock);
					int mutexresponse=0;

					mutexresponse = pthread_mutex_lock(&lock);
					if(mutexresponse==0)
					{
						CurrentAttempt = 0;
						while(CurrentAttempt < MAXATTEMPTS)
						{
							status = oMysql.ReadCell(response, year, month, day, dummyHour);
							oTime.UpdateCurrentTime();
							
							if(status < 0)				
							{
								//Bir bağlantı hatası veya hiç beklenmedik bir durum oluştu. 								
								sprintf(response,"0.0/0.0/0.0/0.0/0.0/0.0/0.0/0.0");
								oMysql.AddNote("ReadLocalDb","Error","1",oTime.timeStr);
								cout << "[thread refresher] Read Local Database ERROR..." << endl << flush;								
								sleep(1);
								CurrentAttempt++;
								
								/**ToDo: LocalDB den okuma yapilamadigi durumlar icin SYNC to CloudDB icin 
								* ozel bir yapi tasarlanacak
								*/
							}
							else if(status > 0)
							{
								//Bağlantı başarılı fakat databasede istenilen formatta veri bulunamadı. O yüzden veriyi istediğimiz şekle sokup devam ediyoruz. 
								sprintf(response,"0.0/0.0/0.0/0.0/0.0/0.0/0.0/0.0");
								oMysql.AddNote("ReadLocalDb","empty_data","0",oTime.timeStr);
								break;
							}
							else 
							{
								//Bağlantı başarılı ve istenilen formatta veri geldi. 
								break;
							}
						}
						
						//saat başında sensör verilerini reorganize etmeliyiz. v2.6.1'de eklendi.
						if(minute < 15)
						{
							sprintf(str,"%s",response);		
							sprintf(str2,"%04d-%02d-%02d %02d",year,month,day,dummyHour);							
							oMysql.AddNote("RawData",str,"0",str2);		
	
							//Organize Sensor Datas v2.6.1'de eklendi.
							ReOrganizeSensors(response);
							
							//Yeni verileri localdb note tablosuna yazalım. 
							//Eğer mutech.net'e gönderebilirsek flagı 0 dan 1 e çevirmeliyiz.
							//0 data yeni demek. 1 olması için Mutech.nete yazmalıyız. 
							sprintf(str,"%s",response);		
							sprintf(str2,"%04d-%02d-%02d %02d",year,month,day,dummyHour);									
							oMysql.AddNote("ReOrganized",str,"0",str2);		
							
							//Yeni Verileri Local DB'ye yaz. v2.6.1'de eklendi.
							oMysql.WriteCell(year, month, day, dummyHour,response);				
						
						}
						
						pthread_mutex_unlock(&lock);
					}
					else
					{
						oTime.UpdateCurrentTime();
						oMysql.AddNote("MutexError","Refresher","1",oTime.timeStr);	
						cout << "[Thread Refresher] Mutex ERROR:" << mutexresponse <<endl<<flush;
					}
				
					oTime.UpdateCurrentTime();
					cout<<"[Refresher] "<< oTime.timeStr <<" Mutex UnLocking..."<<endl;
					
					//localDB'den okuduğun (response) veriyi CloudDB ye  gönder.
					CurrentAttempt = 0;
					while(CurrentAttempt < MAXATTEMPTS)
					{
						//if(oMysqlMainService.MainServiceWriteCell(Config.MACAddress,year,month,day,dummyHour,response))
						if(MainServiceUpdateData(year,month,day,dummyHour,response))
						{
							cout << "[thread refresher] Update Cell Completed." << endl << flush;

							pthread_mutex_lock(&lock);
							//if mainservice update successfull then update sensor status to 1  v2.6.1'de eklendi.
							oMysql.DataBase_ChangeLogStatusOfSensor(year,month,day,dummyHour);
							pthread_mutex_unlock(&lock);
							
							break;
						}
						else
						{
							oTime.UpdateCurrentTime();
							oMysql.AddNote("MainServiceUpdate","Error","1",oTime.timeStr);
							cout << "[thread refresher] Update Cell Not Completed." << endl << flush;
							sleep(1);
							CurrentAttempt++;
							/**ToDo: LocalDB den okuma yapilamadigi durumlar icin SYNC to CloudDB icin 
							* ozel bir yapi tasarlanacak
							*/								
						}	
					}				
				}
				
				if(minute<15)
				{
						//saat başı mutech.net ile localdeki datalardakini eşitlemeye çalış eğer varsa. 
						pthread_mutex_lock(&lock);
						DataBase_GetMissedMainServerSensors();	
						pthread_mutex_unlock(&lock);
				}
				
				//IF DEVICE REGISTERED; SEND FTP TO CUSTOMER FTPHOST ADDRESS
				if(Config.Registration)
				{
					if (strcmp(Config.ftphost, "") != 0)
					{				
						cout << "[SENDING FTP FILE]" << endl << flush;
				
						char table[16];		
						sprintf(table,"%04d%02d",year,month);		
						sprintf(cmd,"sudo rm /home/pi/xCounter/%s.csv",table);
						system(cmd);	 	 	
						
						oMysql.TableToCSV("/home/pi/xCounter/",table);	
						cout << "[CSV Table Created.]" << endl << flush;	
						
						sprintf(cmd,"curl -m 10 -T /home/pi/xCounter/%s.csv %s/xCounter/%s/ --user %s:%s --ftp-create-dirs",table,Config.ftphost,Config.MACAddress,Config.ftpuser,Config.ftppassword);
						system(cmd);	
						cout << "[FTP Sending Command:]" << endl << flush;	
						cout << cmd << endl << flush;	
						
						cout << "[Completed FTP Process]" << endl << flush;								
					}
				}
				else
				{
					cout << "[SENDING FTP FILE] Cancelled due to registration issues." << endl << flush;
				}		
			}
			
			ClearSensorStatus();
		}
		
		//Wait 1 Sec.
		sleep(1);	
		
		//cout<<"Sending Serial Port Message" << endl;
		SerialPort.Write(transmitDataPack.buffer, transmitDataPack.fullLen);
		
	}
}
//
//SERIAL PORT RECEIVER
//
void* UartReceiver(void *ptr)
{
	int index=0;
	while (1)
	{
	
		bool result;
		unsigned char byte;

		result = SerialPort.Read(&byte,1);

		if(result)
		{
			if(SerialPacketRx.Process(byte))
			{
				//printf("UART: %d %d %d %d %d\n",SerialPacketRx.RxBuffer[0],SerialPacketRx.RxBuffer[1],SerialPacketRx.RxBuffer[2],SerialPacketRx.RxBuffer[3],SerialPacketRx.RxBuffer[4]);
				index=8;
				
				if(
				SerialPacketRx.RxBuffer[3] == 'C' &&
				SerialPacketRx.RxBuffer[4] == 'O' &&
				SerialPacketRx.RxBuffer[5] == 'U' &&
				SerialPacketRx.RxBuffer[6] == 'N' &&
				SerialPacketRx.RxBuffer[7] == 'T'
				)
				{													
                                    for(int i=0;i<3;i++)
                                    {
                                    
                                        Sensor[i].In = (SerialPacketRx.RxBuffer[index++]<<8);
                                        Sensor[i].In += SerialPacketRx.RxBuffer[index++];
                                        
                                        Sensor[i].Out = (SerialPacketRx.RxBuffer[index++]<<8);
                                        Sensor[i].Out += SerialPacketRx.RxBuffer[index++];				
                                        
                                    }
				}
				
				else if(
				SerialPacketRx.RxBuffer[3] == 'A' &&
				SerialPacketRx.RxBuffer[4] == 'C' &&
				SerialPacketRx.RxBuffer[5] == 'K' &&
				SerialPacketRx.RxBuffer[6] == 'O' &&
				SerialPacketRx.RxBuffer[7] == 'K'
				)
				{
					cout << "[thread UartReceiver] Acknowledge Recieved." << endl << flush;
				}
				
				else if(
				SerialPacketRx.RxBuffer[3] == 'V' &&
				SerialPacketRx.RxBuffer[4] == 'E' &&
				SerialPacketRx.RxBuffer[5] == 'R' &&
				SerialPacketRx.RxBuffer[6] == 'S' &&
				SerialPacketRx.RxBuffer[7] == 'N'
				)
				{
					sprintf(Version_MCU,"%d.%d.%d.%d r%d%c",
					SerialPacketRx.RxBuffer[8],
					SerialPacketRx.RxBuffer[9],
					SerialPacketRx.RxBuffer[10],
					SerialPacketRx.RxBuffer[11],
					SerialPacketRx.RxBuffer[12],
					SerialPacketRx.RxBuffer[13]					
					);
					
					cout << "[thread UartReceiver] VERSION Recieved:" << Version_MCU << endl << flush;
				}
				else if(
				SerialPacketRx.RxBuffer[3] == 'G' &&
				SerialPacketRx.RxBuffer[4] == 'T' &&
				SerialPacketRx.RxBuffer[5] == 'I' &&
				SerialPacketRx.RxBuffer[6] == 'M' &&
				SerialPacketRx.RxBuffer[7] == 'E'
				)
				{
					printf("[thread UartReceiver] RTC Time: %02d.%02d:%02d",
					SerialPacketRx.RxBuffer[8],
					SerialPacketRx.RxBuffer[9],
					SerialPacketRx.RxBuffer[10]);
					cout << "" << endl << flush;
					
				}
				else if(
				SerialPacketRx.RxBuffer[3] == 'G' &&
				SerialPacketRx.RxBuffer[4] == 'D' &&
				SerialPacketRx.RxBuffer[5] == 'A' &&
				SerialPacketRx.RxBuffer[6] == 'T' &&
				SerialPacketRx.RxBuffer[7] == 'E'
				)
				{
					printf("[thread UartReceiver] RTC Date: %04d/%02d/%02d",
					(SerialPacketRx.RxBuffer[8]<<8) + SerialPacketRx.RxBuffer[9],
					SerialPacketRx.RxBuffer[10],
					SerialPacketRx.RxBuffer[11]);
					cout << "" << endl << flush;
					
				}
				else
				{
					printf("[thread UartReceiver] ");
					
					for(int i=0;i<SerialPacketRx.Length+3;i++)
					{
						printf("%c(%d)",SerialPacketRx.RxBuffer[i],SerialPacketRx.RxBuffer[i]);
					}
					cout << "" << endl << flush;
				}
			}
		}
		else
		{
			printf("\nSerial Read Error\n");
			usleep(100000);
		}
		
		AliveUartReceiver++;		
	}
	return 0;
}

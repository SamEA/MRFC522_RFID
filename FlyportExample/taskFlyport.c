
#include "taskFlyport.h"
#include "rc522.h"


void RFID_Init(void)
{
  MRFC522_Reset();
  MRFC522_AntennaOff();  
  MRFC522_AntennaOn();
  MRFC522_ConfigISOType( 'A' );
}
 

void checkCard(void)
{
 
  char status;
  char buf[16];
  PcdReset();
  status=PcdRequest(PICC_REQIDL,&RevBuffer[0]);  //Search card, return card types
  if (status==MI_OK){
  UARTWrite(1,"Card Detected: ");
   sprintf(buf, "%u\r\n", RevBuffer[0]);
	UARTWrite(1,buf);
  }
  else if(status!=MI_OK)
  {
	 // UARTWrite(1,"Status error ");
    return;
  }
 			
  status=PcdAnticoll(&RevBuffer[2]);
	if (status==MI_OK){
		UARTWrite(1,"The card's number is  :");
   sprintf(buf, "%u\r\n", RevBuffer[2]);
	UARTWrite(1,buf);
	sprintf(buf, "%u\r\n", RevBuffer[3]);
	UARTWrite(1,buf);
	 sprintf(buf, "%u\r\n", RevBuffer[4]);
	UARTWrite(1,buf);
	sprintf(buf, "%u\r\n", RevBuffer[5]);
	UARTWrite(1,buf);
	 if(RevBuffer[2] == 86 && RevBuffer[3] == 195 && RevBuffer[4] == 112 && RevBuffer[5] == 164) {
                         UARTWrite(1,"Hello Sam ");
                        } else if(RevBuffer[2] == 221 && RevBuffer[3] == 132 && RevBuffer[4] == 142 && RevBuffer[5] == 54)) {
                          UARTWrite(1,"Hello Jack ");
                        }
	}
  if(status!=MI_OK)
  {
    return;
  }
  
}

 void FlyportTask()
 {
	SPI2Init();
	DelayMs(100);
	RFID_Init();
	
	WFConnect(WF_DEFAULT);
	while (WFStatus != CONNECTED);
	UARTWrite(1,"Flyport connected... hello world!\r\n");

  DelayMs(50);
	while(1)
	{
		IOPut (p21,on); //Led  On
		DelayMs(500);
	  checkCard();
    IOPut (p21,off); //Led  Off
	  DelayMs(50);
	}
}



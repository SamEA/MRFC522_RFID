


#include "spi.h"
#include "taskFlyport.h"
#include "rc522.h"
#define HARWARE_SPI


#define SET_SPI_CS  IOPut(o3,on);
#define CLR_SPI_CS  IOPut(o3,off);
#define SET_RC522RST  IOPut(o4,on);
#define CLR_RC522RST  IOPut(o4,off);

void ClearBitMask(UCHAR8 reg,UCHAR8 mask);
void WriteRawRC(UCHAR8 Address, UCHAR8 value);
void SetBitMask(UCHAR8 reg,UCHAR8 mask);
char PcdComMF522(UCHAR8 Command, 
                 UCHAR8 *pInData, 
                 UCHAR8 InLenByte,
                 UCHAR8 *pOutData, 
                 UINT  *pOutLenBit);
void CalulateCRC(UCHAR8 *pIndata,UCHAR8 len,UCHAR8 *pOutData);
UCHAR8 ReadRawRC(UCHAR8 Address);
void MFRC522_AntennaOn(void);

    /*********************************************************************
    * Function:        void delay_ns(UINT16 ns)
    *
    * PreCondition:     none
    *
    * Input:		    UINT16 ns - nanoseconds to delay               
    *
    * Output:		    nothing
    *    
    * Side Effects:	    none
    *
    * Overview:		    This function delay 
    *
    * Note:			    it is not precise and needs to be fixed
    ********************************************************************/
void delay_ns(UINT16 ns)
{
  UINT i;
  for(i=0;i<ns;i++)
  {
    asm volatile ("nop" :: );
  }
}

   /*********************************************************************
    * Function:        void SPI2Init(void)
    *
    * PreCondition:     none
    *
    * Input:		    none
    *
    * Output:		    none - write "Spi 2 initialized." on the console
    *    
    * Side Effects:	    none
    *
    * Overview:		    This function initialize the SPI 2
    *
    * Note:			    p4 is remapped as SPI_OUT
    *               p7 is remapped as SPI_IN
    *               p6 is remapped as SPICLKOUT
    *               if you are using other pins remap them properly    
    ********************************************************************/
 void SPI2Init(void){
    IOInit(p4,SPI_OUT);    // Remap the p4 pin as SPI_OUT
	  IOInit(p7,SPI_IN);     // Remap the p7 pin as SPI_IN
	  IOInit(p6,SPICLKOUT);  // Remap the p6 pin as SPI CLK
    SPI2STAT = 0;
    // Initialize SPI2CON1 and SPI2CON2.
 
    // Clear all bits.
    SPI2CON1 = SPI2CON2 = 0;
    // Set the master mode.
    SPI2CON1bits.MSTEN = 1;
    // Set the prescalers.
    // 00 (0) - 64:1
    // 01 (1) - 16:1
    // 10 (2) -  4:1
    // 11 (3) -  1:1 
    SPI2CON1bits.PPRE = 3;
    
    // Set the secondary prescaler.
    // Secondary prescaler options:
    //
    // 000 (0) -  8:1
    // 001 (1) -  7:1
    // 010 (2) -  6:1
    // 011 (3) -  5:1
    // 100 (4) -  4:1
    // 101 (5) -  3:1
    // 110 (6) -  2:1 <-- selecting 2:1 on the seconday prescaler
    // 111 (7) -  1:1   
    SPI2CON1bits.SPRE = 6;
     SPI2CON1bits.CKP = 1;
    // Reset the receiver overflow flag.
    SPI2STATbits.SPIROV = 0;
    // Enable the spi.
    SPI2STATbits.SPIEN = 1;
    // Debug.   
    UARTWrite(1, "Spi 2 initialized.\r\n");
}

   /*********************************************************************
    * Function:         BYTE SPIWriteByte(BYTE v)
    *
    * PreCondition:     SPI has been configured 
    *
    * Input:		    BYTE v - the byte to send over the SPI
    *
    * Output:		    none
    *
    * Side Effects:	    none
    *
    * Overview:		    This function will send a byte over the SPI
    *
    * Note:			    this function use the hardware SPI,  
    *               for bit banging SPI uncomment the #if statement and 
    *               define SPI_SDO2, SPISCK2 and SPI_SDI2  properly      
    ********************************************************************/
void SPIWriteByte(BYTE v)
    {
        BYTE i;
        /* 
        #if !defined(HARDWARE_SPI)
            
            SPI_SDO2 = 0;
            SPI_SCK2 = 0;
            
            for(i = 0; i < 8; i++)
            {
                SPI_SDO2 = (v >> (7-i));      
                SPI_SCK2 = 1;
                SPI_SCK2 = 0;  
            }  
            SPI_SDO2 = 0; 
        #else
        */
                IFS2bits.SPI2IF = 0;
                i = SPI2BUF;
                SPI2BUF = v;
                while(IFS2bits.SPI2IF == 0){}
      //  #endif
    }
    
    
    /*********************************************************************
    * Function:         BYTE SPIReadByte(void)
    *
    * PreCondition:     SPI has been configured 
    *
    * Input:		    none
    *
    * Output:		    BYTE - the byte that was last received by the SPI
    *
    * Side Effects:	    none
    *
    * Overview:		    This function will read a byte over the SPI
    *
    * Note:			    this function use the hardware SPI,  
    *               for bit banging SPI uncomment the #if statement and 
    *               define SPI_SDO2, SPISCK2 and SPI_SDI2 properly
    ********************************************************************/
    BYTE SPIReadByte(void)
    {
       /* #if !defined(HARDWARE_SPI)
            BYTE i;
            BYTE spidata = 0;
    
            SPI_SDO2 = 0;
            SPI_SCK2 = 0;
            
            for(i = 0; i < 8; i++)
            {
                spidata = (spidata << 1) | SPI_SDI2;  
                SPI_SCK2 = 1;
                SPI_SCK2 = 0; 
            }
            
            return spidata;
        #else */
                SPIWriteByte(0x00);
                return SPI2BUF;
            
      //  #endif
    }   

   /*********************************************************************
    * Function:        char MFRC522_Request(UCHAR8 req_code,UCHAR8 *pTagType)
    *
    * PreCondition:     SPI has been configured 
    *
    * Input:		    UCHAR8 req_code - 
    *               UCHAR8 *pTagType - 
    *
    * Output:		    char - return MI_OK if success
    *               pTagType - return card type 
    *                                           0x4400 = Mifare_UltraLight       
    *                                           0x0400 = Mifare_One(S50)
    *                                           0x0200 = Mifare_One(S70)
    *                                           0x0800 = Mifare_Pro(X)
    *                                           0x4403 = Mifare_DESFire
    *    
    * Side Effects:	    none
    *
    * Overview:		    This function search card and return card types
    *
    * Note:			    None
    ********************************************************************/
char MFRC522_Request(UCHAR8 req_code,UCHAR8 *pTagType)
{
	char status;  
	UINT unLen;
	UCHAR8 ucComMF522Buf[MAXRLEN]; 

	ClearBitMask(Status2Reg,0x08);
	WriteRawRC(BitFramingReg,0x07);
	SetBitMask(TxControlReg,0x03);
 
	ucComMF522Buf[0] = req_code;

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);

	if ((status == MI_OK) && (unLen == 0x10))
	{    
		*pTagType     = ucComMF522Buf[0];
		*(pTagType+1) = ucComMF522Buf[1];
	}
	else
	{   status = MI_ERR;   }
   
	return status;
}
 
  /*********************************************************************
    * Function:        char MFRC522_Anticoll(UCHAR8 *pSnr)
    *
    * PreCondition:     none 
    *
    * Input:		    UCHAR8 *pSnr  
    *
    * Output:		    return MI_OK if success
    *               return the 4 bytes serial number     
    *    
    * Side Effects:	    none
    *
    * Overview:		    This function prevent conflict and return the 4 bytes serial number
    *
    * Note:			    None
    ********************************************************************/
char MFRC522_Anticoll(UCHAR8 *pSnr)
{
    char status;
    UCHAR8 i,snr_check=0;
    UINT unLen;
    UCHAR8 ucComMF522Buf[MAXRLEN]; 
    

    ClearBitMask(Status2Reg,0x08);
    WriteRawRC(BitFramingReg,0x00);
    ClearBitMask(CollReg,0x80);
 
    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x20;

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2,ucComMF522Buf,&unLen);

    if (status == MI_OK)
    {
    	 for (i=0; i<4; i++)
         {   
             *(pSnr+i)  = ucComMF522Buf[i];
             snr_check ^= ucComMF522Buf[i];
         }
         if (snr_check != ucComMF522Buf[i])
         {   status = MI_ERR;    }
    }
    
    SetBitMask(CollReg,0x80);
    return status;
}

/*********************************************************************
    * Function:       char MFRC522_Select(UCHAR8 *pSnr)
    *
    * PreCondition:     SPI has been configured 
    *
    * Input:		    UCHAR8 *pSnr
    *
    * Output:		    char - return MI_OK if success
    *               
    *    
    * Side Effects:	    none
    *
    * Overview:		    
    *
    * Note:			    None
    ********************************************************************/
char MFRC522_Select(UCHAR8 *pSnr)
{
    char status;
    UCHAR8 i;
    UINT unLen;
    UCHAR8 ucComMF522Buf[MAXRLEN]; 
    
    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x70;
    ucComMF522Buf[6] = 0;
    for (i=0; i<4; i++)
    {
    	ucComMF522Buf[i+2] = *(pSnr+i);
    	ucComMF522Buf[6]  ^= *(pSnr+i);
    }
    CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);
  
    ClearBitMask(Status2Reg,0x08);

    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);
    
    if ((status == MI_OK) && (unLen == 0x18))
    {   status = MI_OK;  }
    else
    {   status = MI_ERR;    }

    return status;
}

 /*********************************************************************
    * Function:   char MFRC522_AuthState(UCHAR8 auth_mode,UCHAR8 addr,UCHAR8 *pKey,UCHAR8 *pSnr)
    *
    * PreCondition:     none 
    *
    * Input:		    UCHAR8 auth_mode -   Password Authentication Mode
    *                                        0x60 = A key authentication
                                             0x61 = B key authentication    
    *               UCHAR8 addr  -      Block Address
    *               UCHAR8 *pKey  -     Sector Password
    *               UCHAR8 *pSnr  -    4 bytes serial number
    *
    * Output:		    char - return MI_OK if success
    *               
    *    
    * Side Effects:	    none
    *
    * Overview:		    This function verify card password
    *
    * Note:			    None
    ********************************************************************/             
char MFRC522_AuthState(UCHAR8 auth_mode,UCHAR8 addr,UCHAR8 *pKey,UCHAR8 *pSnr)
{
    char status;
    UINT unLen;
    UCHAR8 i,ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = auth_mode;
    ucComMF522Buf[1] = addr;
    for (i=0; i<6; i++)
    {    ucComMF522Buf[i+2] = *(pKey+i);   }
    for (i=0; i<6; i++)
    {    ucComMF522Buf[i+8] = *(pSnr+i);   }
    //memcpy(&ucComMF522Buf[2], pKey, 6); 
    //memcpy(&ucComMF522Buf[8], pSnr, 4); 
    
    status = PcdComMF522(PCD_AUTHENT,ucComMF522Buf,12,ucComMF522Buf,&unLen);
    if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08)))
    {   status = MI_ERR;   }
    
    return status;
}

 /*********************************************************************
    * Function:        char MFRC522_Read(UCHAR8 addr, UCHAR8 *pData)
    *
    * PreCondition:     none
    *
    * Input:		    UCHAR8 addr   - block address
    *               UCHAR8 *pData  - block data
    *
    * Output:		    char - return MI_OK if success
    *    
    * Side Effects:	    none
    *
    * Overview:		    This function read block data
    *
    * Note:			    None
    ********************************************************************/
char MFRC522_Read(UCHAR8 addr, UCHAR8 *pData)
{
    char  status;
    UINT unLen;
    UCHAR8 i,ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = PICC_READ;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
   
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
    if ((status == MI_OK) && (unLen == 0x90))
   // {   memcpy(pData, ucComMF522Buf, 16);   }
    {
        for (i=0; i<16; i++)
        {    *(pData+i) = ucComMF522Buf[i];   }
    }
    else
    {   status = MI_ERR;   }
    
    return status;
}

 /*********************************************************************
    * Function:      char MFRC522_Write(UCHAR8 addr,UCHAR8 *pData)
    *
    * PreCondition:     SPI has been configured 
    *
    * Input:		    UCHAR8 addr  - block address
    *               UCHAR8 *pData  - data to write
    *
    * Output:		    char - return MI_OK if success
    *                                           
    *    
    * Side Effects:	    none
    *
    * Overview:		    This function write a block of data to addr
    *
    * Note:			    None
    ********************************************************************/               
char MFRC522_Write(UCHAR8 addr,UCHAR8 *pData)
{
    UCHAR8 status;
    UINT unLen;
    UCHAR8 i,ucComMF522Buf[MAXRLEN]; 
    
    ucComMF522Buf[0] = PICC_WRITE;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }
        
    if (status == MI_OK)
    {
       // memcpy(ucComMF522Buf, pData, 16);
        for (i=0; i<16; i++)
        {    
        	ucComMF522Buf[i] = *(pData+i);   
        }
        CalulateCRC(ucComMF522Buf,16,&ucComMF522Buf[16]);

        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,18,ucComMF522Buf,&unLen);
        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        {   status = MI_ERR;   }
    }
    
    return status;
}

/*
 * Function：MFRC522_Halt
 * Description：Command the cards into sleep mode
 * Input parameters：null
 * return：MI_OK
 */
 /*********************************************************************
    * Function:       char MFRC522_Halt(void)
    *
    * PreCondition:     none
    *
    * Input:		    none 
    *
    * Output:		    none
    *    
    * Side Effects:	    none
    *
    * Overview:		    This function command the cards into sleep mode
    *
    * Note:			    None
    ********************************************************************/
char MFRC522_Halt(void)
{
    char status;
    UINT unLen;
    UCHAR8 ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = PICC_HALT;
    ucComMF522Buf[1] = 0;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    return MI_OK;
}

 /*********************************************************************
    * Function:   void CalulateCRC(UCHAR8 *pIndata,UCHAR8 len,UCHAR8 *pOutData)
    *
    * PreCondition:     none
    *
    * Input:		    UCHAR8 *pIndata - input datas
    *               UCHAR8 len       - data length
    *               UCHAR8 *pOutData  - output data
    *
    * Output:		    UCHAR8 - 2 bytes CRC result
    *    
    * Side Effects:	    none
    *
    * Overview:		    This function calculate the CRC
    *
    * Note:			    None
    ********************************************************************/
void CalulateCRC(UCHAR8 *pIndata,UCHAR8 len,UCHAR8 *pOutData)
{
    UCHAR8 i,n;
    ClearBitMask(DivIrqReg,0x04);
    WriteRawRC(CommandReg,PCD_IDLE);
    SetBitMask(FIFOLevelReg,0x80);
    for (i=0; i<len; i++)
    {   WriteRawRC(FIFODataReg, *(pIndata+i));   }
    WriteRawRC(CommandReg, PCD_CALCCRC);
    i = 0xFF;
    do 
    {
        n = ReadRawRC(DivIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x04));
    pOutData[0] = ReadRawRC(CRCResultRegL);
    pOutData[1] = ReadRawRC(CRCResultRegM);
}

/*********************************************************************
    * Function:        char MFRC522_Reset(void)
    *
    * PreCondition:     none
    *
    * Input:		    none 
    *
    * Output:		    return MI_OK
    *    
    * Side Effects:	    reset the RC522
    *
    * Overview:		    This function reset the RC522
    *
    * Note:			    None
    ********************************************************************/
char MFRC522_Reset(void)
{
  	SET_RC522RST;
    delay_ns(10);
	  CLR_RC522RST;
    delay_ns(10);
	  SET_RC522RST;
    delay_ns(10);
    WriteRawRC(CommandReg,PCD_RESETPHASE);
    delay_ns(10);
    WriteRawRC(ModeReg,0x3D);           
    WriteRawRC(TReloadRegL,30);           
    WriteRawRC(TReloadRegH,0);
    WriteRawRC(TModeReg,0x8D);
    WriteRawRC(TPrescalerReg,0x3E);
	  WriteRawRC(TxAutoReg,0x40);
    return MI_OK;
}
   /*********************************************************************
    * Function:        char MFRC522_ConfigISOType(UCHAR8 type)
    *
    * PreCondition:     none
    *
    * Input:		    UCHAR8 type
    *
    * Output:		  return MI_OK if type == 'A' 
    *          
    * Side Effects:	    none
    *
    * Overview:		    This function configure the ISO type
    *
    * Note:			    None
    ********************************************************************/
char MFRC522_ConfigISOType(UCHAR8 type)
{
   if (type == 'A')                 
   { 
       ClearBitMask(Status2Reg,0x08);
       WriteRawRC(ModeReg,0x3D);
       WriteRawRC(RxSelReg,0x86);
       WriteRawRC(RFCfgReg,0x7F);
   	   WriteRawRC(TReloadRegL,30); 
	     WriteRawRC(TReloadRegH,0);
       WriteRawRC(TModeReg,0x8D);
	     WriteRawRC(TPrescalerReg,0x3E);
	     delay_ns(1000);
       MFRC522_AntennaOn();
   }
   else{ return -1; }
   
   return MI_OK;
}
   /*********************************************************************
    * Function:        unsigned char ReadRawRC(UCHAR8 Address)
    *
    * PreCondition:     SPI has been configured 
    *
    * Input:		    UCHAR8 Address - register address
    *
    * Output:		    return a byte of data read from the register
    *    
    * Side Effects:	    none
    *
    * Overview:		    This function read a byte of data register
    *
    * Note:			    None
    ********************************************************************/
unsigned char ReadRawRC(UCHAR8 Address)
{
    UCHAR8 ucAddr;
    UCHAR8 ucResult=0;
	  CLR_SPI_CS;
    ucAddr = ((Address<<1)&0x7E)|0x80;
   	SPIWriteByte(ucAddr);
	  ucResult=SPIReadByte();
	  SET_SPI_CS;
    return ucResult;
}

   /*********************************************************************
    * Function:        void WriteRawRC(UCHAR8 Address, UCHAR8 value)
    *
    * PreCondition:     none 
    *
    * Input:		    UCHAR8 Address  -   register address
    *               UCHAR8 value    -    the value to be written
    *
    * Output:		    none 
    *                                      
    *    
    * Side Effects:	    none
    *
    * Overview:		    This function write a byte into a given register
    *
    * Note:			    None
    ********************************************************************/
void WriteRawRC(UCHAR8 Address, UCHAR8 value)
{  
    UCHAR8 ucAddr;

	  CLR_SPI_CS;
    ucAddr = ((Address<<1)&0x7E);

	  SPIWriteByte(ucAddr);
	  SPIWriteByte(value);
	  SET_SPI_CS;
}

   /*********************************************************************
    * Function:        void SetBitMask(UCHAR8 reg,UCHAR8 mask) 
    *
    * PreCondition:     none
    *
    * Input:		   UCHAR8 reg  - register address 
    *              UCHAR8 mask - bit mask
    *
    * Output:		    none
    *    
    * Side Effects:	    none
    *
    * Overview:		    This function set a bit mask on RC522 register
    *
    * Note:			    None
    ********************************************************************/
void SetBitMask(UCHAR8 reg,UCHAR8 mask)  
{
    char tmp = 0x0;
    tmp = ReadRawRC(reg);
    WriteRawRC(reg,tmp | mask);  // set bit mask
}

   /*********************************************************************
    * Function:        void ClearBitMask(UCHAR8 reg,UCHAR8 mask)
    *
    * PreCondition:     none
    *
    * Input:		    UCHAR8 reg - register address
    *               UCHAR8 mask - set value
    *
    * Output:		    none
    *    
    * Side Effects:	    none
    *
    * Overview:		    This function clear a bit mask on RC522 register
    *
    * Note:			    None
    ********************************************************************/
void ClearBitMask(UCHAR8 reg,UCHAR8 mask)  
{
    char  tmp = 0x0;
    tmp = ReadRawRC(reg);
    WriteRawRC(reg, tmp & ~mask);  // clear bit mask
} 

   /*********************************************************************
    * Function:        char PcdComMF522(UCHAR8 Command, 
    *                                    UCHAR8 *pInData, 
    *                                     UCHAR8 InLenByte, 
    *                                      UCHAR8 *pOutData, 
    *                                       UINT *pOutLenBit)
    *
    * PreCondition:     SPI has been configured 
    *
    * Input:		    UCHAR8 Command   -  command type
    *               UCHAR8 *pInData  -  input data
    *               UCHAR8 InLenByte -  input data length
    *               UCHAR8 *pOutData -  output Data
    *               UINT *pOutLenBit -  output data length
    *
    * Output:		    char - return MI_OK if success
    *               
    *    
    * Side Effects:	    none
    *
    * Overview:		    This function search card and return card types
    *
    * Note:			    None
    ********************************************************************/
char PcdComMF522(UCHAR8 Command, 
                 UCHAR8 *pInData, 
                 UCHAR8 InLenByte,
                 UCHAR8 *pOutData, 
                 UINT *pOutLenBit)
{
    char status = MI_ERR;
    UCHAR8 irqEn   = 0x00;
    UCHAR8 waitFor = 0x00;
    UCHAR8 lastBits;
    UCHAR8 n;
    UINT i;
    switch (Command)
    {
        case PCD_AUTHENT:
			irqEn   = 0x12;
			waitFor = 0x10;
			break;
		case PCD_TRANSCEIVE:
			irqEn   = 0x77;
			waitFor = 0x30;
			break;
		default:
			break;
    }
   
    WriteRawRC(ComIEnReg,irqEn|0x80);
    ClearBitMask(ComIrqReg,0x80);
    WriteRawRC(CommandReg,PCD_IDLE);
    SetBitMask(FIFOLevelReg,0x80);
    
    for (i=0; i<InLenByte; i++)
    {   WriteRawRC(FIFODataReg, pInData[i]);    }
    WriteRawRC(CommandReg, Command);
   
    
    if (Command == PCD_TRANSCEIVE)
    {    SetBitMask(BitFramingReg,0x80);  }
    
	i = 2000;
    do 
    {
        n = ReadRawRC(ComIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x01) && !(n&waitFor));
    ClearBitMask(BitFramingReg,0x80);

    if (i!=0)
    {    
        if(!(ReadRawRC(ErrorReg)&0x1B))
        {
            status = MI_OK;
            if (n & irqEn & 0x01)
            {   status = MI_NOTAGERR;   }
            if (Command == PCD_TRANSCEIVE)
            {
               	n = ReadRawRC(FIFOLevelReg);
              	lastBits = ReadRawRC(ControlReg) & 0x07;
                if (lastBits)
                {   *pOutLenBit = (n-1)*8 + lastBits;   }
                else
                {   *pOutLenBit = n*8;   }
                if (n == 0)
                {   n = 1;    }
                if (n > MAXRLEN)
                {   n = MAXRLEN;   }
                for (i=0; i<n; i++)
                {   pOutData[i] = ReadRawRC(FIFODataReg);    }
            }
        }
        else
        {   status = MI_ERR;   }
        
    }
    SetBitMask(ControlReg,0x80);           // stop timer now
    WriteRawRC(CommandReg,PCD_IDLE); 
    return status;
}

/*********************************************************************
    * Function:        void MFRC522_AntennaOn(void)
    *
    * PreCondition:     none
    *
    * Input:		    none
    *
    * Output:		    none
    *    
    * Side Effects:	    Antenna On
    *
    * Overview:	  	This function command the RC522 to switch on the antenna
    *
    * Note:			    None
    ********************************************************************/
void MFRC522_AntennaOn(void)
{
    UCHAR8 i;
    i = ReadRawRC(TxControlReg);
    if (!(i & 0x03))
    {
        SetBitMask(TxControlReg, 0x03);
    }
}

/*********************************************************************
    * Function:        void MFRC522_AntennaOff(void)
    *
    * PreCondition:     none 
    *
    * Input:		   none
    *
    * Output:		    none
    *    
    * Side Effects:	  Antenna Off
    *
    * Overview:		 This function command the RC522 to switch off the antenna
    *
    * Note:			    None
    ********************************************************************/
void MFRC522_AntennaOff(void)
{
	ClearBitMask(TxControlReg, 0x03);
}



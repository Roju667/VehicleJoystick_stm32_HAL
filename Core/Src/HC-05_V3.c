/*
 * HC-05.c
 *
 *  Created on: Dec 9, 2021
 *      Author: ROJEK
 */

/*
 * There is a lot of info about HC05 all around internet, but at the same time its not that easy to find how to properly make it work.
 * So after reading multiple websites , looking everywhere, i finally managed to connect it with cheap clone JDY09 that i also made lib for.
 *
 * This is for sure not the library that will work in all the cases and its not perfect,
 * but you can try to find something useful for you. I have a plan to use it for sending data to a simple bluetooth controlled vehicle
 *
 * For UART comm i use 38400 baud rate to communicate with AT MODE 1 (slow blinking)
 * I put command SetUart to 38400 to make all communications with HC05 at 38400. That takes away changing baud rate on the fly.
 *
 * For recieveing message i reccomend using DMA callbacks.
 *
 * if RX interrupt mode : enable USARTx Global Interrupt
 * if RX DMA mode : enable USARTx Global Interrupt, enable DMAx streamx global interrupt
 *
 * Default settings for DMA:
 * USARTx_RX
 * Mode: Normal
 * Peripheral to memory
 * Increment address of memory
 * Data width : byte
 *
 * if RX interrupt mode : put HC05_RxCpltCallbackIT in HAL_UART_RxCpltCallback in main.c
 * if RX DMA mode : put HC05_RxCpltCallbackIT in HAL_UARTEx_RxEventCallback in main.c
 *
 * To switch between AT Modes by software i put PWR pin on BJT because it was easier than clicking the button and restarting.
 *
 * Display Terminal is a useful function when you want to see what is actually coming on UART.
 * I connect it to Nucleo UART2 and then open it on console (RealTerm/Teraterm etc.)
 *
 * Biggest problem that i faced when i had no idea about this module :
 * - There are 2 AT Modes that is pain to swtich all the time
 * - CubeMX can generate UART Init before DMA Init that will stop it from working correctly
 * - Firmware version between v2,v3,v4 are very different so look for answer for yours (i worked on v3)
 * - Bluetooth address format that is described in ConnectToAddress function
 * - I wasnt getting uart DMA callback when inquiring, i reduced buffer size from 128 to 64 and it helped (logic analyzer helped)
 *
 *
 */

#include "usart.h"
#include "stdio.h"
#include "string.h"
#include "HC-05_V3.h"

/*
 * Terminal defined by user, commands sent in offline mode will be displayed
 * with responses from HC-05. Used for configuration/initialization
 *
 * @param[*Msg] - string to send to display terminal
 *
 * @return - void
 */
static void HC05_DisplayTerminal(char *Msg)
{
}

/*
 * Data exchange between MCU and HC05
 *
 * @param[*hc05] - pointer to struct for HC05 bluetooth module
 * @param[Command] - predefined command to send
 * @return - void
 */
static void HC05_SendAndReceiveCmd(HC05_t *hc05, uint8_t *Command)
{
	uint8_t MsgRecieved[HC05_RECIEVEBUFFERSIZE];

	//display send info on user display terminal
	HC05_DisplayTerminal("Sending: ");
	HC05_DisplayTerminal((char*) Command);

	//send data to HC05
	HAL_UART_Transmit(hc05->huart, Command, strlen((char*) Command), HC05_UART_TIMEOUT);

	uint32_t responsetime = HAL_GetTick();

	//wait for response line
	while (hc05->LinesRecieved == 0)
	{
		if (HAL_GetTick() - responsetime < HC05_UART_TIMEOUT)
		{
			// wait until timeout
		}
		else
		{
			HC05_DisplayTerminal("No response, UART communication error\n\r");
			return;
		}
	}

	HC05_DisplayTerminal("Response: ");
	while(hc05->LinesRecieved != 0)
	{
		//get message out of ring buffer
		HC05_CheckPendingMessages(hc05, MsgRecieved);
		//display response
		HC05_DisplayTerminal((char*) MsgRecieved);
	}

	//clear message pending flag
	HC05_ClearMsgPendingFlag(hc05);

}

/*
 * Init MCU ports that will be connected to EN pin and STATE pin
 *
 * @param[*hc05] - pointer to struct for HC05 bluetooth module
 * @param[Command] - predefined command to send
 * @return - void
 */
static void HC05_GPIO_Init(void)
{
	HC05_EN_CLK_ENABLE();
	HC05_STATE_CLK_ENABLE();

	GPIO_InitTypeDef gpio =
	{ 0 };
	gpio.Mode = GPIO_MODE_OUTPUT_PP; // OD = open drain
	gpio.Speed = GPIO_SPEED_FREQ_MEDIUM;
	gpio.Pin = (HC05_PWR_PIN);
	HAL_GPIO_Init(HC05_EN_PORT, &gpio);

	gpio.Pin = (HC05_EN_PIN);
	HAL_GPIO_Init(HC05_EN_PORT, &gpio);

	gpio.Pin = HC05_STATE_PIN;
	gpio.Mode = GPIO_MODE_INPUT;
	gpio.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(HC05_STATE_PORT, &gpio);

}


/*
 * Send info command to HC-05 in AT commands mode.
 *
 * @param[*hc05] - pointer to struct for HC05 bluetooth module
 * @param[Command] - predefined commands that are in .h file @infocommands
 *
 * @return - void
 */
void HC05_SendInfoCommand(HC05_t *hc05, HC05_INFO Command)
{

		switch (Command)
		{
	case HC05_INFO_AT:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT\r\n");
			break;

	case HC05_INFO_AT_VERSION:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+VERSION?\r\n");
			break;

	case HC05_INFO_AT_ADDR:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+ADDR?\r\n");
			break;

	case HC05_INFO_AT_NAME:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+NAME?\r\n");
			break;

	case HC05_INFO_AT_MODE:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+ROLE?\r\n");
			break;

	case HC05_INFO_AT_ROLE:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+CLASS?\r\n");
			break;

	case HC05_INFO_AT_IAC:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+IAC\r\n");
			break;

	case HC05_INFO_AT_INQM:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+INQM??\r\n");
			break;

	case HC05_INFO_AT_PSWD:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+PSWD??\r\n");
			break;

	case HC05_INFO_AT_UART:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+UART??\r\n");
			break;

	case HC05_INFO_AT_CMODE:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+CMODE?\r\n");
			break;

	case HC05_INFO_AT_BIND:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+BIND?\r\n");
			break;

	case HC05_INFO_AT_POLAR:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+POLAR?\r\n");
			break;

	case HC05_INFO_AT_IPSCAN:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+IPSCAN?\r\n");
			break;

	case HC05_INFO_AT_SNIFF:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+SNIFF?\r\n");
			break;

	case HC05_INFO_AT_SENM:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+SENM?\r\n");

	case HC05_INFO_AT_ADCN:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+ADCN?\r\n");
			break;

	case HC05_INFO_AT_MRAD:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+MRAD?\r\n");
			break;

	case HC05_INFO_AT_STATE:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+STATE?\r\n");
			break;

	case HC05_INFO_AT_HELP:
		HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+HELP\r\n");
		break;
		}
		return;
}

/*
 * Send info command to HC-05 in AT commands mode.
 *
 * @param[*hc05] - pointer to struct for HC05 bluetooth module
 * @param[Command] - predefined commands that are in .h file @actioncommands
 *
 * @return - void
 */
void HC05_SendActionCommand(HC05_t *hc05, HC05_ACT Command)
{

		switch (Command)
		{
	case HC05_ACT_AT_RESET:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+RESET\r\n");
			break;

	case HC05_ACT_AT_ORGL:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+ORGL\r\n");
			break;

	case HC05_ACT_AT_RMAAD:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+RMAAD?\r\n");
			break;

	case HC05_ACT_AT_INIT:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+INIT\r\n");
			break;

	case HC05_ACT_AT_INQ:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+INQ\r\n");
			break;

	case HC05_ACT_AT_INQC:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+INQC\r\n");
			break;

	case HC05_ACT_AT_DISC:
			HC05_SendAndReceiveCmd(hc05, (uint8_t*) "AT+DISC\r\n");
			break;

		}
		return;
}

/*
 * Set new name to HC05 module
 *
 * @param[*hc05] - pointer to struct for HC05 bluetooth module
 *
 * @return - void
 */
void HC05_SetName(HC05_t *hc05, uint8_t* Name)
{

	uint8_t Msg[32];
	sprintf((char*) Msg, "AT+NAME=%s\r\n", Name);
	HC05_SendAndReceiveCmd(hc05, Msg);
	return;

}

/*
 * Set mode to HC05 module
 *
 * @param[*hc05] - pointer to struct for HC05 bluetooth module
 * @param[Mode]	 - HC05 mode @modes
 * @return - void
 */
void HC05_SetRole(HC05_t *hc05, uint8_t Mode)
{

	uint8_t Msg[32];
	sprintf((char*) Msg, "AT+ROLE=%d\r\n", Mode);
	HC05_SendAndReceiveCmd(hc05, Msg);
	return;

}

/*
 * Set Query access patterns to HC05 module
 *
 * @param[*hc05]		- pointer to struct for HC05 bluetooth module
 * @param[InqMode]	    - HC05 inquiry mode @inquirymodes
 * @param[MaxDevices]	-  Maximum number of Bluetooth devices to respond to
 * @param[Timeout]		-  Timeout (1-48 : 1.28s to 61.44s)
 * @return - void
 */
void HC05_SetINQM(HC05_t *hc05, uint8_t InqMode, uint8_t MaxDevices, uint8_t Timeout)
{

	uint8_t Msg[32];
	sprintf((char*) Msg, "AT+INQM=%d,%d,%d\r\n", InqMode, MaxDevices,
			Timeout);
	HC05_SendAndReceiveCmd(hc05, Msg);
	return;

}

/*
 * Set new password
 *
 * @param[*hc05]		- pointer to struct for HC05 bluetooth module
 * @param[*Password]    - pointer to uint8_t array
 * @return - void
 */
void HC05_SetPassword(HC05_t *hc05, uint8_t* Password)
{

	uint8_t Msg[32];
	sprintf((char*) Msg, "AT+PSWD=\"%s\"\r\n", Password);
	HC05_SendAndReceiveCmd(hc05, Msg);
	return;

}

/*
 * Set Connection mode
 *
 * @param[*hc05]		- pointer to struct for HC05 bluetooth module
 * @param[CMODE]    - connection modes @connectionmodes
 * @return - void
 */
void HC05_SetConnectionMode(HC05_t *hc05, uint8_t CMODE)
{

	uint8_t Msg[32];
	sprintf((char*) Msg, "AT+CMODE=%d\r\n", CMODE);
	HC05_SendAndReceiveCmd(hc05, Msg);
	return;

}

/*
 * Set Uart parameters
 *
 * @param[*hc05]		- pointer to struct for HC05 bluetooth module
 * @param[Baudrate] 	- Baudrate for uart
 * @param[Stopbit] 		- How many stop bits
 * @param[Parity]		- Parity bit
 * @return - void
 */
void HC05_SetUart(HC05_t *hc05, uint32_t Baudrate,uint8_t Stopbit,uint8_t Parity)
{

	uint8_t Msg[32];
	sprintf((char*) Msg, "AT+UART=%ld,%d,%d\r\n", Baudrate, Stopbit, Parity);
	HC05_SendAndReceiveCmd(hc05, Msg);
	return;

}

/*
 * Set Uart parameters
 *
 * @param[*hc05]		- pointer to struct for HC05 bluetooth module
 * @param[Address1] 	- first part of BT address format XXXX,xx,xxxxxx
 * @param[Address2] 	- second part of BT address format xxxx,XX,xxxxxx
 * @param[Address3] 	- third part of BT address format xxxx,xx,XXXXXX
 * Address got from inquiring : 8327:0:8182
 * Address to connect : 		8327:00:008182

 * @return - void
 */
void HC05_ConnectToAddress(HC05_t *hc05, uint32_t Address1, uint32_t Address2, uint32_t Address3)
{

	uint8_t Msg[32];

	sprintf((char*) Msg, "AT+PAIR=%04ld,%02ld,%06ld\n\r", Address1, Address2, Address3);
	HC05_SendAndReceiveCmd(hc05, Msg);
	sprintf((char*) Msg, "AT+BIND=%04ld,%02ld,%06ld\n\r", Address1, Address2, Address3);
	HC05_SendAndReceiveCmd(hc05, Msg);
	sprintf((char*) Msg, "AT+LINK=%04ld,%02ld,%06ld\n\r", Address1, Address2, Address3);
	HC05_SendAndReceiveCmd(hc05, Msg);
	return;

}

/*
 * Initialize HC05_Init structure
 *
 * @param[*hc05] - pointer to struct for HC05 bluetooth module
 * @param[huart] - handler to uart that HC05 is connected to
 *
 * @return - void
 */
void HC05_Init(HC05_t *hc05, UART_HandleTypeDef *huart)
{

	HC05_GPIO_Init();

	// init msg
	HC05_DisplayTerminal("HC-05 Initializing... \n\r");

	// reset the ring buffer
	RB_Flush(&(hc05->RingBuffer));

	// Assign uart
	hc05->huart = huart;

	// if irq mode is used for receive
#if (HC05_UART_RX_IT == 1)
	HAL_UART_Receive_IT(hc05->huart, &(hc05->RecieveBufferIT), 1);
#endif

	// if dma mode is used for receive
#if (HC05_UART_RX_DMA == 1)
	HAL_UARTEx_ReceiveToIdle_DMA(hc05->huart, hc05->RecieveBufferDMA,
	HC05_RECIEVEBUFFERSIZE);
	// to avoid callback from half message this has be disabled
	__HAL_DMA_DISABLE_IT(hc05->huart->hdmarx, DMA_IT_HT);
#endif

#if(HC05_FIRSTINIT == 1)
	// Enter AT MODE 1 - 38400 baud - slow blinking
	HC05_PWR_LOW;
	HC05_EN_HIGH;
	HAL_Delay(100);
	HC05_PWR_HIGH;

	//send AT commands with 38400 baud
	HC05_SendActionCommand(hc05, HC05_ACT_AT_ORGL);				// reset to default parameters
	HC05_SendActionCommand(hc05, HC05_ACT_AT_RMAAD);			// forget all devices
	HC05_SetRole(hc05, HC05_MODE_MASTER);						// mode master
	HC05_SetName(hc05, (uint8_t*) "MrProper");					// add name
	HC05_SetConnectionMode(hc05, HC05_CMODE_ANYADDRESS);		// connect to anything
	HC05_SetINQM(hc05, HC05_INQUIRY_MODE_STANDARD, 9, 48);		// inquiry mode
	HC05_SetPassword(hc05, (uint8_t*) "1234");					// !! PASSWORD MUST BE THE SAME AS IN SLAVE TO CONNECT !!
	HC05_SetUart(hc05,38400,1,0);								// set uart parameters to be the same as AT MODE 1 - makes it easier
	HC05_SendActionCommand(hc05, HC05_ACT_AT_RESET);			// reset
	// change AT mode

	// enter AT MODE 2 - 38400 (after setting it with SetUart) - fast blinking
	HC05_PWR_LOW;
	HC05_EN_LOW;
	HAL_Delay(1000);
	HC05_PWR_HIGH;
	HAL_Delay(1000);
	HC05_EN_HIGH;
	HAL_Delay(1000);


	// Pair + bind + link to target device
	HC05_ConnectToAddress(hc05,8327,0,8182);
	// exit AT mode
#endif

	HC05_PWR_HIGH;

}

/*
 * Clear message pending flag
 *
 * @param[*hc05] - pointer to struct for HC05 bluetooth module
 * @return - void
 */
void HC05_ClearMsgPendingFlag(HC05_t *hc05)
{
	hc05->MessagePending = 0;
}

/*
 * Check if there is a message Pending, if yes -> write it to a MsgBuffer
 *
 * @param[*hc05] - pointer to struct for HC05 bluetooth module
 * @param[*MsgBuffer] - pointer to buffer where message has to be written
 * @return - status : massage pending 1/0
 */
uint8_t HC05_CheckPendingMessages(HC05_t *hc05, uint8_t *MsgBuffer)
{

	// Check if there is message finished
	if (hc05->LinesRecieved > 0)
	{

		uint8_t i = 0;
		uint8_t temp = 0;
		do
		{
			// Move a sign to ring buffer
			RB_Read(&(hc05->RingBuffer), &temp);
			if (temp == HC05_LASTCHARACTER)
			{
				MsgBuffer[i] = HC05_LASTCHARACTER;
				MsgBuffer[i + 1] = 0;
			}
			else
			{
				MsgBuffer[i] = temp;
			}
			i++;
			//rewrite signs until last character defined by user
		} while (temp != HC05_LASTCHARACTER);
		//decrement LinesRecieved
		hc05->LinesRecieved--;
		//set up flag that message is ready to parse
		hc05->MessagePending = HC05_MESSAGEPENDING;
	}

	// return if flag status
	return hc05->MessagePending;
}

/*
 * Callback to put in HAL_UART_RxCpltCallback for IT mode
 *
 * @param[*hc05] - pointer to struct for HC05 bluetooth module
 * @param[*huart] - uart handle
 * @return - void
 */
#if (HC05_UART_RX_IT == 1)
void HC05_RxCpltCallbackIT(HC05_t *hc05, UART_HandleTypeDef *huart)
{

	//check if IRQ is coming from correct uart
	if (hc05->huart->Instance == huart->Instance)
	{
		//write a sign to ring buffer
		RB_Write((&(hc05->RingBuffer)), hc05->RecieveBufferIT);

		// when line is complete -> add 1 to received lines
		if (hc05->RecieveBufferIT == HC05_LASTCHARACTER)
		{
			(hc05->LinesRecieved)++;
		}

		// start another IRQ for single sign
		HAL_UART_Receive_IT(hc05->huart, &(hc05->RecieveBufferIT), 1);
	}
}
#endif

/*
 * Callback to put in HAL_UARTEx_RxEventCallback for DMA mode
 *
 * @param[*hc05] - pointer to struct for HC05 bluetooth module
 * @param[*huart] - uart handle
 * @param[size] - size of the recieved message
 * @return - void
 */
#if (HC05_UART_RX_DMA == 1)
void HC05_RxCpltCallbackDMA(HC05_t *hc05, UART_HandleTypeDef *huart,
		uint16_t size)
{

	//check if IRQ is coming from correct uart
	if (hc05->huart->Instance == huart->Instance)
	{

		uint8_t i;
		uint8_t newlines = 0;
		//write message to ring buffer
		for (i = 0; i < size; i++)
		{
			RB_Write((&(hc05->RingBuffer)), hc05->RecieveBufferDMA[i]);

			// when line is complete -> add 1 to received lines
			// only when last char is \n
			if (hc05->RecieveBufferDMA[i] == HC05_LASTCHARACTER)
			{
				newlines++;
			}
		}

		if (newlines == 0)
		{
			// if formt of data is not correct print msg

			//flush ringbuffer to not send later trash data
			RB_Flush(&(hc05->RingBuffer));
		}

		// add new lines
		hc05->LinesRecieved = +newlines;

		// start another IRQ for single sign
		HAL_UARTEx_ReceiveToIdle_DMA(hc05->huart, hc05->RecieveBufferDMA,
		HC05_RECIEVEBUFFERSIZE);
		// to avoid callback from half message this has be disabled
		__HAL_DMA_DISABLE_IT(hc05->huart->hdmarx, DMA_IT_HT);
	}
}
#endif

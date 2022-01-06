/*
 * HC-05.h
 *
 *  Created on: Dec 9, 2021
 *      Author: ROJEK
 */
#include "ringbuffer.h"

#ifndef INC_HC_05_V3_H_
#define INC_HC_05_V3_H_

/*
 * definitions to edit by user :
 */

#define HC05_UART_TIMEOUT			1000
#define HC05_RECIEVEBUFFERSIZE		64				// with 128 didnt recieve callback during inquiring
#define HC05_FIRSTINIT				0				// use only to set up device

// Define RX - IT/DMA
#define HC05_UART_RX_IT				0

// GPIO for EN pin
#define HC05_EN_PORT 				GPIOC
#define HC05_EN_PIN 				GPIO_PIN_0
#define HC05_EN_CLK_ENABLE			__HAL_RCC_GPIOC_CLK_ENABLE

// GPIO for module VCC pin (on BJT)
#define HC05_PWR_PORT 				GPIOC
#define HC05_PWR_PIN 				GPIO_PIN_3
#define HC05_PWR_CLK_ENABLE			__HAL_RCC_GPIOC_CLK_ENABLE

// GPIO for STATE pin
#define HC05_STATE_PORT 			GPIOA
#define HC05_STATE_PIN 				GPIO_PIN_7
#define HC05_STATE_CLK_ENABLE		__HAL_RCC_GPIOA_CLK_ENABLE

/*
 * library definitions :
 */
#define HC05_LASTCHARACTER			'\n'

#if (HC05_UART_RX_IT == 0)
#define HC05_UART_RX_DMA			1
#endif

#define HC05_NOMESSAGE				0
#define HC05_MESSAGEPENDING			1

#define HC05_EN_LOW					HAL_GPIO_WritePin(HC05_EN_PORT,HC05_EN_PIN,GPIO_PIN_RESET)
#define HC05_EN_HIGH				HAL_GPIO_WritePin(HC05_EN_PORT,HC05_EN_PIN,GPIO_PIN_SET)

#define HC05_PWR_LOW				HAL_GPIO_WritePin(HC05_PWR_PORT,HC05_PWR_PIN,GPIO_PIN_RESET)
#define HC05_PWR_HIGH				HAL_GPIO_WritePin(HC05_PWR_PORT,HC05_PWR_PIN,GPIO_PIN_SET)

/*
 * @modes
 */
#define HC05_MODE_SLAVE				0
#define HC05_MODE_MASTER			1
#define HC05_MODE_SLAVELOOP			2

/*
 * @inquirymodes
 */
#define HC05_INQUIRY_MODE_STANDARD	0
#define HC05_INQUIRY_MODE_RSSI		1

/*
 * @connectionmodes
 */
#define HC05_CMODE_FIXEDADDRESS		0
#define	HC05_CMODE_ANYADDRESS		1
#define HC05_CMODE_SLAVELOOP		2
/*
 * @infocommands
 * Information commands:
 * Commands that do not require any parameters that
 * are send in AT command mode between MCU and HC05.
 * HC-05 sends back information
 */
typedef enum HC05_INFO_CMD{

	HC05_INFO_AT,					// 1. Test command:
	HC05_INFO_AT_VERSION,			// 3. Get firmware version
	HC05_INFO_AT_ADDR,				// 5. Get module address
	HC05_INFO_AT_NAME,				// 6. Check module name
	HC05_INFO_AT_MODE,				// 8. Check module mode:
	HC05_INFO_AT_ROLE,				// 9. Check device class
	HC05_INFO_AT_IAC,				// 10. Check GIAC (General Inquire Access Code
	HC05_INFO_AT_INQM,				// 11. Check -- Query access patterns
	HC05_INFO_AT_PSWD,				// 12. Check PIN code:
	HC05_INFO_AT_UART,				// 13. Check serial parameter
	HC05_INFO_AT_CMODE,				// 14. Check connect mode
	HC05_INFO_AT_BIND,				// 15. Check fixed address:
	HC05_INFO_AT_POLAR,				// 16. Set/Check LED I/O
	HC05_INFO_AT_IPSCAN,			// 18. Check – scan parameter
	HC05_INFO_AT_SNIFF,				// 19. Check – SHIFF parameter
	HC05_INFO_AT_SENM,				// 20. Check security mode
	HC05_INFO_AT_ADCN,				// 24. Get Authenticated Device Count
	HC05_INFO_AT_MRAD,				// 25. Most Recently Used Authenticated Device
	HC05_INFO_AT_STATE,				// 26. Get the module working state
	HC05_INFO_AT_HELP

}HC05_INFO;

/*
 * @actioncommands
 * Action commands:
 * Commands that do not require any parameters that
 * are send in AT command mode between MCU and HC05.
 * HC-05 do the action and send back confirmation
 */
typedef enum HC05_ACT_CMD{

	HC05_ACT_AT_RESET,				// 2. Reset
	HC05_ACT_AT_ORGL,				// 4. Restore default
	HC05_ACT_AT_RMAAD,				// 22. Delete All Authenticated Device
	HC05_ACT_AT_INIT,				// 27. Initialize the SPP profile lib
	HC05_ACT_AT_INQ,				// 28. Inquiry Bluetooth Device
	HC05_ACT_AT_INQC,				// 28. Cancel Inquiring Bluetooth Device
	HC05_ACT_AT_DISC				// 31. Disconnect

}HC05_ACT;

/*
 * @module structure
 *
 */
typedef struct HAC05_t
{
	UART_HandleTypeDef*	huart; 							// Uart handle

	uint8_t RecieveBufferDMA[HC05_RECIEVEBUFFERSIZE];	// buffer for received messages in DMA mode

	uint8_t RecieveBufferIT;							// 1 byte buffer for a single char received in IRQ mode

	Ringbuffer_t RingBuffer;							// ring buffer to save data

	volatile uint8_t LinesRecieved;						// lines that were received

	uint8_t MessagePending;								// status that message is ready to parse

}HC05_t;

/*
 * @functions
 *
 */
void HC05_Init(HC05_t *hc05, UART_HandleTypeDef *huart);
void HC05_SendInfoCommand(HC05_t *hc05, HC05_INFO Command);
void HC05_SendActionCommand(HC05_t *hc05, HC05_ACT Command);

#if (HC05_UART_RX_DMA == 1)
void HC05_RxCpltCallbackDMA(HC05_t *hc05, UART_HandleTypeDef *huart,
		uint16_t size);
#endif
#if (HC05_UART_RX_IT == 1)
void HC05_RxCpltCallbackIT(HC05_t *hc05, UART_HandleTypeDef *huart);
#endif

uint8_t HC05_CheckPendingMessages(HC05_t *hc05, uint8_t *MsgBuffer);
void HC05_ClearMsgPendingFlag(HC05_t *hc05);

#endif /* INC_HC_05_V3_H_ */

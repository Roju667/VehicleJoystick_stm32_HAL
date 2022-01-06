/*
 * movement_control.h
 *
 *  Created on: Dec 21, 2021
 *      Author: pawel
 */

#ifndef INC_MOVEMENTT_CONTROL_H_
#define INC_MOVEMENT_CONTROL_H_

// MOTOR defines
#define	MOTOR_PWM_COUNTERPERIOD 	40000
#define MOTOR_LOW_LIMIT 			0
#define MOTOR_HIGH_LIMIT			MOTOR_PWM_COUNTERPERIOD
#define MOTOR_RESOLUTION_1PERCENT	MOTOR_PWM_COUNTERPERIOD / 100
#define MOTOR_DEADBAND_LOW_LIMIT	18000
#define MOTOR_DEADBAND_HIGH_LIMIT 	22000


// SERVO defines
#define	SERVO_PWM_COUNTERPERIOD 	40000								// has to be 50 Hz (20ms) - (0,5ms = 0 degree) - (2,5ms = 180 degree)
#define SERVO_LOW_LIMIT_DEGREES		60									// lowest position for servo
#define SERVO_HIGH_LIMIT_DEGREES 	120									// highest position for servo
#define SERVO_0_DEGREES_PWM			SERVO_PWM_COUNTERPERIOD / 40		// starting value is 2,5% of 50Hz PWM signal
#define SERVO_RESOLUTION_1DEGREE	SERVO_PWM_COUNTERPERIOD / 1800		// 1 degree is 22 adc points
#define SERVO_LOW_LIMIT 			((SERVO_0_DEGREES_PWM) + (SERVO_LOW_LIMIT_DEGREES * (SERVO_PWM_COUNTERPERIOD / 1800)))		// calculate it to PWM values
#define SERVO_HIGH_LIMIT			((SERVO_0_DEGREES_PWM) + (SERVO_HIGH_LIMIT_DEGREES * (SERVO_PWM_COUNTERPERIOD / 1800)))		// calculate it to PWM values


#endif /* INC_MOVEMENT_CONTROL_H_ */

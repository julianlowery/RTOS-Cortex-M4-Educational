/*
 * mutex_demo.h
 *
 *  Created on: Jan 29, 2026
 *      Author: julianlowery
 */

#ifndef INC_MUTEX_DEMO_H_
#define INC_MUTEX_DEMO_H_


void mutex_demo_high_task(void *arg);
void mutex_demo_medium_task(void *arg);
void mutex_demo_low_task(void *arg);
void mutex_demo_init();


#endif /* INC_MUTEX_DEMO_H_ */

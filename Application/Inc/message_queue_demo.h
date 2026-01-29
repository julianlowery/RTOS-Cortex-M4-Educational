/*
 * message_queue_demo.h
 *
 *  Created on: Jan 29, 2026
 *      Author: julianlowery
 */

#ifndef INC_MESSAGE_QUEUE_DEMO_H_
#define INC_MESSAGE_QUEUE_DEMO_H_

void queue_demo_producer1_task(void* arg);
void queue_demo_producer2_task(void* arg);
void queue_demo_consumer_task(void* arg);
void message_queue_demo_init();

#endif /* INC_MESSAGE_QUEUE_DEMO_H_ */

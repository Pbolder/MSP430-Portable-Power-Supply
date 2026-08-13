/*
 * timer.h
 *
 * Timer-driven scheduler interface.
 */

#ifndef TIMER_H_
#define TIMER_H_

/*
 * These flags are set by the Timer_A interrupt and cleared
 * by the main application after the corresponding task runs.
 */
extern volatile unsigned char g_task10ms;
extern volatile unsigned char g_task100ms;
extern volatile unsigned char g_task500ms;

/*
 * Used only to verify that the scheduler is operating.
 * It increments once every 500 ms.
 */
extern volatile unsigned int g_heartbeatCount;

void Timer_Init(void);

#endif /* TIMER_H_ */

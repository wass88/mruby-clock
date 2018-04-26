#ifndef L_MERUBY_TASK_H
#define L_MERUBY_TASK_H
void mruby_task(void*);
void led_init(void);
void led_print(void*);
extern bool task_switched;
extern bool task_script;
#endif





#include <zephyr/kernel.h>
// #include <zephyr/random/random.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

#define STACK_SIZE 1024

/* Define stacks and thread data */
K_THREAD_STACK_DEFINE(stack_l, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_m, STACK_SIZE);
K_THREAD_STACK_DEFINE(stack_h, STACK_SIZE);

LOG_MODULE_REGISTER(foo, 4);

int64_t deadline_sh; // 

struct k_thread thread_l_data, thread_m_data, thread_h_data;

void thread_low_entry(void *p1, void *p2, void *p3) {
      LOG_DBG("system time t1: %lld\n", k_cycle_get_64());
	//   k_thread_deadline_set(&thread_l_data,2147483648);
        // k_thread_deadline_set(&thread_l_data,5000);
        k_thread_absolute_deadline_set(&thread_l_data, 28475);
      LOG_DBG("Thread_Low: Started with deadline %lld ticks\n", _current->base.prio_deadline);
//  k_sched_lock();
//   deadline_sh =  _current->base.prio_deadline;
//     k_sched_unlock();
      LOG_DBG("system time t1: %lld\n", k_cycle_get_64());
	      k_reschedule(); 
    // printf("Thread_Low: Started\n");
    LOG_DBG("Thread_Low: Started after reschedule with deadline %lld ticks\n", _current->base.prio_deadline);
    k_busy_wait(10000000); // Simulate 1s work
    // printf("Thread_Low: Finished work\n");

    LOG_DBG("Thread_Low: Finished work\n");
      LOG_DBG("system time t1: %lld\n", k_cycle_get_64());
        LOG_DBG("Thread_Low: finished with deadline %lld ticks\n", _current->base.prio_deadline);
}

void thread_med_entry(void *p1, void *p2, void *p3) {
  
      LOG_DBG("Thread_Med: Starting with deadline %lld ticks\n", _current->base.prio_deadline);
    LOG_DBG("system time t2: %lld\n", k_cycle_get_64());
//  k_sched_lock();
k_thread_absolute_deadline_set(&thread_m_data, 28475);
// k_sched_unlock();

        LOG_DBG("Thread_Med: Started with deadline %lld ticks\n", _current->base.prio_deadline);
        LOG_DBG("system time t2: %lld\n", k_cycle_get_64());
	   k_reschedule();
    // printf("Thread_Med: Started\n");
    LOG_DBG("Thread_Med: Started after reschedule with deadline %lld ticks\n", _current->base.prio_deadline);
    for (int i = 0; i < 10; i++) {
        // printf("Thread_Med: Working... (%d/5)\n", i + 1);
        k_busy_wait(1000000); // Simulate work in chunks
        //  k_reschedule();

    }
    // k_busy_wait(10000000); // Simulate 1s work
    // printf("Thread_Med: Finished work\n");
    LOG_DBG("Thread_Med: Finished work\n");

      LOG_DBG("system time t2: %lld\n", k_cycle_get_64());
}

void thread_high_entry(void *p1, void *p2, void *p3) {
        // printf("Thread_High: Started\n");
        LOG_DBG("system time t3: %lld\n", k_cycle_get_64());
	//  k_thread_deadline_set(&thread_h_data, 4800); 
//  k_sched_lock();
// k_thread_absolute_deadline_set(&thread_h_data, deadline_sh);
// k_sched_unlock();

k_thread_absolute_deadline_set(&thread_h_data, 28475);

     LOG_DBG("system time t3: %lld\n", k_cycle_get_64());
        LOG_DBG("Thread_High: Started with deadline %lld ticks\n", _current->base.prio_deadline);
	   k_reschedule();
    // printf("Thread_High: Started\n");
    LOG_DBG("Thread_High: Started after reschedule with deadline %lld ticks\n", _current->base.prio_deadline);
    k_busy_wait(1000000); // Simulate 1s work
    // printf("Thread_High: Finished work\n");
    LOG_DBG("Thread_High: Finished work\n");
    LOG_DBG("system time t3: %lld\n", k_cycle_get_64());
}

int main(void) {
    /* 1. Create threads in K_FOREVER (suspended) state */
    /* Note: Priority is set to 10 for all, but Deadline will override this */

LOG_DBG("Main: start\n");
    k_tid_t thread1_pointer, thread2_pointer, thread3_pointer;

   thread1_pointer= k_thread_create(&thread_l_data, stack_l, STACK_SIZE, thread_low_entry, 
                    NULL, NULL, NULL, 10, 0, K_FOREVER);
    
  thread2_pointer=  k_thread_create(&thread_m_data, stack_m, STACK_SIZE, thread_med_entry, 
                    NULL, NULL, NULL, 10, 0, K_FOREVER);
    
   thread3_pointer= k_thread_create(&thread_h_data, stack_h, STACK_SIZE, thread_high_entry, 
                    NULL, NULL, NULL, 10, 0, K_FOREVER);

    /* 2. Set deadlines from main before starting */
    /* k_thread_deadline_set(thread_ptr, deadline_value) */
    // k_thread_deadline_set(&thread_h_data, 100);   // Earliest deadline (Highest priority)
    // k_thread_deadline_set(&thread_l_data, 500);   // Middle deadline
    // k_thread_deadline_set(&thread_m_data, 20000); // Furthest deadline (Lowest priority)

      k_thread_name_set(thread1_pointer, "Thread_Low");
        k_thread_name_set(thread2_pointer, "Thread_Med");
        k_thread_name_set(thread3_pointer, "Thread_High");

    /* 3. Start the threads */
   LOG_DBG("Main: starting thread1\n"); 
    k_thread_start(thread1_pointer);
    k_msleep(1);
    LOG_DBG("Main: starting thread2\n");
    k_thread_start(thread2_pointer);
     k_msleep(100000);
     LOG_DBG("Main: starting thread3\n");
     	//  k_thread_deadline_set(&thread_h_data, 4800); 
     k_thread_start(thread3_pointer);


    k_thread_join(thread1_pointer, K_FOREVER);
    k_thread_join(thread2_pointer, K_FOREVER);
    k_thread_join(thread3_pointer, K_FOREVER);
    while (1) {
        k_msleep(1000); // Keep main thread alive
    }


    return 0;
}
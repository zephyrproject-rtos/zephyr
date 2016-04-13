/*This is a hello world example*/
/*taskA exchanges hello messages with taskB*/
/*
* @brief Does Hello message.
* -Calls function helloLoop.
*/
void taskA(void)
{
	helloLoop(__func__, TASKBSEM, TASKASEM);
}

#else

 /* @brief A loop saying hello.
 *
 \param taskname The task's identification string.
 \param mySem    The task's semaphore.
 \param otherSem The other task's semaphore.
 *
 */
void taskA(void)
{
	helloLoop(__func__, TASKBSEM, TASKASEM);
}
#else
 /* Actions:
 *
 * -# Ouputs "Hello World!".
 * -# Waits, then lets another task run.
 */

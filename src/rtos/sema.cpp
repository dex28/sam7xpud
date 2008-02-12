
#include "sema.hpp"

//---------------------------------------------------------------------------------------

void* xSEMA::operator new( size_t size )
{
    return pvPortMalloc( size );
}

void xSEMA::operator delete( void* p )
{
    vPortFree( p );
}

//---------------------------------------------------------------------------------------

xSEMA::xSEMA( unsigned portBASE_TYPE uxInitialCount )
{
    isMutex = false;
    pxMutexHolder = NULL;

    // Initialise the semaphore members as described above where the
    // semaphore type is defined.
    //
    xItemCount = uxInitialCount;
    xTxLock = semaUNLOCKED;
    
    // Likewise ensure the event semas start with the correct state.
    //
    vListInitialise( &xTasksWaitingToReceive );
}

//---------------------------------------------------------------------------------------

void xSEMA::Release( unsigned portBASE_TYPE count  )
{
    // Make sure other tasks do not access the semaphore.
    //
    vTaskSuspendAll ();

    // It is important that interrupts do not access the event list of the
    // semaphore being modified here. Places where the event list is modified
    // include:
    //
    //   -  xSEMA::ReleaseFromISR(). This checks the lock on the semaphore to see
    //      if it has access. If the semaphore is locked then the Tx lock count is
    //      incremented to signify that a task waiting for items can be made ready
    //      once the semaphore lock is removed. If the semaphore is not locked then
    //      a task can be moved from the event list, but will not be removed
    //      from the delayed list or placed in the ready list until the scheduler
    //      is unlocked.
    //
    //   -  xSEMA::WaitFromISR(). As per xSEMA::ReleaseFromISR().
    //
    Lock ();

    taskENTER_CRITICAL();
    
    xItemCount += count;

    if( isMutex )
    {
        // The mutex is no longer being held.
        //
        vTaskPriorityDisinherit( pxMutexHolder );
    }

    // Items were posted while the semaphore was locked. Are any tasks
    // blocked waiting for items to become available?
    //
    while( ! listLIST_IS_EMPTY( &xTasksWaitingToReceive ) )
    {
        // Tasks that are removed from the event list will get added to
        // the pending ready list as the scheduler is still suspended.
        //
        if( xTaskRemoveFromEventList( &xTasksWaitingToReceive ) != pdFALSE )
        {
            // The task waiting has a higher priority so record that a
            // context switch is required.
            //
            vTaskMissedYield ();
        }
    }           

    // We no longer require exclusive access to the semaphore.
    //
    xTxLock = semaUNLOCKED;

    taskEXIT_CRITICAL ();

    xTaskResumeAll ();
}

//---------------------------------------------------------------------------------------

signed portBASE_TYPE xSEMA::ReleaseFromISR
( 
    unsigned portBASE_TYPE count, 
    signed portBASE_TYPE xTaskPreviouslyWoken
    )
{
    xItemCount += count;

    if( isMutex )
    {
        // The mutex is no longer being held.
        //
        vTaskPriorityDisinherit( pxMutexHolder );
    }
    
    // If the semaphore is locked we do not alter the event list. This will
    // be done when the semaphore is unlocked later.
    //
    if( xTxLock == semaUNLOCKED )
    {
        // We only want to wake one task per ISR, so check that a task has
        // not already been woken.
        //
        if( ! xTaskPreviouslyWoken )     
        {
            if( ! listLIST_IS_EMPTY( &xTasksWaitingToReceive ) )
            {
                if( xTaskRemoveFromEventList( &xTasksWaitingToReceive ) != pdFALSE )
                {
                    // The task waiting has a higher priority so record that a
                    // context switch is required.
                    //
                    return pdTRUE;
                }
            }
        }
    }
    else
    {
        // Increment the lock count so the task that unlocks the sema
        // knows that items was posted while it was locked.
        //
        ++xTxLock;
    }

    return xTaskPreviouslyWoken;
}

//---------------------------------------------------------------------------------------

signed portBASE_TYPE xSEMA::Wait
(
    unsigned portBASE_TYPE count,
    portTickType xTicksToWait,
    portBASE_TYPE xJustPeeking
    )
{
    // This function is very similar to xQueueGenericSend(). See comments
    // within xQueueGenericSend() for a more detailed explanation.
    
    // Make sure other tasks do not access the semaphore.
    //
    vTaskSuspendAll();

    // Capture the current time status for future reference.
    //
    xTimeOutType xTimeOut;
    vTaskSetTimeOutState( &xTimeOut );

    // It is important that this is the only thread/ISR that modifies the
    // ready or delayed lists until xTaskResumeAll() is called. Places where
    // the ready/delayed lists are modified include:
    //  vTaskDelay()             - Nothing can call vTaskDelay as the scheduler is
    //                             suspended, vTaskDelay() cannot be called from an ISR.
    //  vTaskPrioritySet()       - Has a critical section around the access.
    //  vTaskSwitchContext()     - This will not get executed while the scheduler
    //                             is suspended.
    //  prvCheckDelayedTasks()   - This will not get executed while the
    //                             scheduler is suspended.
    //  xTaskCreate()            - Has a critical section around the access.
    //  vTaskResume()            - Has a critical section around the access.
    //  xTaskResumeAll()         - Has a critical section around the access.
    //  xTaskRemoveFromEventList - Checks to see if the scheduler is
    //                             suspended. If so then the TCB being removed from the 
    //                             event is removed from the event and added to the 
    //                             xPendingReadyList.

    // Make sure interrupts do not access the semaphore.
    //
    Lock ();

    const signed portBASE_TYPE semaERRONEOUS_UNBLOCK = -1;
    signed portBASE_TYPE xReturn = pdTRUE;

    do
    {
        // If there are no enough items in the semaphore we may have to block.
        //
        if( ! DoesSemaContains( count ) )
        {
            // There are no messages in the sema, do we want to block or just
            // leave with nothing?
            //
            if( xTicksToWait > 0 )
            {
                if( isMutex )
                {
                    portENTER_CRITICAL();
                    vTaskPriorityInherit( pxMutexHolder );
                    portEXIT_CRITICAL();
                }

                vTaskPlaceOnEventList( &xTasksWaitingToReceive, xTicksToWait );

                taskENTER_CRITICAL ();
                {
                    Unlock ();

                    if( ! xTaskResumeAll () )
                    {
                        taskYIELD ();
                    }

                    if( xItemCount < (signed portBASE_TYPE) count )
                    {
                        // We unblocked but the semaphore is empty. We probably
                        // timed out.
                        //
                        xReturn = errQUEUE_EMPTY;
                    }
    
                    vTaskSuspendAll ();
                    Lock ();
                }
                taskEXIT_CRITICAL ();
            }
        }
    
        if( xReturn != errQUEUE_EMPTY )
        {
            taskENTER_CRITICAL ();
            {
                if( xItemCount >= (signed portBASE_TYPE) count )
                {
                    if( xJustPeeking == pdFALSE )
                    {
                        // We are actually removing items.
                        //
                        xItemCount -= count;
                        
                        if( isMutex )
                        {
                            // Record the information required to implement
                            // priority inheritance should it become necessary.
                            //
                            pxMutexHolder = (xTaskHandle*) xTaskGetCurrentTaskHandle ();
                        }
                    }
                    else
                    {
                        // The item countsis unchanged, so increment the lock
                        // count so Unlock() knows to check for other tasks waiting
                        // for items to be available.
                        //
                        ++xTxLock;
                    }
                    
                    xReturn = pdPASS;                   
                }
                else
                {
                    xReturn = errQUEUE_EMPTY;
                }
            }
            taskEXIT_CRITICAL ();
        }

        if( xReturn == errQUEUE_EMPTY )
        {
            if( xTicksToWait > 0 )
            {
                if( xTaskCheckForTimeOut( &xTimeOut, &xTicksToWait ) == pdFALSE )
                {
                    xReturn = semaERRONEOUS_UNBLOCK;
                }
            }
        }
    } while( xReturn == semaERRONEOUS_UNBLOCK );

    // We no longer require exclusive access to the semaphore.
    //
    Unlock ();
    xTaskResumeAll ();

    return xReturn;
}

//---------------------------------------------------------------------------------------

signed portBASE_TYPE xSEMA::WaitFromISR( unsigned portBASE_TYPE count )
{
    // We cannot block from an ISR, so check there are items available.
    //
    if( xItemCount < (signed portBASE_TYPE) count )
        return pdFAIL;
    
    xItemCount -= count;

    return pdPASS;
}

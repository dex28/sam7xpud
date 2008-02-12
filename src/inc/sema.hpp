#ifndef _SEMA_HPP_INCLUDED
#define _SEMA_HPP_INCLUDED

//---------------------------------------------------------------------------------------
//     Includes
//---------------------------------------------------------------------------------------

#include "FreeRTOS.h"
#include "task.h"

#include "trace.h"

//---------------------------------------------------------------------------------------
// Definition of the semaphore used by the scheduler.
//---------------------------------------------------------------------------------------
class xSEMA
{
protected:

    enum { semaUNLOCKED  = -1 };

    bool isMutex;
    xTaskHandle* pxMutexHolder;

    // The number of items currently in the semaphore.
    //
    volatile signed portBASE_TYPE xItemCount;

    // Stores the number of items transmitted to the semaphore (added to the sema) 
    // while the semaphore was locked. Set to semaUNLOCKED when the semaphore is 
    // not locked.
    //
    signed portBASE_TYPE xTxLock;  

    // List of tasks that are blocked waiting to read from this semaphore.
    // Stored in priority order.
    //
    xList xTasksWaitingToReceive; 

    //-----------------------------------------------------------------------------------
    // Function to mark a semaphore as locked. Locking a semaphore prevents an ISR from
    // accessing the semaphore event lists.
    //
    inline void Lock( void )
    {
        taskENTER_CRITICAL ();
        ++xTxLock;
        taskEXIT_CRITICAL();
    }

    //-----------------------------------------------------------------------------------
    // Unlocks a semaphore locked by a call to Lock(). Locking a semaphore does not
    // prevent an ISR from adding or removing items to the sema, but does prevent
    // an ISR from removing tasks from the semaphore event lists. If an ISR finds a
    // semaphore is locked it will instead increment the appropriate semaphore lock count
    // to indicate that a task may require unblocking. When the semaphore in unlocked
    // these lock counts are inspected, and the appropriate action taken.
    // THIS FUNCTION MUST BE CALLED WITH THE SCHEDULER SUSPENDED.
    // 
    inline void Unlock( void )
    {
        // The lock counts contains the number of extra items placed or
        // removed from the semaphore while the semaphore was locked. When a semaphore is
        // locked items can be added or removed, but the event lists cannot be
        // updated.
        //
        taskENTER_CRITICAL();
        
        --xTxLock;

        // See if items were added to the semaphore while it was locked.
        //
        if( xTxLock > semaUNLOCKED )
        {
            xTxLock = semaUNLOCKED;

            // Items were posted while the semaphore was locked. Are any tasks
            // blocked waiting for items to become available?
            //
            if( ! listLIST_IS_EMPTY( &xTasksWaitingToReceive ) )
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
        }
        
        taskEXIT_CRITICAL ();
    }

    //-----------------------------------------------------------------------------------
    // Uses a critical section to determine if there is certain amount of items in 
    // the semaphore.
    // \return pdTRUE if the semaphore contains items less than needed, otherwise pdFALSE.
    //
    inline bool DoesSemaContains( unsigned portBASE_TYPE count ) const
    {
        taskENTER_CRITICAL();
        bool rc = xItemCount >= (signed portBASE_TYPE)count;
        taskEXIT_CRITICAL();
        return rc;
    }

public:

    //-----------------------------------------------------------------------------------
    // Returns current semaphore count
    //
    inline unsigned portBASE_TYPE GetCount( void ) const
    {
        taskENTER_CRITICAL();
        unsigned portBASE_TYPE uxReturn = xItemCount;
        taskEXIT_CRITICAL();
        return uxReturn;
    }

    //-----------------------------------------------------------------------------------
    static void* operator new( size_t size );

    //-----------------------------------------------------------------------------------
    static void operator delete( void* p );

    //-----------------------------------------------------------------------------------
    xSEMA( unsigned portBASE_TYPE uxInitialCount = 0 );

    //-----------------------------------------------------------------------------------
    void Release( unsigned portBASE_TYPE count = 1 );

    //-----------------------------------------------------------------------------------
    signed portBASE_TYPE ReleaseFromISR( unsigned portBASE_TYPE count, 
            signed portBASE_TYPE xTaskPreviouslyWoken );

    //-----------------------------------------------------------------------------------
    signed portBASE_TYPE Wait( unsigned portBASE_TYPE count, portTickType xTicksToWait, 
            portBASE_TYPE xJustPeeking = pdFALSE );

    //-----------------------------------------------------------------------------------
    signed portBASE_TYPE WaitFromISR( unsigned portBASE_TYPE count );
};

//---------------------------------------------------------------------------------------
// Definition of the mutex used by the scheduler.
//---------------------------------------------------------------------------------------
class xMUTEX : public xSEMA
{
public:

    xMUTEX( void )
        : xSEMA( 1 )
    {
        isMutex = true; 
    }

    bool Lock( const portTickType waitTicks )
    {
        return Wait( 1, waitTicks );
    }

    void Unlock( void )
    {
        Release( 1 );
    }
};

#endif // _SEMA_HPP_INCLUDED

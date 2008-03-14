#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#include <sys/select.h>

int handle = -1;

int open_port( char* port )
{
    struct termios dcb;

    // Open serial port as a file
    //
    handle = open(port, O_RDWR | O_NOCTTY );

    if ( handle < 0 ) 
    {
        printf("Couldn't open serial device!\n");
        return -1;
        }

    // Get current communication state
    //
    tcgetattr(handle, &dcb);

    // Turn on hardware handshake
    //
    //dcb.c_cflag |= CLOCAL;
    dcb.c_cflag |= CRTSCTS;

    cfsetospeed( &dcb, B115200 );
    cfsetispeed( &dcb, B115200 );

    // Set communication state
    //
    if( tcsetattr(handle, TCSANOW, &dcb) )
    {
        perror( "open_port:tcsetattr:" );
        close( handle );
        return -1;
        }

    printf( "Open handle %d\n", handle );
    return 0;
    }

#define outb1000 \
        "test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2" \
        "test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2" \
        "test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2" \
        "test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2" \
        "test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2" \
        "test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2" \
        "test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2" \
        "test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2" \
        "test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2" \
        "test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2test1test2" \

char outb[] =  
outb1000 outb1000 
;

int ser_read( void )
{
    write( handle, outb, sizeof(outb) - 1 );

    char buf[ 10240 ] = { 0 };
    int bufp = 0;

    for ( ;; )
    {
        clock_t t1 = clock();
        fd_set readfds;
        FD_ZERO( &readfds );
        FD_SET( handle, &readfds );

        timeval tvSelect;
        tvSelect.tv_sec = 0;
        tvSelect.tv_usec = 1000000;

        int rc = select( handle+1, &readfds, NULL, NULL, &tvSelect );
        if ( rc < 0 )
        {
            perror( "ser_read:select:" );
            return -1;
            }
        else if ( rc == 0 )
        {
            printf( "SEND_ECHO: Timeout\n" ); fflush( stdout );
            return -1;
            }

        clock_t t2 = clock();
	if( t2 > t1 + 2 ) { printf( " %5d", t2 - t1 ); fflush( stdout ); }


        int len = read( handle, buf + bufp, sizeof(buf) - bufp  );
        bufp += len;

        // printf( "Got: %.*s\n", len, buf );

        if ( memcmp( outb, buf, bufp ) == 0 )
            break;
        }
    }

int main( int argc, char** argv )
{
    int rc;

    int N = 5000;
    if ( argc > 1 ) sscanf( argv[1], "%d", &N );
    printf( "%d Clocks per sec, sizeof outb %d, N = %d\n", CLOCKS_PER_SEC, sizeof( outb ), N );

    if ( open_port( "/dev/ttyS4" ) )
        return -1;

    time_t t1 = time( NULL );
    for ( int i = 0; i < N; i++ )
    {
        ser_read ();
        }
    time_t t2 = time( NULL );

    close( handle );
    printf( "\nDone in %ds: 2 x %.3lf Mbit/s\n", t2-t1, (sizeof(outb)-1)*N*8/1e6/double(t2-t1) );
    return 0;
    }

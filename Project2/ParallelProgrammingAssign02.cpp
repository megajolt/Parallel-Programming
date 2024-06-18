#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

int	NowYear = 2024;		// 2024- 2029
int	NowMonth = 0;		// 0 - 11

float	NowPrecip;		// inches of rain per month
float	NowTemp;		// temperature this month
float	NowHeight = 5;		// grain height in inches
int	NowNumDeer = 2;		// number of deer in the current population
int NowNumWolves = 2;        // number of wolves in the current population

const float GRAIN_GROWS_PER_MONTH =	       12.0;
const float ONE_DEER_EATS_PER_MONTH =		1.0;

const float AVG_PRECIP_PER_MONTH =		7.0;	// average
const float AMP_PRECIP_PER_MONTH =		6.0;	// plus or minus
const float RANDOM_PRECIP =			2.0;	// plus or minus noise

const float AVG_TEMP =				60.0;	// average
const float AMP_TEMP =				20.0;	// plus or minus
const float RANDOM_TEMP =			10.0;	// plus or minus noise

const float MIDTEMP =				40.0;
const float MIDPRECIP =				10.0;

omp_lock_t	Lock;
volatile int	NumInThreadTeam;
volatile int	NumAtBarrier;
volatile int	NumGone;

unsigned int seed = 0;

// specify how many threads will be in the barrier:
//	(also init's the Lock)

void
InitBarrier( int n )
{
        NumInThreadTeam = n;
        NumAtBarrier = 0;
	omp_init_lock( &Lock );
}


// have the calling thread wait here until all the other threads catch up:

void
WaitBarrier( )
{
        omp_set_lock( &Lock );
        {
                NumAtBarrier++;
                if( NumAtBarrier == NumInThreadTeam )
                {
                        NumGone = 0;
                        NumAtBarrier = 0;
                        // let all other threads get back to what they were doing
			// before this one unlocks, knowing that they might immediately
			// call WaitBarrier( ) again:
                        while( NumGone != NumInThreadTeam-1 );
                        omp_unset_lock( &Lock );
                        return;
                }
        }
        omp_unset_lock( &Lock );

        while( NumAtBarrier != 0 );	// this waits for the nth thread to arrive

        #pragma omp atomic
        NumGone++;			// this flags how many threads have returned
}

float
Ranf(float low, float high)
{
    float r = (float)rand();               // 0 - RAND_MAX
    float t = r / (float)RAND_MAX;       // 0. - 1.

    return   low + t * (high - low);
}

float
SQR( float x )
{
        return x*x;
}


void Deer(){
    while( NowYear < 2030 )
    {
        // compute a temporary next-value for this quantity
        // based on the current state of the simulation:
        
        int nextNumDeer = NowNumDeer;
        int numWolves = NowNumWolves;
        int carryingCapacity = (int)( NowHeight );
        if( nextNumDeer < carryingCapacity )
                nextNumDeer++;
        else
                if( nextNumDeer > carryingCapacity)
                        nextNumDeer--;
                if(NowNumWolves > 2)
                        nextNumDeer--;
        if( nextNumDeer < 0 )
                nextNumDeer = 0;
        // DoneComputing barrier:
        WaitBarrier( );
        NowNumDeer = nextNumDeer;

        // DoneAssigning barrier:
        WaitBarrier( );


        // DonePrinting barrier:
        WaitBarrier( );

    }
}

void Grain(){
    while( NowYear < 2030 )
    {
        // compute a temporary next-value for this quantity
        // based on the current state of the simulation:
        float tempFactor = exp(   -SQR(  ( NowTemp - MIDTEMP ) / 10.  )   );

        float precipFactor = exp(   -SQR(  ( NowPrecip - MIDPRECIP ) / 10.  )   );

        float nextHeight = NowHeight;
        nextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
        nextHeight -= (float)NowNumDeer * ONE_DEER_EATS_PER_MONTH;
        if( nextHeight < 0. ) nextHeight = 0.;
        // DoneComputing barrier:
        WaitBarrier( );
        NowHeight = nextHeight;
        // DoneAssigning barrier:
        WaitBarrier( );


        // DonePrinting barrier:
        WaitBarrier( );

    }
}

void Wolves(){
    while( NowYear < 2030 )
    {
        // compute a temporary next-value for this quantity
        // based on the current state of the simulation:
        int nextNumWolves = NowNumWolves;
        int numDeer = NowNumDeer;
        if(nextNumWolves > numDeer*2){
            nextNumWolves--;
        }
        else if(nextNumWolves%2==0 || nextNumWolves<NowNumDeer){
            nextNumWolves++;
        }
        if(nextNumWolves < 0){
            nextNumWolves = 0;
        }
        // DoneComputing barrier:
        WaitBarrier( );

        NowNumWolves = nextNumWolves;
        // DoneAssigning barrier:
        WaitBarrier( );
        
        // DonePrinting barrier:
        WaitBarrier( );
        
    }
}

void Watcher(){
    fprintf(stderr,"Year,Month,Temperature,Precipitation, Grain Height,Deer,Wolves\n");
    while( NowYear < 2030 )
    {
        // compute a temporary next-value for this quantity
        // based on the current state of the simulation:


        // DoneComputing barrier:
        WaitBarrier( );
        

        // DoneAssigning barrier:
        WaitBarrier( );
        fprintf(stderr,"%d,%d,%f,%f,%f,%d,%d\n",NowYear,NowMonth,NowTemp,NowPrecip,NowHeight,NowNumDeer,NowNumWolves);
        NowMonth++;
        if( NowMonth == 12 )
        {
            NowMonth = 0;
            NowYear++;
        }

        float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );	// angle of earth around the sun

        float temp = AVG_TEMP - AMP_TEMP * cos( ang );
        NowTemp = temp + Ranf( -RANDOM_TEMP, RANDOM_TEMP );

        float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin( ang );
        NowPrecip = precip + Ranf( -RANDOM_PRECIP, RANDOM_PRECIP );
        if( NowPrecip < 0. )
            NowPrecip = 0.;

        // DonePrinting barrier:
        WaitBarrier( );
        
    }
}




int
main(int argc, char* argv[])
{
    float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );	// angle of earth around the sun

    float temp = AVG_TEMP - AMP_TEMP * cos( ang );
    NowTemp = temp + Ranf( -RANDOM_TEMP, RANDOM_TEMP );

    float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin( ang );
    NowPrecip = precip + Ranf( -RANDOM_PRECIP, RANDOM_PRECIP );
    if( NowPrecip < 0. )
        NowPrecip = 0.;

    omp_set_num_threads( 4 );	// same as # of sections
    InitBarrier( 4 );
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            Deer( );
        }

        #pragma omp section
        {
            Grain( );
        }

        #pragma omp section
        {
            Watcher( );
        }
        #pragma omp section
        {
            Wolves();
        }
    }	// implied barrier -- all functions must return in order to allow any of them to get past here
}
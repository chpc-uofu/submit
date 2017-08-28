#include <sys/time.h>
#include <unistd.h>
struct timeval _tstart, _tend;
struct timezone tz;

#ifdef AIX
void cgettimeofday(double ttime_[2], int tzone_[2])
#elif CRAY
void CGETTIMEOFDAY(double ttime_[2], int tzone_[2])
#else
void cgettimeofday_(double ttime_[2], int tzone_[2])
#endif
{
	struct timeval _ttime;
	struct timezone _tzone;

        gettimeofday(&_ttime, &_tzone);

/*        printf("%e %e\n",(double)_ttime.tv_sec,(double)_ttime.tv_usec); */

	ttime_[0] = (double)_ttime.tv_sec;	
	ttime_[1] = (double)_ttime.tv_usec;
}

double gettime()
{
        struct timeval _ttime;
        struct timezone _tzone;
	double ttime;

        gettimeofday(&_ttime, &_tzone);

/*        printf("%e %e\n",(double)_ttime.tv_sec,(double)_ttime.tv_usec);  */

        ttime = (double)_ttime.tv_sec+(double)_ttime.tv_usec/1000000.;

        return ttime;
}


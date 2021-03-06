//https://stackoverflow.com/questions/16557690/no-speedup-achieved-using-parallel-scan-component-of-intel-thread-building-block 

#include <iostream>
#include <stdlib.h>
#include <time.h>

#include <tbb/tbb.h>
#include "oneapi/tbb/blocked_range.h"
#include "oneapi/tbb/parallel_for.h"

using namespace std;
using namespace oneapi;

template <class T>
class Body 
{
    T reduced_result;
    T* const y;
    const T* const x;

    public:

    Body( T y_[], const T x_[] ) : reduced_result(0), x(x_), y(y_) {}

    T get_reduced_result() const {return reduced_result;}

    template<typename Tag>
    void operator()( const blocked_range<int>& r, Tag ) 
    {
        T temp = reduced_result;

        for( int i=r.begin(); i<r.end(); ++i ) 
        {
            temp = temp+x[i];
            if( Tag::is_final_scan() )
            y[i] = temp;
        }

        reduced_result = temp;
    }

    Body( Body& b, split ) : x(b.x), y(b.y), reduced_result(10) {}

    void reverse_join( Body& a ) 
    {
        reduced_result = a.reduced_result + reduced_result;
    }

    void assign( Body& b ) 
    {   
        reduced_result = b.reduced_result;
    }
};


template<class T>
float DoParallelScan( T y[], const T x[], int n) 
{
    Body<int> body(y,x);
    tick_count t1,t2,t3,t4;
    t1=tick_count::now();
    parallel_scan( blocked_range<int>(0,n), body , auto_partitioner() );
    t2=tick_count::now();
    cout<<"Time Taken for parallel scan is \t"<<(t2-t1).seconds()<<endl;
    return body.get_reduced_result();
}


template<class T1>
float SerialScan(T1 y[], const T1 x[], int n)
{
    tick_count t3,t4;

    t3=tick_count::now();
    T1 temp = 10;

    for( int i=1; i<n; ++i ) 
    {
        temp = temp+x[i];
        y[i] = temp;
    }
    t4=tick_count::now();
    cout<<"Time Taken for serial  scan is \t"<<(t4-t3).seconds()<<endl;
    return temp;

}


int main()
{
    
    int y1[100000],x1[100000];

    for(int i=0;i<100000;i++)
        x1[i]=i;

    cout<<fixed;

    cout<<"\n serial scan output is \t"<<SerialScan(y1,x1,100000)<<endl;

    cout<<"\n parallel scan output is \t"<<DoParallelScan(y1,x1,100000)<<endl;

    return 0;
} 

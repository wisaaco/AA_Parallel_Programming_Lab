#include <oneapi/tbb/info.h>
#include <oneapi/tbb/parallel_scan.h>
#include <cassert>
#include <iostream>

// https://docs.oneapi.io/versions/latest/onetbb/tbb_userguide/Migration_Guide/Task_Scheduler_Init.html

using namespace oneapi;

int main() {
    // Get the default number of threads
    int num_threads = oneapi::tbb::info::default_concurrency();
    std::cout << "Default concurrency " << num_threads << std::endl;

    const std::vector<double> altitude = [10.,15.,20.0,3.,8.,90.]
    const int dx = 4

    const int N = altidude.size();
    double max_angle = std::atan2(dx,altitude[0]-altitude[1]);

    double final_max_angle = tbb::parallel_scan(
      tbb::blocked_range<int>(1,N), //range
      0.0, //identity
      // scan body
      [altitude,is_visible,dx](const tbb::blocked_range<int> &r,
        double max_angle,
        bool is_final_scan) -> double {
          for (int i=r.begin(); i!= r.end(); i++){
            double angle = std::atan2(i*dx, altitude[0]-altitude[i]);
            if (angle>=max_angle)
              max_angle = angle;
            if (is_final_scan && angle < max_angle)
              is_visible[i] = false;
          }
          return max_angle;
        },
        //reduce
        [](double a,double b){
          return std::max(a,b);
        }
    );



    std::cout << "final_max_angle: " << final_max_angle << std::endl;
    return 0;
}

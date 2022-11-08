//https://oneapi-src.github.io/oneAPI-spec/elements/oneTBB/source/algorithms/functions/parallel_invoke_func.html

#include <iostream>
#include "oneapi/tbb/parallel_invoke.h"

int max(int num1, int num2);

int max(int num1, int num2) {
   int result;
 
   if (num1 > num2)
      result = num1;
   else
      result = num2;
 
   return result; 
}


int main(){
   // https://www.intel.com/content/www/us/en/develop/documentation/advisor-user-guide/top/model-threading-designs/add-parallelism-to-your-program/replace-annotations-with-tbb-code/parallelize-functions-tbb-tasks.html
    
    oneapi::tbb::parallel_invoke(
        [](){std::cout << "Hello " << std::endl;},
        [](){std::cout << "TBB " << std::endl;},
        [](){std::cout << max(10039,128903) << std::endl;},
        [&]{std::cout << "Otra tipo de llamada " <<std::endl;},
        [=]{std::cout << "Thanks! " <<std::endl;}
        );
    return 0;
}
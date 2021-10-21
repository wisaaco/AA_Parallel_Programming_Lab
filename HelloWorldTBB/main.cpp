//https://oneapi-src.github.io/oneAPI-spec/elements/oneTBB/source/algorithms/functions/parallel_invoke_func.html

#include <iostream>
#include "oneapi/tbb/parallel_invoke.h"

int max(int num1, int num2);


int main(){
    oneapi::tbb::parallel_invoke(
        [](){std::cout << "Hello " << std::endl;},
        [](){std::cout << "TBB " << std::endl;},
        [](){std::cout << max(10039,128903) << std::endl;}
    );
    return 0;
}

int max(int num1, int num2) {
   int result;
 
   if (num1 > num2)
      result = num1;
   else
      result = num2;
 
   return result; 
}

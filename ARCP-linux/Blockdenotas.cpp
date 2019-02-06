// basic file operations
#include <iostream>
#include <fstream>
#include <boost/lexical_cast.hpp>
#define MASK 7
using namespace std;

int main () {
  ofstream myfile;
  myfile.open ("example.txt");
  myfile << "Writing this to a file gg.\n";





struct Vector{
    double* x;
    int n;
};

struct Vector *y = (struct Vector*)malloc(sizeof(struct Vector));


y->x = (double*)malloc(10*sizeof(double));

myfile << y;

printf("El valor de a es: %d. \n",*y);
printf("El valor de a es: %d. \n",y);
printf("El valor de a es: %d. \n",&y);
  myfile.close();
  
  
  
  
  return 0;
}

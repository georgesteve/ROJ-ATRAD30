
#include "SSTmanager.h"
#include <stdio.h>  
#include <iostream>
#include <stdint.h>
#include <inttypes.h>
#include <cstdlib>
#include <boost/lexical_cast.hpp>
#define MASK 7
arcp_handle_t *handle; 
using namespace std;
int main(int argc, char *argv[])
{   
    signed int result;
    
    arcp_stx2stat_t *stat(void);
    arcp_stx2stat_t status;
    printf("Temp: %i \n",status.ambient_temp);
    //arcp_sysid_t *sysid;
    //struct arcp_sysid_t *sysid=malloc(sizeof(struct arcp_sysid_t));
    //struct Vector *y = (struct Vector*)malloc(sizeof(struct Vector));
    //y->x = (double*)malloc(10*sizeof(double));
    
    //result=arcp_get_sysid(handle, &sysid);
    
  
    uint32_t arcp_get_lib_version(void);
    unsigned int arcp_get_lib_proto_version(void);
    printf("version= %"  PRIu32 "\n",*arcp_get_lib_proto_version);
    //printf( "%u  \n" , (unsigned int) arcp_get_lib_proto_version);    
    printf("Fin");
    return 0;
      
      }

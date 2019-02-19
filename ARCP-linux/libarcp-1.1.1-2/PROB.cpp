
#include "SSTmanager.h"
  
#include <stdio.h>
#include <iostream>
#include <stdint.h>
#include <inttypes.h>
#include <fstream>

#define MASK 7





//Define un modo de monitoreo por batch con el siguiente orden: arcp.exe + TX destino + ruta del archivo txt, otro tipo de llamado a la función no funcionará 

arcp_handle_t *handle; 

using namespace std;

int main()

{
    ofstream ArchivoTexto;
    ArchivoTexto.open("example.txt") ;
    
    //Get status variables
    
    arcp_sysstat_t *statusResult,**ptrAux;
    ptrAux = &statusResult;
    
    signed int result;
    result= arcp_get_sysstat(handle,ptrAux);
    
    
       uint32_t arcp_get_lib_version(void);
    unsigned int arcp_get_lib_proto_version(void);
    
    printf("version= %"  PRIu32 "\n", *arcp_get_lib_proto_version);
    printf( "%u \n" , (unsigned int) arcp_get_lib_proto_version);

    
   return 0;
      
      }

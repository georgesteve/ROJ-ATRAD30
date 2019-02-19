
#include "SSTmanager.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <inttypes.h>
#include <cstdlib>
#include <string> 





//Define un modo de monitoreo por batch con el siguiente orden: arcp.exe + TX destino + ruta del archivo txt, otro tipo de llamado a la función no funcionará 

arcp_handle_t *handle; 

using namespace std;

int main()

{   ofstream myfile;
    myfile.open ("example1.csv");
    //ofstream ArchivoTexto;
    
    //Get status variables
    
    arcp_sysstat_t *statusResult,**ptrAux;
    ptrAux = &statusResult;
    
    signed int result;
   
    arcp_stx2stat_t *stat(void);
    arcp_stx2stat_t status;
    status.ambient_temp=12;
    status.status_code=0X03;

    std::string s = std::to_string(status.ambient_temp);
    printf("Power,Temp\n");
    printf("\n123,%i \n",status.ambient_temp);
    printf("%u\n",(unsigned int)status.status_code);
    
      
    myfile <<"Power,Temp\n";
    myfile <<"123,";
    myfile << s;
  
    uint32_t arcp_get_lib_version(void);
    unsigned int arcp_get_lib_proto_version(void);
    
    printf("version= %"  PRIu32 "\n", *arcp_get_lib_proto_version);
    printf( "%u \n" , (unsigned int) arcp_get_lib_proto_version);
   


    myfile.close();
    
   return 0;
      
      }

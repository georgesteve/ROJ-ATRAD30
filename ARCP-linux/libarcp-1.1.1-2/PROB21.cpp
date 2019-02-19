
#include "SSTmanager.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <inttypes.h>
#include <cstdlib>
#include <string> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#define defaultPort 49490



//Define un modo de monitoreo por batch con el siguiente orden: arcp.exe + TX destino + ruta del archivo txt, otro tipo de llamado a la función no funcionará 

arcp_handle_t *handle; 

using namespace std;

char const *ip_addr= "172.16.16.10";

int main()


{   
    signed int sock;
    struct sockaddr_in serv_addr;                           //Connection information struct
    
    
    
    

    
    memset(&serv_addr, 0, sizeof(serv_addr));                //Se rellena con 0s los espacios restantes
    serv_addr.sin_family=AF_INET;                            //Para TCP se usa por defecto AF_INET
    
    
    if(inet_aton(ip_addr, &serv_addr.sin_addr) == 0){
        cout<<"gg";
        return NULL; 
        
         }
         
    serv_addr.sin_port=htons(49490);
    
    sock = socket(PF_INET,SOCK_STREAM,0);
    if(sock <0) {
        return NULL; 
        }
    
    
    
    
    if(connect(sock, (sockaddr *)&serv_addr, sizeof(sockaddr))>0){
        return NULL; 
        //close(sock);
        }
        
    handle=arcp_handle_new(sock);
    if(handle == NULL){
        return NULL; 
        }
    
     signed int result = arcp_ping(handle)

    cout<<result;

      }

      
      

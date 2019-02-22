
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
#include <unistd.h>


#define defaultPort 49490



//Define un modo de monitoreo por batch con el siguiente orden: arcp.exe + TX destino + ruta del archivo txt, otro tipo de llamado a la función no funcionará 

arcp_handle_t *handle;

using namespace std;

char const *ip_addr= "172.16.16.5";
signed int  result ,resultsys;

int i,includedFans,cards_included;                                   //Arreglos para datos del transmisor

int main()


{   int a=0;
    while ( a<1){
    
           

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
        cout<<"Esperando";
        return NULL; 
        }
    
    
    
    
    if(connect(sock, (sockaddr *)&serv_addr, sizeof(sockaddr))>0){
        cout<<"Esperando1";
        return NULL; 
        //close(sock);
        }
        
    handle=arcp_handle_new(sock);
    
    if(handle == NULL){
        cout<<"Esperando2";
        return NULL; 
        }
    
     result = arcp_ping(handle);
     //printf( "%u \n" , (unsigned int) result);
    

    arcp_sysstat_t *sysstat;
    
    resultsys= arcp_get_sysstat(handle, &sysstat);
     //printf( "%u \n" , (unsigned int) resultsys);
    
    //cout<<unsigned(resultsys.module_status);
    
    //int8_t m=123546;    
    //cout<<unsigned(m)<<endl;
        
 
  
    if(resultsys !=0){
          cout<<"Esperando3";}
    else{    ofstream archivotexto;
            archivotexto.open ("ATRADvalues.csv");
        
       cout<<unsigned(sysstat->module_status)<<endl;
       cout<<unsigned(sysstat->data.stx2->n_chassis_fans)<<endl;
       cout<<unsigned(sysstat->data.stx2->ambient_temp*1)<<endl;
       cout<<unsigned(sysstat->data.stx2->card_map)<<endl;
       cout<<unsigned(sysstat->data.stx2->status_code)<<endl;
       
       includedFans = (int) sysstat->data.stx2->n_chassis_fans;
       cards_included= (int) sysstat->data.stx2->n_rf_cards;
       
       archivotexto<<"Temperature,Status,forward_power1,retrun_loss1,forward_power2,retrun_loss2,forward_power3,retrun_loss3\n";
       archivotexto<<sysstat->data.stx2->ambient_temp*1<<",";
       archivotexto<<sysstat->data.stx2->status_code<<",";
       
       for (i=0; i < cards_included; i++)
            {                   
               
                cout<<unsigned(sysstat->data.stx2->rf_card_stat[i].output_stat->forward_power)<< "\n";    //"Potencia incidente:" 
                cout<<unsigned(sysstat->data.stx2->rf_card_stat[i].output_stat->return_loss)<< "\n";      //"Pérdida de retorno:" 
                archivotexto<<sysstat->data.stx2->rf_card_stat[i].output_stat->forward_power<<",";
                archivotexto<<sysstat->data.stx2->rf_card_stat[i].output_stat->return_loss<<",";
                
            }
       
       
        archivotexto<<"/n";
       
       
       
       
       
       archivotexto.close();  
       
       cout<<"fin";
        }
   
  
    sleep(2);
   }
  
      }

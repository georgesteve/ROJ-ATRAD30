#define MASK 7

//Define un modo de monitoreo por batch con el siguiente orden: arcp.exe + TX destino + ruta del archivo txt, otro tipo de llamado a la función no funcionará 

arcp_handle_t *handle; 

using namespace std;

int main(int argc, char *argv[])

{      //ofstream ArchivoTexto;
    //ofstream ArchivoTexto;
    
        //Get status variables
    signed int resultado;

    arcp_sysstat_t *sysstat;
    
    
    resultado=arcp_get_sysstat(handle, &sysstat);
    
    //if (resultado == 0)                       //Configuración del trigger fue exitoso
    //    {
          //  resultado = arcp_set_module_enable(handle,1);          //Habilitar el transmisor
         //   if (resultado != 0)
           // {
              ////  printf("Codigo error al habilitar modulo: %d\n",resultado);
             //   return false;
           // }else{
            //    return true;
            //}
      //  }else{
            //printf("Codigo error al configurar trigger: %d\n",resultado);
            //return false;
     //   }  
    
    uint32_t arcp_get_lib_version(void);
    unsigned int arcp_get_lib_proto_version(void);
    
    printf("version= %"  PRIu32 "\n",*arcp_get_lib_proto_version);
    printf( "%u \n" , (unsigned int) *arcp_get_lib_proto_version);
  
  
  
    //string a;

    //a=boost::lexical_cast<string>(statusResult->data.stx2->status_code & MASK);                                //"Código de estado:
            
    
    
   return 0;
      
      }

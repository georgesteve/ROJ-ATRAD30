/*Esta libreria esta compuesta de dos clases, la clase arcpConnecction, que se utiliza para establecer
una conexion adecuada utilizando Sockets, en ella se hace uso de de tres metodos, createSocket, que se utiliza
para crear un socket por el cual se enviaran los datos al dispositivo, closeSocket, que se utiliza para cerrar
el socket abierto para la comunicacion, y createArcpHandle, el cual asigna el socket creado para el manejo en
el codigo entregado con el fabricante.
La otro clase es arcpCommand, el cual se utiliza especificamente para el envio de commandos con el protocolo ARCP
al dispositivo, en ella se instancia a un objeto de la clase arcpConnection, para poder crear un socket adecuado
y actualmente solo esta compuesto de un metodo, getAtradStatus el cual se encarga de solicitar los parametros del
estado del transmisor.*/


#include "SSTmanager.h"

    arcp_handle_t *handle;              //Variable global que contiene conexión ARPC




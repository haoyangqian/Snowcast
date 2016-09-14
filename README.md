# Snowcast
Project in Computer Networks

#snowcast_controller
Receive the arguments as hostname, portNumber, udpPort.

Use open_client() to create socket and connect to Server,and return the sockfd.

After connecting to the server Successfully,use send_hello() to send a HELLO command to Server.

Use FD_SET and select() to controll streams from stdin and sockfd.

After sending HELLO,controller should receive WELCOME from Server using recv_welcome().

Set the STATIONNUM by send_station() then receive the ANNOUCE or INVALID_COMMAND_REPLY by recv_string().


#snowcast_listener
Receive the arguments as udpPort.

Use open_client() to create socket and bind this socket to udpPort.

Receive Bytes from Server continuiously and write them to stdout.

#snowcast_server

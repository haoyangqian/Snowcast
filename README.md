***Snowcast***

Project in Computer Networks Course

Author:Haoyang Qian

Login:hqian1

###snowcast_server
The server has three type of thread.

####Main thread
Function:
* accept user input and assign space for the global variable according to the numer of stations
* initialize the stations information and create threads for every station
* open socket to listen new connections from clients
* handle user input like 'p'(print stations info) and 'q' (quit)

In order to handle stream both from listenfd and stdin at the same time,we use ``fd_set`` and ``select()`` function.First,we use ``FD_SET()``  function to  assign listenfd and stdin as readable fd.Then,user ``select()`` to listen to both stream.When either stream receives an input,we can handle it. 

In stdin,Server only accepts two input:``'p'`` and ``'q'``.(1)'p':first lock the global variable of stations,print all the station info including the song name and the listening clients address and port.(2)'q' will close all the clients and release all the resourses then exit.

In listenfd,once ``select()`` finds a input from listenfd,which means a client is trying to connect to the server,we use ``accept()`` to connect this client.Then create a thread to handle this client.

####Station thread
Number: every station have a thread

Function:
* read music file
* broadcast music to udp listener

In order to keep a rate of ``16KiB/s``,station should read the song file to buffer 16 times per second and every time the buffer length should be 1024KB.Before we send bytes to every clients,we clock time once to record the start time.Then after send() we clock the time again to record the end time,then you got the ``sendtime`` between start and end.Then use ``nanosleep()`` to sleep ``1/16s - sendtime``,which could keep a rate of 16KiB/S.

Once a music file is over,the thread should restart this song(using ``rewind()``),and lock the station info then send announce to every client who is listening to this station.

When send to listeners,fisrt lock the station info,then iterate all the clients in the linked list then send buffer to the udpPort of every client.

####Client thread
Number:every client have a thread

Function:
* receive command from clients and send command to clients
* set station for every client
* receive invalid command and return invalid reply

First,thread receive HELLO command from client,then return a WELCOME command with the total number for stations.

Second,thread wait for a SET_STATION command,after successfully received the correct SET_STATION command,thread call ``unset_station()`` first to remove the client from the previous station(if there's no station this client is listening,do nothing),then call ``set_station()`` to add this client to the new station.In both ``unset_station()`` and ``set_station()`` ,thread would lock the stations variable to pretect the shared data.

After successfully set to a new station.thread send a ANNOUNCE command with the song name to the client.

####Invalid command
Thread use state value and return types to prevent Invalid command and wrong order of command, we have ``DEFAULT``,``WAIT_WEL``,``WAIT_ANC``,``RCV_ANC`` which indicates different state of client thread.And we have ``DUP_HELLO``,``INVALID_CMD``,``CLI_CLOSE``,``WRONG_STATION``which indicates the different types of return value.

For example,if a thread receive a HELLO when it is supposed to receive a SET_STATION,it will return a DUP_HELLO error and close the connection.And if the station number which received from client is not in range of the server's station number,it will return a WRONG_STATIONNUM eroor and send the INVALID_COMMAND to client,then close the connection to this client.

####Timeout
Server check timeout when receive part of command from client.

When receving HELLO or SET_STATION,thread found that the bytes received is less than the bytes we expected,then it calls ``poll()`` to wait for the other bytes,and if it does not receive the other bytes in one particular time(We set it to 100ms),then we timeout this connection,print error and close the connection.

And we will also timeout the connection that if the client is not sending HELLO to the server in a particular time (We set it to 100ms) after the connection was build.



###snowcast_controller
Function:
* connect to the server and send HELLO command
* receive WELCOME command and get the number of stations
* accept user input to set a station number and send SET_STATION command to server
* receive ANNOUNCE command from server

First,receive the arguments as hostname, portNumber, udpPort.Use ``open_client()`` to create socket and connect to Server,and return the sockfd.After connecting to the server Successfully,use ``send_hello()`` to send a HELLO command to Server.After sending HELLO,controller should receive WELCOME from Server using ``recv_welcome()``.

In order to handle stream both from ``sockfd`` and ``stdin`` at the same time,we use ``fd_set`` and ``select()`` function.
From stdin stream we only accept number or 'q'.After user entered a valid number for station,call ``send_station()`` to send command to the server.From sockfd stream,controller receive the ANNOUCE or INVALID_COMMAND_REPLY by ``recv_string()``.

####Invalid command
We have ``DEFAULT``,``WAIT_WEL``,``WAIT_ANC``to represent the state for controller.

When we receive any command with replytype other than '0'or'1'or'2',we return a unknown response error and close the connection.when we receive a command with replytype 2,we print the replystring and close the connection.
####Timeout
Like the server,controller will timeout the connection when receive part of command.

When receving WELCOME,controller found that the bytes received is less than the bytes we expected,then it calls ``poll()`` to wait for the other bytes,and if it does not receive the other bytes in one particular time(We set it to 100ms),then we timeout this connection,print error and close the connection.

when receiving ANNOUNCE or INVALID_COMMAND_REPLY,controller first reveive 2 bytes of the buffer to get the expected length of this message,then check if the received bytes is less than expected,if so,it calls ``poll()`` to wait for the other bytes,and if it does not receive the other bytes in one particular time(We set it to 100ms),then we timeout this connection,print error and close the connection.

###snowcast_listener
Function:
* receive the music from server and write them to stdout
* keep the rate as 16KiB/S

Receive the arguments as udpPort.Use ``open_client()`` to create socket and bind this socket to udpPort.Receive Bytes from Server continuiously and write them to stdout.

To keep the rate as 16KiB/S,use the same way as server does.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h> 

#define max_buffer_size 1024

int main(int argc, char *argv[])
{
    if(argc != 3)               //Invalid input by user
    {
        printf("\n Input format: %s <Server IP address> <Server port number> \n",argv[0]);
        return 1;
    } 

    int socket_id = socket(AF_INET, SOCK_STREAM, 0);    //socket file descriptor for client
    if (socket_id < 0)                                  // failed to get valid socket file descriptor
    {
        printf("\n Socket creation error, terminating! \n");
        return -1;
    }

    struct sockaddr_in server_address;
    
    server_address.sin_family = AF_INET;                //initializing peer_node_address attributes
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(atoi(argv[2]));

    char buffer[max_buffer_size] = {0};

    if(inet_pton(AF_INET, argv[1], &server_address.sin_addr)<=0)
    {
        printf("\n Invalid server IP, terminating!\n");
        return -1;
    }

    if( connect(socket_id, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)  //Connecting with relay sever
    {
       printf("\n Unable to connect to the server, terminating! \n");
       return -1;
    }

    // Requesting response from relay server
    char * message = "0#Hi there! This is the client." ;
    int message_len = strlen(message);
    
    if( send(socket_id, message, message_len,0)!= message_len){
        perror("Message not sent from client to server\n");
        return -1;
    }

    //Response from relay server
    if(recv(socket_id,buffer,max_buffer_size,0) < 0)
    {
        perror("Message sent by server, not received by client\n");
        return -1;
    }

    //printf("%s\n",buffer);
    char *temp = strtok(buffer,"$");
    printf("Server says: %s\n\n",temp);
    temp=strtok(NULL,"$");
    
    int peernode_count = 0;
    
    peernode_count = atoi(strtok(temp,":"));
    printf("PeerNode Count : %d\n\n",peernode_count);
    

    temp = strtok(NULL, ":");

    //array to store ports of peer nodes
    char* peernode_ports[peernode_count];
    //array to store addresses of peer nodes
    struct sockaddr_in peernode_address[peernode_count];
    int i = 0;
    
    while(temp != NULL) //extracting peer node addresses and ports from message sent by relay server
    {   
        peernode_address[i].sin_family = AF_INET;
        
        if(inet_pton(AF_INET, temp, &(peernode_address[i].sin_addr.s_addr)) < 0) 
        {
            printf("Invalid Address\n");
            
            return -1;
        }    
        temp = strtok(NULL, ":");
        
        peernode_address[i].sin_port = htons(atoi(temp));
        temp = strtok(NULL, ":");
        i++;
    }

    // Details of all peer nodes

    for(i=0; i< peernode_count;i++)
    {   
        char ip_addr[max_buffer_size];
        printf("PeerNode IP: %s\n",inet_ntop(AF_INET, &(peernode_address[i].sin_addr), ip_addr, max_buffer_size));
        printf("PeerNode port: %d\n",ntohs(peernode_address[i].sin_port));
    }

    close(socket_id); 

    // Phase three
    
    char filename[max_buffer_size];
    printf("Enter the name of the file: \n");
    scanf("%s",filename); //Filename input from user
    
    int found=0;
    for(i=0;i<peernode_count;i++){ //search in all peer nodes

        char ip_addr[max_buffer_size];
        printf("Peer node number:%d\n",i+1);
        printf("Peer node port: %d\n",ntohs(peernode_address[i].sin_port));
        printf("Peer node IP: %s\n",inet_ntop(AF_INET, &(peernode_address[i].sin_addr), ip_addr, max_buffer_size));

        if ((socket_id = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("\n Socket creation error \n");
            return -1;
        }

        // Connecting with peer nodes

        if( connect(socket_id, (struct sockaddr *)&(peernode_address[i]), sizeof(peernode_address[i])) < 0)
        {
           printf("\n Couldn't connect to peer node\n");
           return -1;
        }

        printf("Connected to peer node\n");

        if( send(socket_id, filename, strlen(filename),0)!=strlen(filename)) //sending filename to peer node
        {
            perror("Couldn't send filename\n");
            return -1;
        }

        memset(buffer,'\0',max_buffer_size);

        if (recv(socket_id, buffer, 1024, 0) < 0)
        {
            perror("Receive error\n");
            exit(-1);
        }

        long int filesize;

        char *temp = strtok(buffer, "@");
        char * found_in_peer = temp;

        if(*found_in_peer=='0') //File not found
        {
            printf("File not found in peer node number %d\n\n",i+1);
            close(socket_id);
            continue;
        }

        printf("File found in peer node number %d\n",i+1);
        found= 1; //Found in atleast one peer node

        if(temp != NULL){
            temp = strtok(NULL,"$");
            filesize= atoi(temp);
        }

        long int remaining_bytes =filesize; //number of bytes left to be transferred

        printf("File size = %ld \n",filesize);

        FILE *rcvd_file=fopen(filename,"w");
        int len;

        while( remaining_bytes >0 && ( (len=recv(socket_id, buffer, 1024, 0)) > 0))
        {
            fwrite(buffer, sizeof(char), len, rcvd_file);
            remaining_bytes  -= len;
            printf("Buffer currently contains: %s \n",buffer); //denotes current buffer
            printf("Received = %d bytes, Remaining bytes  = %ld bytes \n",len, remaining_bytes );//show updates on bytes received and left
        }

        printf("File transfer completed\n\n"); //File completely transferred

        fclose(rcvd_file); //close file
        close(socket_id);  //close socket
    }

    if(found==0) //found not found in any of the peers
    {
        printf("File not found in all peer_nodes\n");
    }

    return 0;
}


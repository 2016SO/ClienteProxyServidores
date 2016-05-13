int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno, pid;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;

     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     while (1) {
         newsockfd = accept(sockfd, 
               (struct sockaddr *) &cli_addr, &clilen);
         if (newsockfd < 0) 
             error("ERROR on accept");
         pid = fork();
         if (pid < 0)
             error("ERROR on fork");
         if (pid == 0)  {
             close(sockfd);
             dostuff(newsockfd);
             exit(0);
         }
         else close(newsockfd);
     } /* end of while */
     close(sockfd);
     return 0; /* we never get here */
}

/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void dostuff (int sock)
{
   int n, p;
   char buffer[256];
   char request;
   FILE *file; 
   file = fopen("process.log","a+");

   do
   {
       //here the proxy server receives data from the client
   bzero(buffer,256);
   p = read(sock,buffer,255);
   if (n < 0) error("ERROR reading from socket");

   printf("num: %s\n",buffer);

       //here the proxy servers replies to the client.
   n = write(sock,buffer,sizeof(buffer));

       //here the process should send data to the server
       //...codes i need help with...           

   if (n < 0) error("ERROR writing to socket");
   fprintf(file,"%s\n",buffer); /*writes*/ 

   }while(p != 0); //this runs the process +1 more than it should. wonder why?

   fclose(file);
}
// stds
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <pthread.h>

#include <arpa/inet.h>

// socks
#include <sys/socket.h>
#include <netinet/in.h>
#define VideoPORT 8888
#define VideoMetPort 8889
#define VideoMetPort2 8890
#define OBPORT 8891
#define BUFLEN 1024
#define SA struct sockaddr

// mem
//

// Declarations
    bool run = false;
    const char* exitstr = "exit";
    //char msg[44] = {0};
    //char arguments[7] = {0};
    int env = 20;
    bool fullrun, enable_vid_dat, enable_vid_met, enable_backend1, enable_backend2, enable_fifo;
    char boolCheck;
    bool cont = false;

    // Video data sock
    bool runVideo1 = true;
    int sock_video_data, valread, recv_len ;
    struct sockaddr_in addressVidRec, si_other;
    int opt = 1;
    int addrlen = sizeof(addressVidRec);
    int slen = sizeof(si_other);
    char buffer[1024] = {0};

    // Video meta sock
    bool runVideoMeta1 = true;
    int sock_video_meta, vidcl, video_meta_len;
    struct sockaddr_in servaddr_vid_met, cli_vid_met;
    char bufferMeta1[1024] = {0};
    int slenMet1 = sizeof(si_other);
    // Video meta sock 2
    int sock_video_meta2, vidcl2, video_meta_len2;
    struct sockaddr_in servaddr_vid_met2, cli_vid_met2;
    char bufferMeta2[1024] = {0};
    int slenMet2 = sizeof(si_other);

    // on board sock
    bool runOB = true;
    int sock_ob, backendcl, backend_len;
    struct sockaddr_in servaddr_backend, cli_backend;
    char bufferOB[1024] = {0};
    int slenOB = sizeof(si_other);


void *videoRecThread(void *vargp)
{
    printf("VideoRecThread started with id: %lu\n", pthread_self());
    while (runVideo1)
    {
        printf("Waiting for data...");
        fflush(stdout);
        //try to receive some data, this is a blocking call
	    if ((recv_len = recvfrom(sock_video_data, buffer, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
	    {
		    perror("Video recieve failed");
            exit(EXIT_FAILURE);
	    }
	    //print details of the client/peer and the data received
	    printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
	    printf("Data: %d\n" , atoi(buffer));

        if (enable_fifo)
        {

        }

	    //now reply the client with the same data
	    if (sendto(sock_video_data, buffer, recv_len, 0, (struct sockaddr*) &si_other, slen) == -1)
	    {
	    	//die("sendto()");
	    }

   	    memset(buffer, 0, 1024);
    }
}

void *videoMetaThreadCli(void *vargp)
{
    printf("VideoMeta1Thread started with id: %lu\n", pthread_self());

    while (runVideoMeta1)
    {
        printf("Waiting for data...");
        fflush(stdout);
        //try to receive some data, this is a blocking call
        valread = read( vidcl , bufferMeta1, 1024);
        printf("%s\n",bufferMeta1 );
        // redirect data
        if (sendto(vidcl2, bufferMeta1, video_meta_len, 0, (struct sockaddr*) &cli_vid_met2, slenMet2) == -1)
	    {
	    	printf("data redirection to backend from video failed\n");
	    }
        //send(new_socket , hello , strlen(hello) , 0 );
        //printf("Hello message sent\n");
        /*
	    if ((video_meta_len = recvfrom(sock_video_meta, bufferMeta1, BUFLEN, 0, (struct sockaddr *) &cli_vid_met, &slenMet1)) == -1)
	    {
		    perror("VideoMeta recieve failed");
            exit(EXIT_FAILURE);
	    }
	    //print details of the client/peer and the data received
	    printf("Received packet from %s:%d\n", inet_ntoa(cli_vid_met.sin_addr), ntohs(cli_vid_met.sin_port));
	    printf("Data: %d\n" , atoi(bufferMeta1));
        */

   	    memset(bufferMeta1, 0, 1024);
    }
}

void *videoMetaThreadBackend(void *vargp)
{
    printf("VideoMeta2Thread started with id: %lu\n", pthread_self());

    while (runVideoMeta1)
    {
        printf("Waiting for data...");
        fflush(stdout);
        //try to receive some data, this is a blocking call
        valread = read( vidcl2 , bufferMeta2, 1024);
        printf("%s\n",bufferMeta2 );
        // redirect data
        if (sendto(vidcl, bufferMeta2, video_meta_len2, 0, (struct sockaddr*) &cli_vid_met, slenMet1) == -1)
	    {
	    	printf("data redirection to video from backend failed\n");
	    }

   	    memset(bufferMeta2, 0, 1024);
    }
}

void *OBThreadBackend(void *vargp)
{
    while (runOB)
    {
        printf("Waiting for data...");
        fflush(stdout);
        //try to receive some data, this is a blocking call
        valread = read( backendcl , bufferOB, 1024);
        printf("%s\n",bufferOB );
        // redirect data
	    printf("Fifo pointers not yet implemented. Data recieved is %s\n", bufferOB);

   	    memset( bufferOB, 0, 1024);
   	}
}

void *OBThreadFIFO(void *vargp)
{

}

int main(int argc, char const *argv[])
{
    printf("Initiating setup.\n");
    while (env != 0 && env != 1)
    {
        printf("Please choose running enviroment(0=arm processor, 1=testing enviroment)\n");
        scanf(" %d", &env);
    }
    while (!run)
    {
        printf("Open all bridges? (y/n)\n");
        scanf(" %c", &boolCheck);
        if (boolCheck == 'y')
        {
            fullrun = true;
            run = true;
        }
        else if (boolCheck == 'n')
        {
            fullrun = false;
            run = true;
        }
        else
        {
            printf("Please enter valid response\n");
        }
        boolCheck = 'r';
    }
    if (fullrun)
    {
        enable_vid_dat = true;
        enable_vid_met = true;
        enable_backend1 = true;
        enable_backend2 = true;
        enable_fifo = true;
    }
    else
    {
        cont = false;
        while(!cont)
        {
            printf("enable video data? (y/n)\n");
            scanf(" %c", &boolCheck);
            if (boolCheck == 'y'){
                enable_vid_dat = true;
                cont = true;
            }else if (boolCheck == 'n'){
                enable_vid_dat = false;
                cont = true;
            }else
                printf("Please enter valid response\n");
            boolCheck = 'r';
        }
        cont = false;
        while(!cont)
        {
            printf("enable video Meta clientside? (y/n)\n");
            scanf(" %c", &boolCheck);
            if (boolCheck == 'y'){
                enable_vid_met = true;
                cont = true;
            }else if (boolCheck == 'n'){
                enable_vid_met = false;
                cont = true;
            }else
                printf("Please enter valid response\n");
            boolCheck = 'r';
        }
        cont = false;
        while(!cont)
        {
            printf("enable video Meta backend? (y/n)\n");
            scanf(" %c", &boolCheck);
            if (boolCheck == 'y'){
                enable_backend1 = true;
                cont = true;
            }else if (boolCheck == 'n'){
                enable_backend1 = false;
                cont = true;
            }else
                printf("Please enter valid response\n");
            boolCheck = 'r';
        }
        cont = false;
        while(!cont)
        {
            printf("enable fifo backend? (y/n)\n");
            scanf(" %c", &boolCheck);
            if (boolCheck == 'y'){
                enable_backend2 = true;
                cont = true;
            }else if (boolCheck == 'n'){
                enable_backend2 = false;
                cont = true;
            }else
                printf("Please enter valid response\n");
            boolCheck = 'r';
        }
        cont = false;
        while(!cont)
        {
            printf("enable fifo? (y/n)\n");
            scanf(" %c", &boolCheck);
            if (boolCheck == 'y'){
                enable_fifo = true;
                cont = true;
            }else if (boolCheck == 'n'){
                enable_fifo = false;
                cont = true;
            }else
                printf("Please enter valid response\n");
            boolCheck = 'r';
        }
    }


    // thread pointers
    printf("Creating thread pointers\n");
    pthread_t thread_id_vid, thread_id_met_vid, thread_id_met_vid2, thread_id_OB;


    // Creating sock descs
    printf("Creating socket descriptors\n");
    printf("(1/4) - Video Data recieve socket initing\n");
    // Video data recieve
    if ((sock_video_data = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("video data socket failed");
        exit(EXIT_FAILURE);
    }


    // Socket-port 8888
    if (setsockopt(sock_video_data, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    addressVidRec.sin_family = AF_INET;
    addressVidRec.sin_addr.s_addr = INADDR_ANY;
    addressVidRec.sin_port = htons( VideoPORT );

    // Forcefully attaching socket to the port 8888
    if (bind(sock_video_data, (SA*)&addressVidRec,
                                 sizeof(addressVidRec))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Video meta
    printf("(2/4) - Video Meta socket video side initiating\n");
    if ((sock_video_meta = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("video Meta socket failed");
        exit(EXIT_FAILURE);
    }
    // assign IP, PORT
    servaddr_vid_met.sin_family = AF_INET;
    servaddr_vid_met.sin_addr.s_addr = INADDR_ANY;
    servaddr_vid_met.sin_port = htons( VideoMetPort );

    // Binding newly created socket to given IP and verification
    if ((bind(sock_video_meta, (SA*)&servaddr_vid_met, sizeof(servaddr_vid_met))) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("(3/4) - Video Meta socket backend side initiating\n");
    if ((sock_video_meta2 = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("video Meta socket failed");
        exit(EXIT_FAILURE);
    }
    // assign IP, PORT
    servaddr_vid_met2.sin_family = AF_INET;
    servaddr_vid_met2.sin_addr.s_addr = INADDR_ANY;
    servaddr_vid_met2.sin_port = htons( VideoMetPort2 );

    // Binding newly created socket to given IP and verification
    if ((bind(sock_video_meta2, (SA*)&servaddr_vid_met2, sizeof(servaddr_vid_met2))) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("(4/4) - Backend socket initiating\n");
    if ((sock_ob = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("on board comms socket failed");
        exit(EXIT_FAILURE);
    }
    // assign IP, PORT
    servaddr_backend.sin_family = AF_INET;
    servaddr_backend.sin_addr.s_addr = INADDR_ANY;
    servaddr_backend.sin_port = htons( OBPORT );


    // Binding newly created socket to given IP and verification
    if ((bind(sock_ob, (SA*)&servaddr_backend, sizeof(servaddr_backend))) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Sockets declared\n");


    if (enable_vid_met) {
        if ((listen(sock_video_meta, 5)) != 0) {
            printf("Listen failed...\n");
            exit(0);
        }
        else
            printf("VideoMeta1Thread %lu listening for video client..\n", pthread_self());
        video_meta_len = sizeof(cli_vid_met);

        vidcl = accept(sock_video_meta, (SA*)&cli_vid_met, &video_meta_len);
        if (vidcl < 0) {
            printf("server acccept failed...\n");
            exit(0);
        }
        else
            printf("server acccept the client...\n");
    }

    if (enable_backend1)
    {
        if ((listen(sock_video_meta2, 5)) != 0) {
            printf("Listen failed...\n");
            exit(0);
        }
        else
            printf("VideoMeta1Thread %lu listening for backend client..\n", pthread_self());
        video_meta_len2 = sizeof(cli_vid_met2);

        vidcl2 = accept(sock_video_meta2, (SA*)&cli_vid_met2, &video_meta_len2);
        if (vidcl2 < 0) {
            printf("server acccept failed...\n");
            exit(0);
        }
        else
            printf("server acccept the client...\n");
    }

    if (enable_backend2)
    {
        if ((listen(sock_ob, 5)) != 0) {
            printf("Listen failed...\n");
            exit(0);
        }
        else
            printf("On board socket %lu listening for backend client..\n", pthread_self());
        backend_len = sizeof(cli_backend);

        backendcl = accept(sock_ob, (SA*)&cli_backend, &backend_len);
        if (backendcl < 0) {
            printf("server acccept failed...\n");
            exit(0);
        }
        else
            printf("server acccept the client...\n");
    }


    //starting threads
    printf("Starting threads\n");

    if (enable_vid_dat)
    {
        pthread_create(&thread_id_vid, NULL, videoRecThread, NULL);
    }
    if (enable_vid_met || enable_backend1)
    {
        pthread_create(&thread_id_met_vid, NULL, videoMetaThreadCli, NULL);
        pthread_create(&thread_id_met_vid2, NULL, videoMetaThreadBackend, NULL);
    }
    if (enable_backend2 || enable_fifo)
    {
        pthread_create(&thread_id_OB, NULL, OBThreadBackend, NULL);
    }



    while(run)
    {

    }
    return 0;

}
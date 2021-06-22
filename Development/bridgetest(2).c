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

// memory
#define soc_cv_av
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "hps_soc_system.h"
#include "hps_linux.h"

#define FIFO_FRAMING_TX_FULL		  ((*(fifo_framing_txstatus_ptr+1))& 1 )
#define FIFO_FRAMING_TX_EMPTY	  ((*(fifo_framing_txstatus_ptr+1))& 2 )
#define FIFO_FRAMING_RX_FULL		  ((*(fifo_framing_rxstatus_ptr+1))& 1 )
#define FIFO_FRAMING_RX_EMPTY	  ((*(fifo_framing_rxstatus_ptr+1))& 2 )

#define FIFO_CONTROL_TX_FULL		  ((*(fifo_control_txstatus_ptr+1))& 1 )
#define FIFO_CONTROL_TX_EMPTY	  ((*(fifo_control_txstatus_ptr+1))& 2 )
#define FIFO_CONTROL_RX_FULL		  ((*(fifo_control_rxstatus_ptr+1))& 1 )
#define FIFO_CONTROL_RX_EMPTY	  ((*(fifo_control_rxstatus_ptr+1))& 2 )

#define alt_write_word(dest, src)       (*ALT_CAST(volatile uint32_t *, (dest)) = (src))

// Declarations
    bool run = false;
    char exitstr[] = "exit";
    char usercomm[4] = {0};
    int env = 20;
    bool fullrun, enable_vid_dat, enable_vid_loop, enable_vid_met, enable_backend1, enable_backend2, enable_fifo;
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
    char smallBuff[2] = {0};
    char bufferRec[1024] = {0};
	
	// Video data sock
    int sock_video_loop, valreadloop, recv_len_loop ;
    struct sockaddr_in addressVidLoop;
    int addrlenLoop = sizeof(addressVidLoop);

    // Video meta sock
    bool runVideoMeta1 = true;
    int sock_video_meta, vidcl, video_meta_len, bit_rate;
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
    char bufferOBBack[1024] = {0};

volatile unsigned char * fifo_framing_receive_ptr = NULL ;
volatile unsigned int  * fifo_framing_rxstatus_ptr = NULL ;
volatile unsigned char * fifo_framing_transmit_ptr = NULL ;
volatile unsigned int  * fifo_framing_txstatus_ptr = NULL ;
volatile unsigned char* fifo_control_receive_ptr = NULL;
volatile unsigned int* fifo_control_rxstatus_ptr = NULL;
volatile unsigned char* fifo_control_transmit_ptr = NULL;
volatile unsigned int* fifo_control_txstatus_ptr = NULL;
volatile unsigned int  * fpga_leds = NULL;
volatile unsigned int  * fpga_switches = NULL;

void open_physical_memory_device() {
    // We need to access the system's physical memory so we can map it to user
    // space. We will use the /dev/mem file to do this. /dev/mem is a character
    // device file that is an image of the main memory of the computer. Byte
    // addresses in /dev/mem are interpreted as physical memory addresses.
    // Remember that you need to execute this program as ROOT in order to have
    // access to /dev/mem.

    fd_dev_mem = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd_dev_mem  == -1) {
        printf("ERROR: could not open \"/dev/mem\".\n");
        printf("    errno = %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void close_physical_memory_device() {
    close(fd_dev_mem);
}

void mmap_fpga_peripherals() {
    h2f_lw_axi_master = mmap(NULL, h2f_lw_axi_master_span, PROT_READ | PROT_WRITE, MAP_SHARED, fd_dev_mem, h2f_lw_axi_master_ofst);
    if (h2f_lw_axi_master == MAP_FAILED) {
        printf("Error: h2f_lw_axi_master mmap() failed.\n");
        printf("    errno = %s\n", strerror(errno));
        close(fd_dev_mem);
        exit(EXIT_FAILURE);
    }
	h2f_axi_master = mmap(NULL, h2f_axi_master_span, (PROT_READ | PROT_WRITE), MAP_SHARED, fd_dev_mem, h2f_axi_master_ofst);
    if (h2f_axi_master == MAP_FAILED) {
        printf("Error: h2f_lw_axi_master mmap() failed.\n");
        printf("    errno = %s\n", strerror(errno));
        close(fd_dev_mem);
        exit(EXIT_FAILURE);
    }
	fifo_framing_transmit_ptr = (unsigned char *) (h2f_axi_master + FIFO_TX_VIDEO_IN_BASE);
	fifo_framing_txstatus_ptr = (unsigned int *)(h2f_lw_axi_master +  FIFO_TX_VIDEO_IN_CSR_BASE);
	fifo_framing_receive_ptr = (unsigned char *) (h2f_axi_master + FIFO_RX_VIDEO_OUT_BASE);
	fifo_framing_rxstatus_ptr = (unsigned int *)(h2f_lw_axi_master +  FIFO_RX_VIDEO_OUT_CSR_BASE);

	fifo_control_transmit_ptr = (unsigned char*)(h2f_lw_axi_master + FIFO_CONTROL_TX_IN_BASE);
	fifo_control_txstatus_ptr = (unsigned int*)(h2f_lw_axi_master + FIFO_CONTROL_TX_IN_CSR_BASE);
	fifo_control_receive_ptr = (unsigned char*)(h2f_lw_axi_master + FIFO_CONTROL_RX_OUT_BASE);
	fifo_control_rxstatus_ptr = (unsigned int*)(h2f_lw_axi_master + FIFO_CONTROL_RX_OUT_CSR_BASE);

	fpga_leds =   (unsigned int *) (h2f_lw_axi_master +  HPS_FPGA_LEDS_BASE);
	fpga_switches = h2f_lw_axi_master + HPS_FPGA_SWITCHES_BASE;
}

void munmap_fpga_peripherals() {
    if (munmap(h2f_lw_axi_master, h2f_lw_axi_master_span) != 0) {
        printf("Error: h2f_lw_axi_master munmap() failed\n");
        printf("    errno = %s\n", strerror(errno));
        close(fd_dev_mem);
        exit(EXIT_FAILURE);
    }
	if (munmap(h2f_axi_master, h2f_axi_master_span) != 0) {
        printf("Error: h2f_lw_axi_master munmap() failed\n");
        printf("    errno = %s\n", strerror(errno));
        close(fd_dev_mem);
        exit(EXIT_FAILURE);
    }
    h2f_lw_axi_master = NULL;
	h2f_axi_master    = NULL;
    fpga_leds         = NULL;
	fpga_switches	  = NULL;
	fifo_framing_transmit_ptr = NULL ;
	fifo_framing_txstatus_ptr = NULL ;
	fifo_framing_receive_ptr = NULL ;
	fifo_framing_rxstatus_ptr = NULL ;
	fifo_control_transmit_ptr = NULL ;
	fifo_control_txstatus_ptr = NULL ;
	fifo_control_receive_ptr = NULL ;
	fifo_control_rxstatus_ptr = NULL ;

}
// VIDEO to FIFO (video data)
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
	    bit_rate += recv_len * 8;
	    printf("Data: ");
		if (sendto(sock_video_loop, buffer, sizeof(buffer), 0, (struct  sockaddr*) &addressVidLoop, sizeof(addressVidLoop)) == -1){}
	    for (int i = 0; i < recv_len; i++)
	    {
	    	smallBuff[i % 2] = buffer[i];
	    	if (i % 2 == 1)
        	{
        	    if (enable_fifo && env == 0)
        	    {
            	        if (!FIFO_FRAMING_TX_FULL) {* fifo_framing_transmit_ptr=(unsigned char)strtol(smallBuff, NULL, 16);};
            	}
                    if (enable_vid_loop)
                    {
		    printf("%0x%0x" , smallBuff[0],smallBuff[1]);
                    //now reply the client with the same data
                }
        	}
	    }
        printf("\n");
   	    memset(buffer, 0, 1024);
    }
}

// Bitrate messenger
void *videoMetaThreadCli(void *vargp)
{
    printf("VideoMeta1Thread started with id: %lu\n", pthread_self());

    while (runVideoMeta1)
    {
        sleep(1);
        printf("Bit rate is:");
        printf("%i\n", bit_rate);
        printf("Waiting for data...");
        fflush(stdout);
        //try to receive some data, this is a blocking call
        sprintf(bufferMeta1, "%d", bit_rate);
        printf("%s\n",bufferMeta1 );
        // redirect data
        if (sendto(vidcl2, bufferMeta1, video_meta_len, 0, (struct sockaddr*) &cli_vid_met2, slenMet2) == -1)
	    {
	    	printf("data redirection to backend from video failed\n");
	    }

   	memset(bufferMeta1, 0, 1024);
   	bit_rate = 0;

    }
}

// Control data from backend to VIDEO
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

// Control data fra backend til FIFO
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
	    printf("Fifo pointers implemented. Data recieved is %s\n", bufferOB);

        if (enable_fifo && env == 0)
        {
            // Drop if full
            if (!FIFO_CONTROL_TX_FULL) {* fifo_control_transmit_ptr=(unsigned char)atoi(bufferOB);};
        }

   	    memset( bufferOB, 0, 1024);
   	}
}

// Control data fra FIFO til backend
void *BackendThreadOB(void *vargp)
{
    printf("On board to backend thread started with id: %lu\n", pthread_self());
    char NBack;
    unsigned int NBackun;
    memset(bufferOBBack, 0, 1024);
    printf("Initiating On board to backend loop\n");
    while (runOB)
    {
        if (!FIFO_CONTROL_RX_EMPTY)
        {
            printf("Data found in On board to backend fifo");
			NBack = * fifo_control_receive_ptr;
			NBackun = (unsigned)NBack;
			printf("This is sending unsigned: %u\n",(unsigned)NBack);
			printf("  ");
            memcpy(bufferOBBack, (char*)&NBackun, sizeof(NBackun));
            printf("This is sending buffer: %c\n",bufferOBBack[0]);
            printf("This is sending buffer attempt 2: %c\n",bufferOBBack[1]);

			if (sendto(backendcl, bufferOBBack, strlen(bufferOBBack), 0, (struct sockaddr*) &si_other, slen) == -1)
	        {
	    	    //die("sendto()");
	        }

	        memset(bufferOBBack, 0, 1024);
		}
   	}
}

// VIDEO data from FIFO to video
void *OBThreadFIFO(void *vargp)
{
    char N;
    unsigned int Nun;
    memset(bufferRec, 0, 1024);
    while (runVideo1)
    {
        if (!FIFO_FRAMING_RX_EMPTY)
        {
			N = * fifo_framing_receive_ptr;
			Nun = (unsigned)N;
			printf("This is sending unsigned: %u\n",(unsigned)N);
			printf("  ");
            memcpy(bufferRec, (char*)&Nun, sizeof(Nun));
            printf("This is sending buffer: %c\n",bufferRec[0]);
            printf("This is sending buffer attempt 2: %c\n",bufferRec[1]);

			if (sendto(sock_video_data, bufferRec, strlen(bufferRec), 0, (struct sockaddr*) &si_other, slen) == -1)
	        {
	    	    //die("sendto()");
	        }

	        memset(bufferRec, 0, 1024);
		}
    }
}

int main(int argc, char const *argv[])
{
    printf("Initiating setup.\n");
	printf("DE1-SoC linux demo\n");

    while (env != 0 && env != 1)
    {
        printf("Please choose running enviroment(0=arm processor, 1=testing enviroment)\n");
        scanf(" %d", &env);
    }
    if (env == 0)
    {
        open_physical_memory_device();
        mmap_fpga_peripherals();
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
        enable_vid_loop = false;
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
            printf("enable video data software looparound? (y/n)\n");
            scanf(" %c", &boolCheck);
            if (boolCheck == 'y'){
                enable_vid_loop = true;
                cont = true;
            }else if (boolCheck == 'n'){
                enable_vid_loop = false;
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
    pthread_t thread_id_vid, thread_id_vid_OB, thread_id_met_vid, thread_id_met_vid2, thread_id_OB, thread_id_OB_Back;

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

	// Creating sock descs
    printf("Creating socket descriptors\n");
    printf("(1/4) - Video Data loop socket initing\n");
    // Video data recieve
    if ((sock_video_loop = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        perror("video data socket failed");
        exit(EXIT_FAILURE);
    }

    // Socket-port 8889

    addressVidLoop.sin_family = AF_INET;
    addressVidLoop.sin_addr.s_addr = inet_addr("10.16.173.0"); // THIS IS ADDRESS
    addressVidLoop.sin_port = htons( 8889 );

    // Video meta
    printf("(2/4) - Video Meta socket video side initiating\n");
    if ((sock_video_meta = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("video Meta socket failed");
        exit(EXIT_FAILURE);
    }
    // assign IP, PORT 8889
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
    // assign IP, PORT 8890
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
    // assign IP, PORT 8891
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
            printf("server accept failed...\n");
            exit(0);
        }
        else
            printf("server accepted the client...\n");
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
            printf("server accept failed...\n");
            exit(0);
        }
        else
            printf("server accepted the client...\n");
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
            printf("server accept failed...\n");
            exit(0);
        }
        else
            printf("server accepted the client...\n");
    }

    //starting threads
    printf("Starting threads\n");

    if (enable_vid_dat)
    {
        pthread_create(&thread_id_vid, NULL, videoRecThread, NULL);
        if ( env == 0 )
        {
            pthread_create(&thread_id_vid_OB, NULL, OBThreadFIFO, NULL);

        }
    }
    if (enable_vid_met || enable_backend1)
    {
        pthread_create(&thread_id_met_vid, NULL, videoMetaThreadCli, NULL);
    }
    if (enable_backend2 || enable_fifo)
    {
        pthread_create(&thread_id_OB, NULL, OBThreadBackend, NULL);
        if (env == 0)
            pthread_create(&thread_id_OB_Back, NULL, BackendThreadOB, NULL);
    }

    while(run)
    {
        printf("enter 'exit' to exit\n");
        scanf(" %s", usercomm);
        printf("scanf is %s", usercomm);
        if (strcmp(exitstr, usercomm) == 0){
            printf("Exitting");
            run = false;
        }
	else{
	    printf("sending %s", usercomm);
	    if (sendto(backendcl, usercomm, backend_len, 0, (struct sockaddr*) &cli_backend, slenOB) == -1){}
	}
    }
    if (env == 0)
    {
        munmap_fpga_peripherals();
        close_physical_memory_device();
    }

    return 0;

}

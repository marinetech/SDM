#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <err.h> 
#include <arpa/inet.h> /* inet_addr() */
#include <math.h>
#include <limits.h> /* SHRT_MAX */
#include <endian.h>

#define BUFSIZE  1024
#define NSAMPLES 1024
#define SAMPLERATE 250000

/* SDM PHY commands in binary form                                                                                             */
/*			     |     SDM prefix                               | cmd |     param       |         length        |  */
unsigned char cmd_config[] = {0x80 ,0x00 ,0x7f ,0xff ,0x00 ,0x00 ,0x00 ,0x00 ,0x04 ,0x5e ,0x01 ,0x03 ,0x00 ,0x00 ,0x00 ,0x00};
unsigned char cmd_rx[]   =   {0x80 ,0x00 ,0x7f ,0xff ,0x00 ,0x00 ,0x00 ,0x00 ,0x02 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00};
unsigned char cmd_ref[]   =  {0x80 ,0x00 ,0x7f ,0xff ,0x00 ,0x00 ,0x00 ,0x00 ,0x03 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00};
unsigned char cmd_stop[]   = {0x80 ,0x00 ,0x7f ,0xff ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00};

unsigned char buf[BUFSIZE] = {0};

/* function executing data/command exchange with the SDM PHY via the socket */
void sdm_cmd(int sockfd, unsigned char *cmd, ssize_t len)
{
    int i, n;

    printf("snd: ");
    for (i = 0; i < len; i++) {
        printf("%02x ", cmd[i]);
        if (i && (i + 1) % 16 == 0)
            printf("  ");
        if (i && (i + 1) % 32 == 0)
            printf("\n     ");
    }
    printf("\n");

    n = write(sockfd, cmd, len);
    if (n < 0) 
      err(1, "write(): ");

    if (n != len)
      errx(1, "n != len: %d != %ld\n", n , len);

    n = read(sockfd, buf, BUFSIZE);
    if (n < 0) 
      err(1, "read(): ");

    printf("rcv: ");
    for (i = 0; i < n; i++) {
	    printf("%02x ", buf[i]);
        if (i && (i + 1) % 16 == 0) {
            printf("\n");
            if (i != n - 1)
                printf("     ");
        }
    }
    printf("\n");
}

/* function generating sweep signal (as signal reference) */
int chirp_sig(int nsamples, float amplitude, float fstart, float fend, float phi_start, signed short int *vchirp)
{
    int i;
     
    fstart = fstart/SAMPLERATE;
    fend = fend/SAMPLERATE;
    if (nsamples <= 0 || amplitude < 0 || fstart < 0 || fend < 0 || vchirp == NULL)
	    return -1;

    if (fstart < fend)
        for (i = 0; i < nsamples; i++)
	     vchirp[i] = amplitude * cos(2.*M_PI*fstart*i + 2.*M_PI*(fend-fstart)/2./(nsamples-1)*i*i + phi_start) * SHRT_MAX;
    else
        for (i = 0; i < nsamples; i++)
             vchirp[i] = amplitude * cos(2.*M_PI*fstart*i - 2.*M_PI*(fstart-fend)/2./(nsamples-1)*i*i + phi_start) * SHRT_MAX;

    return nsamples;
}

/* function emptying the buffer of the SDM PHY*/
void flush_data(int sockfd)
{
    fd_set rfds;
    struct timeval tv;
    int ret;

    for (;;) {
        FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);

        tv.tv_sec  = 1;
        tv.tv_usec = 0;
        ret = select(sockfd + 1, &rfds, NULL, NULL, &tv);
        
        if (ret == -1)	{
		warn("Select() ");
    		return;
	}
        if (!ret) {
            warn("Select() ");
            break;
	}
        ret = read(sockfd, buf, BUFSIZE);
        printf ("flush %d bytes\n", ret);
    }
}

/* function reading received signal from the SDM PHY */
int read_data(int sockfd, unsigned char *readsig, int size)
{
    fd_set rfds;
    struct timeval tv;
    int ret, n = 0;
    int i = 0;

    for (;;) {
        FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);

        tv.tv_sec  = 10;
        tv.tv_usec = 0;
        ret = select(sockfd + 1, &rfds, NULL, NULL, &tv);
        if (ret == -1)	{
		warn("Select()");
		return -1;
	}
        if (!ret) {
	    return n;
	}

        ret = read(sockfd, readsig + n, size - n);
        if (ret == -1)	{
		warn("Read()");
    		return -1;
	}

        printf("read() %d\n", ret);
    	for (i = 0; i < ret; i++) {
    		printf("%02x ", readsig[n+i]);
		if (i && (i + 1) % 16 == 0) {
	    		printf("\n");
	    		if (i != n - 1)
				printf("     ");
		}
    	}
	n += ret;
	if (n >= size)
		return n;
    }
    return 0;
}

/* function creating the socket, connecting the socket, configuring the PHY and reading from the SDM PHY buffer (reading received signal) */
int main(int argc, char *argv[])
{
    int sockfd, portno, i;
    struct sockaddr_in serveraddr;
    char *ip;
    float amplitude = 1;  // reference signal amplitude
    float fstart = 20000; // start frequence of the reference signal  
    float fend = 30000;   // end frequency of the sweep signal 
    float phi_start = 0;  // beginning phase of the sweep signal 
    int retn, retnsread;
    int mp = 15;          // number of reference signal lengths - length of the buffer for read rx signal from the socket 
    signed short int vchirp[NSAMPLES];
    signed short int readsig[mp*NSAMPLES];
    unsigned char ref_buf[sizeof(cmd_ref) + sizeof(vchirp)];
    unsigned char cmd_rx_buf[sizeof(cmd_rx)];
    int nsread = mp*NSAMPLES;
    FILE *fp;

/* check command line arguments */
    if (argc != 3)
       errx(0 ,"usage: %s <IP> <port>\n", argv[0]);
    ip = argv[1];
    portno = atoi(argv[2]);

/* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        err(1, "socket(): ");
    memset((char *)&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_addr.s_addr = inet_addr(ip);
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(portno);

/* connect: create a connection with the SDM PHY */
    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) 
      err(1, "connect(): ");

/* empty the SDM PHY buffer*/
    flush_data(sockfd);

/* send the command line "cmd_stop" to initialize the SDM PHY */
    printf("SDM command 'stop' send and reply:\n");
    sdm_cmd(sockfd, cmd_stop, sizeof(cmd_stop));
    printf("\n");

/* send the command line "cmd_config" to configure the SDM PHY */
    printf("SDM command 'config' send and reply:\n");
    sdm_cmd(sockfd, cmd_config, sizeof(cmd_config));
    printf("\n");

/* generate the reference signal (needed for SDM PHY detector) */
    printf("Generated reference signal :\n");
    retn = chirp_sig(NSAMPLES, amplitude, fstart, fend, phi_start, vchirp);
    if (retn <= 0)
	    errx(1, "chirp_sig() return %d\n", retn);
    retn = htobe32(retn);
    printf ("%08x\n", retn);
    printf ("\n");
    
    sleep(3);

/* fill out the field 'len' with the length of the reference signal (always 1024 samples; in user applications: use zero padding if necessary) */
/* fill out the field 'data' with the reference signal, and send the command line "cmd_ref" to the SDM PHY */
    printf("SDM command 'ref':\n");
    memcpy(ref_buf, cmd_ref, sizeof(cmd_ref));
    memcpy(ref_buf + sizeof(cmd_ref) - 5, &retn, sizeof(retn));
    memcpy(ref_buf + sizeof(cmd_ref), &vchirp, sizeof(vchirp));
    sdm_cmd(sockfd, ref_buf, sizeof(ref_buf));
    printf("\n");

/* send the command line "cmd_rx" to the SDM PHY for initiating rx mode */
    printf("SDM command 'rx':\n");    
    memcpy(cmd_rx_buf, cmd_rx, sizeof(cmd_rx));
    memcpy(cmd_rx_buf + sizeof(cmd_rx_buf) - 7, &nsread, sizeof(nsread));
    sdm_cmd(sockfd, cmd_rx_buf, sizeof(cmd_rx_buf));
    
/* reading from the socket of the SDm PHY: after detecting a matching signal the SDM PHY returns via the socket the received */
/* signal samples (the samples comming after the detected matching signal) */
    retnsread = read_data(sockfd, (char *)readsig, sizeof(readsig));
    printf ("read %d bytes\n", retnsread);

    fp = fopen("sig_rx.txt","w+");
    for (i = 0; i < mp*NSAMPLES; i++)
	    fprintf(fp,"%d\n",readsig[i]);
    fclose(fp);   

/* send the command line "cmd_stop" to the SDM PHY */
    printf("SDM command 'stop' send and reply:\n");
    sdm_cmd(sockfd, cmd_stop, sizeof(cmd_stop));
    printf("\n");
 
    close(sockfd);

    return 0;
}

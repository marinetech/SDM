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

#define BUFSIZE 1024
#define NSAMPLES 125000
#define SAMPLERATE 250000

/* SDM PHY commands in binary form                                                        				       */
/*			     |                   SDM PHY prefix              | cmd |     param       |         length        | */
unsigned char cmd_config[] = {0x80 ,0x00 ,0x7f ,0xff ,0x00 ,0x00 ,0x00 ,0x00 ,0x04 ,0x2C ,0x01 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00};
unsigned char cmd_tx[]   =   {0x80 ,0x00 ,0x7f ,0xff ,0x00 ,0x00 ,0x00 ,0x00 ,0x01 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00};
unsigned char cmd_stop[]   = {0x80 ,0x00 ,0x7f ,0xff ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00};

unsigned char buf[BUFSIZE] = {0};

char **strsplit(const char* str, const char* delim, size_t* numtokens) {
    // copy the original string so that we don't overwrite parts of it
    // (don't do this if you don't need to keep the old line,
    // as this is less efficient)
    char *s = strdup(str);
    // these three variables are part of a very common idiom to
    // implement a dynamically-growing array
    size_t tokens_alloc = 1;
    size_t tokens_used = 0;
    char **tokens = calloc(tokens_alloc, sizeof(char*));
    char *token, *strtok_ctx;
    for (token = strtok_r(s, delim, &strtok_ctx);
            token != NULL;
            token = strtok_r(NULL, delim, &strtok_ctx)) {
        // check if we need to allocate more space for tokens
        if (tokens_used == tokens_alloc) {
            tokens_alloc *= 2;
            tokens = realloc(tokens, tokens_alloc * sizeof(char*));
        }
        tokens[tokens_used++] = strdup(token);
    }
    // cleanup
    if (tokens_used == 0) {
        free(tokens);
        tokens = NULL;
    } else {
        tokens = realloc(tokens, tokens_used * sizeof(char*));
    }
    *numtokens = tokens_used;
    free(s);
    return tokens;
}

/* function executing data/command exchange with the SDM PHY via the socket*/
void sdm_cmd(int sockfd, unsigned char *cmd, ssize_t len)
{
    // int i, n;
    int n;
    // printf("snd: ");
    // for (i = 0; i < len; i++) {
    //     printf("%02x ", cmd[i]);
    //     if (i && (i + 1) % 16 == 0)
    //         printf("  ");
    //     if (i && (i + 1) % 32 == 0)
    //         printf("\n     ");
    // }
    // printf("\n");

    n = write(sockfd, cmd, len);
    if (n < 0)
      err(1, "write(): ");

    if (n != len)
      errx(1, "n != len: %d != %ld\n", n , len);

    n = read(sockfd, buf, BUFSIZE);
    if (n < 0)
      err(1, "read(): ");

    // printf("rcv: ");
    // for (i = 0; i < n; i++) {
	  //   printf("%02x ", buf[i]);
    //     if (i && (i + 1) % 16 == 0) {
    //         printf("\n");
    //         if (i != n - 1)
    //             printf("     ");
    //     }
    // }
    // printf("\n");
}

/* function generating sweep signal */
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

        if (ret == -1)
            return;
        if (!ret)
            break;

        ret = read(sockfd, buf, BUFSIZE);
        printf ("flush %d bytes\n", ret);
    }
}

/* function creating the socket, connecting the socket, configuring the PHY and sending signal to the PHY */
int main(int argc, char *argv[])
{
    int sockfd, portno, i;
    struct sockaddr_in serveraddr;
    char *ip;
	    float amplitude = 1;  // signal amplitude
	    float fstart = 7000; // start frequence of the sweep signal
	    float fend = 17000;   // end frequency of the sweep signal
	    float phi_start = 0;  // beginning phase of the sweep signal
	    int retn, mp = 10;    // number of signals to be generated and sent to the SDM PHY
	    signed short int vchirp[NSAMPLES];
      unsigned char *v;
      char line[1024];
      char **tokens;
      size_t numtokens;

	    unsigned char tx_buf[sizeof(cmd_tx) + mp*sizeof(vchirp)];
	    FILE *fp;

/* check command line arguments */
	    if (argc != 3)
	       errx(0 ,"usage: %s <IP> <port>\n", argv[0]);
	    ip = argv[1];
	    portno = atoi(argv[2]);

/* get experiment data from file */
    fp = fopen("exp.txt","r");
    if (fp) {
      fgets(line,1024,fp);
      tokens = strsplit(line, ",", &numtokens);
      if (numtokens == 4){
        amplitude = atof(tokens[0]);
	      fstart = atof(tokens[1]);
	      fend = atof(tokens[2]);
        retn = atof(tokens[3]);
        mp = atof(tokens[3]);
      } else {
        printf ("fail to load exp file\n");
      }
      // printf("%s\n",line);
      fclose(fp);
    }

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

/* empty the SDM PHY buffer */
	    flush_data(sockfd);

/* send the command line "cmd_stop" to initialize the SDM PHY */
	    printf("SDM command 'stop' send and reply:\n");
	    sdm_cmd(sockfd, cmd_stop, sizeof(cmd_stop));
	    printf("\n");

/* send the command line "cmd_config" to configure the SDM PHY */
	    printf("SDM command 'config' send and reply:\n");
	    sdm_cmd(sockfd, cmd_config, sizeof(cmd_config));
	    printf("\n");

/* gererate the signal form (sweep) */
	    printf("Generated tx signal form:\n");
	    retn = chirp_sig(NSAMPLES, amplitude, fstart, fend, phi_start, vchirp);
	    if (retn <= 0)
		    errx(1, "sig() return %d\n", retn);
	    retn = htobe32(retn*mp);
	    printf ("%08x\n", retn);

	    sleep(3);

/* fill out the 'len' field of the commnd with the actual length of the generated signal,
 * fill out the 'data' field with the generated signal, and send the command line "cmd_tx" to the SDM PHY */
	    printf("SDM command 'tx':\n");
	    memcpy(tx_buf, cmd_tx, sizeof(cmd_tx));
	    memcpy(tx_buf + sizeof(cmd_tx) - 5, &retn, sizeof(retn));
	    for (i = 0; i < mp; i++)
		    memcpy(tx_buf + sizeof(cmd_tx) + i*sizeof(vchirp), vchirp, sizeof(vchirp));
	    sdm_cmd(sockfd, tx_buf, sizeof(tx_buf));

	    fp = fopen("sig_tx.txt","w+");
	    v = &tx_buf[16];
	    for (i = 0; i < mp*NSAMPLES; i++)
		    fprintf(fp,"%d\n",v[i]);
	    fclose(fp);

/* send the command line "cmd_stop" to the SDM PHY */
	    printf("SDM command 'stop' send and reply:\n");
	    sdm_cmd(sockfd, cmd_stop, sizeof(cmd_stop));
	    printf("\n");

	    close(sockfd);

	    return 0;
	}

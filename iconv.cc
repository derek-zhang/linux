#include <iostream>
#include <fstream>
#include <iconv.h>
#include <sys/time.h>
#include <stdint.h>

using namespace std;
typedef char fstring[256];

int main(int argc, char *argv[])
{
    char src[] = "abccdefg";
    char dst[100];
    size_t srclen = 6;
    size_t dstlen = 12;


    fstring user;
    uint16_t blob_len = 7;
    size_t tmp_len = blob_len;
    size_t buf_len = sizeof(user);
    //fprintf(stderr,"in: %s\n",src);

    char * pIn = src;
    char *blob_pos = pIn+1;
    char * pOut = ( char*)dst;

    iconv_t conv = iconv_open("UTF8","UTF16");
    timeval starttime, endtime;
    double timeuse = 0.0;
    ::gettimeofday(&starttime, 0);

    iconv(conv, NULL, NULL, NULL, NULL);
    //iconv(conv, &pIn, &srclen, &pOut, &dstlen);
    iconv(conv, &blob_pos, &tmp_len, (char **)&user, &buf_len);


    //iconv_close(conv);
    ::gettimeofday(&endtime, 0);
    timeuse = 1000000*(endtime.tv_sec - starttime.tv_sec) + endtime.tv_usec - starttime.tv_usec;
    printf("incidnet sync(insert) new time=%f\n", timeuse/1000);


    ::gettimeofday(&starttime, 0);

    char src2[] = "hijklmop";
    char *in = src2;
    //conv = iconv_open("UTF8","UTF16");
    iconv(conv, NULL, NULL, NULL, NULL);
    iconv(conv, &in, &srclen, &pOut, &dstlen);
    //iconv_close(conv);

    ::gettimeofday(&endtime, 0);
    timeuse = 1000000*(endtime.tv_sec - starttime.tv_sec) + endtime.tv_usec - starttime.tv_usec;
    printf("incidnet sync(insert) new time=%f\n", timeuse/1000);

    //iconv_close(conv);
    //fprintf(stderr,"out: %s\n",dst);
}

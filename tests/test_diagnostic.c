#include "diagnostic.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    uint8_t config[] = {9,2,32,0,1,1,0,0x80,50, 9,4,0,0,2,7,1,2,0,
                       7,5,0x03,2,64,0,0, 7,5,0x84,2,64,0,0};
    assert(configuration_valid(config,sizeof(config)));
    for (size_t n=0;n<sizeof(config);++n) assert(!configuration_valid(config,n));
    config[18]=0; assert(!configuration_valid(config,sizeof(config))); config[18]=7;
    config[9]=1; assert(!configuration_valid(config,sizeof(config))); config[9]=9;
    config[2]=33; assert(!configuration_valid(config,sizeof(config))); config[2]=32;
    config[18]=255; assert(!configuration_valid(config,sizeof(config))); config[18]=7;
    uint8_t maximum[DESC_CAP] = {9,2,0,2,0,1,0,0x80,0};
    // Opaque class-specific descriptors; validate bounds without assuming contents.
    maximum[9]=251; maximum[10]=0x24; maximum[260]=252; maximum[261]=0x24;
    assert(configuration_valid(maximum,sizeof(maximum)));
    assert(read_le16((uint8_t[]){0x23,0x11}) == 0x1123);
    const char *requests[]={"GET / HTTP/1.1\r\nHost: pico\r\n\r\n", "GET /api/status HTTP/1.0\r\n\r\n",
        "POST /api/jobs HTTP/1.1\r\n\r\n", "GET /api/jobs HTTP/1.1\r\n\r\n", "GET /api/status?x=1 HTTP/1.1\r\n\r\n",
        "GET / HTTP/9.9\r\n\r\n", "GET / HTTP/1.1\r\n", "nonsense\r\n\r\n"};
    const int expected[]={200,201,405,404,404,400,400,400};
    for (unsigned i=0;i<sizeof(expected)/sizeof(expected[0]);++i) assert(http_route(requests[i],strlen(requests[i]))==expected[i]);
    char nul[]="GET / HTTP/1.1\r\nX: a\r\n\r\n"; nul[20]=0;
    assert(http_route(nul,sizeof(nul)-1)==400);
    char too_long[HTTP_REQUEST_CAP+1]; memset(too_long,'x',sizeof(too_long));
    assert(http_route(too_long,sizeof(too_long))==400);
    char out[HTTP_RESPONSE_CAP];
    diagnostic.usb.configuration_len=DESC_CAP;
    memcpy(diagnostic.usb.configuration,maximum,DESC_CAP);
    assert(diagnostic_json(out,sizeof(out),&diagnostic));
    assert(strstr(out,"\"motion_enabled\":false"));
    assert(strstr(out,"\"plotting_complete\":null"));
    assert(!diagnostic_json(out,20,&diagnostic));
    diagnostic.usb.configuration_len=DESC_CAP+1;
    assert(!diagnostic_json(out,sizeof(out),&diagnostic));
    diagnostic.usb.configuration_len=DESC_CAP;
    size_t n=http_response(out,sizeof(out),201,&diagnostic);
    assert(n && n<sizeof(out));
    assert(!strncmp(out,"HTTP/1.1 200 OK",15));
    const char *body=strstr(out,"\r\n\r\n"); assert(body); body+=4;
    unsigned length=0; assert(sscanf(strstr(out,"Content-Length:"),"Content-Length: %u",&length)==1);
    assert(length==strlen(body));
    puts(body); // Python test runner validates JSON independently.
    assert(http_response(out,sizeof(out),405,&diagnostic));
    assert(strstr(out,"Allow: GET\r\n"));
    assert(!http_response(out,10,200,&diagnostic));
    return 0;
}

#ifndef __HTTP_PARSE_H_
#define __HTTP_PARSE_H_

//
int http_parse_request_line(ftp_request_t* request);

int http_parse_request_body(http_request_t* request);

#endif
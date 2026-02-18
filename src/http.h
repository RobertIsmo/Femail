#ifndef HTTP_H
#define HTTP_H

char nocontentresponse[] =					\
	"HTTP/1.1 204 No Content\r\n"			\
	"Server: Femail\r\n"					\
	"Connection: close\r\n"					\
	"Cache-Control: no-store\r\n"			\
	"\r\n";

#define httpmsg_no_content()	dprintf(conn->clientsocket,	\
										nocontentresponse)

#define httpmsg_redirect_https()	dprintf(conn->clientsocket,			\
											"HTTP/1.1 301 Moved Permanently\r\n" \
											"Server: Femail\r\n"		\
											"Location: https://%s/\r\n"	\
											"Connection: close\r\n"		\
											"\r\n",						\
											get_mail_domain())
#define httpsmsg_no_content()	SSL_write(conn->sslconn,				\
										  nocontentresponse,			\
										  sizeof(nocontentresponse) - 1)
#endif // HTTP_H

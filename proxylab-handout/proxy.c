/*
 * CSAPP: lab7 proxylab
 * Name = Hailei Yu
 * Andrew id = haileiy
 */

/*
 * Implemented a concurrent proxy with caching
 * The implementations of a multithread proxy are fairly simple
 * when the proxy gets a request from client, it firstly parses the client
 * then try to connect to the server using the hostname and port number
 * if successfully establishes connection, then configure a new header using
 * information from client's header
 * Forward the header, get the responses, forward the responses back to client
 * and store the response to cache (if the size is not too big)
 * next time the client requests the same object, just fetch it from cache
 * and forward to client, there is no need to connect to the server
 *
 * Design issues of cache
 * Used a linked stack to implement a LRU cache
 * if an node is inserted, place it after the additional header
 * it an node is visited, move it after the additional header
 * if the cache is bigger than MAX_CACHE_SIZE, simple evict nodes at the end
 */

#include <stdio.h>
#include "csapp.h"
#include "cache.h"

/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
static const char *host_hdr = "Host: %s\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";

//========================function declarations
void get_key_from_client_header(char *header_client, char *key);
int parse_uri(char *uri, char *host, char *suffix);
void config_header_server (rio_t *client_riop, char *header_buf,
                           char *host, char *suffix);
void clienterror(int fd, char *cause, char *errnum, char *smsg, char *lmsg);
void *doit_thread(void *vargp);
void doit(int connfd_client);
void serve_cached (CM *mycache, char *uri, int connfd_client);
//========================functions and variables
/* CM stands for cache manager
 * which is an additional data structure for managing the cache
 */
CM *mycache;
/* get_key_from_client_header
 * parses client's header, and get the key
 */
void get_key_from_client_header(char *header_from_client, char *key) {
    char key_buf[MAXLINE];
    strcpy(key_buf, header_from_client);
    char *ptr = key_buf;
    while (*ptr != ':' && *ptr != '\0') {
        ptr++;
    }
    if (*ptr == ':') {
        *ptr = '\0';
        strcpy(key, key_buf);
    }
    return;
}
/* parse_uri: parses the uri, stores the host part and suffix part in strings
 * and returns the port number
 * If the port number is not specified, then simply return default port(80)
 */
int parse_uri(char *uri, char *host, char *suffix) {
    char uribuf[MAXLINE];
    strcpy(uribuf, uri);
    char *uribuf_ptr = uribuf;

    char port_buf[MAXLINE];
    char *port_start;
    int port;

    if (!strncmp(uribuf, "http://", 7)) {
        uribuf_ptr += 7;
    }
    //get hostname
    while (*uribuf_ptr != ':' && *uribuf_ptr != '/') {
        *host++ = *uribuf_ptr++;
    }
    *host = '\0';
    if (*uribuf_ptr == '/') {
        while (*uribuf_ptr != '\0') {
            *suffix++ = *uribuf_ptr++;
        }
        port = 80;
    }
    //there is port number
    else {
        uribuf_ptr++;
        port_start = uribuf_ptr;/*remember the start of port section*/
        while (*uribuf_ptr != '/') {
            uribuf_ptr++;
        }
        *uribuf_ptr = '\0';/*make strcpy happy*/
        strcpy(port_buf, port_start);/*get the port string*/
        *uribuf_ptr = '/';/*restore uribuf_ptr*/
        port = atoi(port_buf);/*convert to integer*/
        strcpy(suffix, uribuf_ptr);
    }
    return port;
}
/* config_header_server: configures the header to server
 * extract information from client's header
 * and configure a new header, which is forwarded to server
 */
void config_header_server (rio_t *client_riop, char *header_buf,
                           char *host, char *suffix) {
    char client_request_buf[MAXLINE];
    char host_buf[MAXLINE];
    char other_buf[MAXLINE];
    char key[MAXLINE];

    Rio_readlineb(client_riop, client_request_buf, MAXLINE);
    while (strcmp(client_request_buf, "\r\n")) {
        sprintf(host_buf, host_hdr, host);
        get_key_from_client_header(client_request_buf, key);
        if (!strcmp(key, "Host")) {
            memset(host_buf, 0, strlen(host_buf));
            strcpy(host_buf, client_request_buf);
        }
        else if (strcmp(key, "User-Agent") &&
                 strcmp(key, "Accept") &&
                 strcmp(key, "Accept-Encoding") &&
                 strcmp(key, "Connection") &&
                 strcmp(key, "Proxy-Connection")) {
            strcpy(other_buf, client_request_buf);
        }
        //read the next line
        if (Rio_readlineb(client_riop, client_request_buf, MAXLINE) < 0) {
            break;
        }
    }
    char header_buf_temp[MAXLINE];
    sprintf(header_buf_temp, "GET %s HTTP/1.0\r\n", suffix);
    strcat(header_buf, header_buf_temp);
    strcat(header_buf, host_buf);
    strcat(header_buf, user_agent_hdr);
    strcat(header_buf, accept_hdr);
    strcat(header_buf, accept_encoding_hdr);
    strcat(header_buf, connection_hdr);
    strcat(header_buf, proxy_connection_hdr);
    strcat(header_buf, "\r\n");
    strcat(header_buf, other_buf);

    return;
}
/* clienterror
 * configures error messages
 * I copied this function from CSAPP.
 */
void clienterror(int fd, char *cause, char *errnum, char *smsg, char *lmsg) {
    char buf[MAXLINE], body[MAXBUF];
    /* Build the HTTP response body */
    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, smsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, lmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, smsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}
/* doit_thread
 * basically a wrapper for doit
 */
void *doit_thread(void *vargp) {
    int connfd_client = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);
    doit(connfd_client);
    Close(connfd_client);
    return NULL;
}
/* serve_cached: send the content of a cached object back to client
 * Works when the requested object is in the cache
 */
void serve_cached (CM *mycache, char *uri, int connfd_client) {
    printf("Cache hit\n");
    CB *cached_obj =cache_get(mycache, uri);
    //write back to client
    if (rio_writen(connfd_client, cached_obj->data, cached_obj->size) < 0) {
        printf("Error occured when trying to write to client\n");
        if(errno == EPIPE) {
            Close(connfd_client);
            printf("Error occured when trying to write to client\n");
        }
    }
}
/* doit
 * called within doit_thread
 */
void doit(int connfd_client) {
    char client_request_buf[MAXLINE], method[MAXLINE], uri[MAXLINE];
    rio_t rio_client;
    int is_cached = 0;
    char version[MAXLINE];
    //read the request from client
    Rio_readinitb(&rio_client, connfd_client);
    if (Rio_readlineb(&rio_client, client_request_buf, MAXLINE) < 0) {
        Close(connfd_client);
        unix_error("Rio_readn ECONNRESET error");
    }
    sscanf(client_request_buf, "%s %s %s", method, uri, version);
    //check if the method is get
    if (strcmp(method, "GET") != 0) {
        clienterror(connfd_client, method, "501", "Not Implemented",
                    "Proxy does not implement this method");
        return;
    }
    //else, work!
    is_cached = cache_check(mycache, uri);
    //cache hit
    if (is_cached) {
        serve_cached(mycache, uri, connfd_client);
    }
    //cache miss
    else {
        //serve uncached
        printf("Cache miss\n");
        //parse the required information from uri
        char host[MAXLINE], suffix[MAXLINE];
        int port_server = parse_uri(uri, host, suffix);
        if (((port_server < 1000) || (port_server > 65535))
                && (port_server != 80)) {
            printf("Invalid port, please specify one within 1000~65535\n");
            exit(0);
        }
        //try to connect to the server
        int server_fd;
        if ((server_fd = open_clientfd_r(host, port_server)) < 0) {
            clienterror(connfd_client, "GET", "999", "Cannot connect to server",
                        "Note that the return number is not standard");
            Close(connfd_client);
            return;
        }
        //config the header to server and try to foward the header to server
        char header_server[MAXLINE];
        rio_t rio_server;
        config_header_server(&rio_client, header_server, host, suffix);
        if (rio_writen(server_fd, header_server, strlen(header_server)) < 0) {
            if (errno == EPIPE) {
                printf("Error occured when sending data to server\n");
                Close(server_fd);
                return;
            }
        }
        //read from server, write to client and buffer
        Rio_readinitb(&rio_server, server_fd);
        char object_buf[MAX_OBJECT_SIZE];
        int n = 0;
        int size = 0;
        char buf[MAXLINE];
        while ((n = Rio_readlineb(&rio_server, buf, MAXLINE)) != 0) {
            if (size+n <= MAX_OBJECT_SIZE) {
                memcpy(object_buf + size, buf, n);
                size += n;
            }
            //forward the object to client
            if (rio_writen(connfd_client, buf, n) < 0) {
                printf("Error occured when sending data to client\n");
            }
            memset(buf, 0, strlen(buf));
        }
        if (size <= MAX_OBJECT_SIZE) {
            printf("This object is not too big\n");
            cache_insert(mycache, uri, object_buf, size);
        }
        Close(server_fd);
    }
}
/* main function
 * the main routine
 * featuring figure 12.14, CSAPP 2e
 */
int main(int argc, char **argv) {
    int listenfd, *connfdp, port_client;
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    pthread_t tid;

    mycache = cache_create_new_cache();

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    port_client = atoi(argv[1]);
    Signal(SIGPIPE, SIG_IGN);

    if ((listenfd = Open_listenfd(port_client)) < 0) {
        fprintf(stderr, "Error: open_listenfd\n");
        exit(0);
    }

    while (1) {
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        Pthread_create(&tid, NULL, doit_thread, connfdp);
    }

    return 0;
}

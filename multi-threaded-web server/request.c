#include "io_helper.h"
#include "request.h"

#define MAXBUF (8192)
#define FIFO 0
#define SSF 1
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wait_buffer_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t wait_buffer_empty = PTHREAD_COND_INITIALIZER;

//
//	TODO: add code to create and manage the buffer
typedef struct client_request
{
    int fd;
    int filesize;
    char *filename;
    struct client_request *next;
} cli_req;
//using list for creating queue data structure for FIFO operations

typedef struct list
{
    cli_req *tail;
    int size;
} list;

list q = {NULL, 0};

int check_NULL() { return q.tail == 0; }
int check_BUFFER_max() { return q.size ==buffer_max_size ; }

void enqueue(int conn_fd, int file_size, char *filename)
{
    if (check_BUFFER_max())
    {
        printf("Queue is full");
        return;
    }
    cli_req *req = (cli_req *)malloc(sizeof(cli_req));
    req->fd = conn_fd;
    req->filesize = file_size;
    req->filename = strdup(filename);
    req->next = NULL;
    
    if (check_NULL())
    {
        q.tail = req;
        q.tail->next = q.tail;
    }
    else
    {
        req->next = q.tail->next;
        q.tail->next = req;
        q.tail = req;
    }
    q.size++;
    buffer_size++;

}

cli_req *dequeue_req()
{
    if (check_NULL())
    {
        printf("Queue is empty");
        return NULL;
    }
    // getting first cli_req;
    cli_req *first = q.tail->next;
    // removing cli_req
    if (q.tail->next == q.tail)
    {
        q.tail = NULL;
    }
    else
    {
        q.tail->next = first->next;
    }
    q.size--;
    buffer_size--;
    return first;

}

void enqueue_sff(int conn_fd, int file_size, char *filename)
{
    if (check_BUFFER_max())
    {
        printf("Queue is full");
        return;
    }
    cli_req *req = (cli_req *)malloc(sizeof(cli_req));
    req->fd = conn_fd;
    req->filesize = file_size;
    req->filename = strdup(filename);
    req->next = NULL;
    
    if (check_NULL())
    {
        q.tail = req;
        q.tail->next = q.tail;
    }
    else
    {
        cli_req *temp = q.tail->next;
        cli_req *prev = q.tail;
        while (temp != q.tail && temp->filesize < file_size)
        {
            prev = temp;
            temp = temp->next;
        }
        if (temp == q.tail && temp->filesize < file_size)
        {
            req->next = q.tail->next;
            q.tail->next = req;
            q.tail = req;
        }
        else
        {
            req->next = temp;
            prev->next = req;
        }
    }
    q.size++;
    buffer_size++;
}

cli_req *dequeue_sff_req()
{
    if (check_NULL())
    {
        printf("Queue is empty");
        return NULL;
    }
    // getting first cli_req;
    cli_req *first = q.tail->next;
    // removing cli_req
    if (q.tail->next == q.tail)
    {
        q.tail = NULL;
    }
    else
    {
        q.tail->next = first->next;
    }
    q.size--;
    buffer_size--;
    return first;
}

// Sends out HTTP response in case of errors
//
void request_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) {
    char buf[MAXBUF], body[MAXBUF];
    
    // Create the body of error message first (have to know its length for header)
    sprintf(body, ""
	    "<!doctype html>\r\n"
	    "<head>\r\n"
	    "  <title>OSTEP WebServer Error</title>\r\n"
	    "</head>\r\n"
	    "<body>\r\n"
	    "  <h2>%s: %s</h2>\r\n" 
	    "  <p>%s: %s</p>\r\n"
	    "</body>\r\n"
	    "</html>\r\n", errnum, shortmsg, longmsg, cause);
    
    // Write out the header information for this response
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    write_or_die(fd, buf, strlen(buf));
    
    sprintf(buf, "Content-Type: text/html\r\n");
    write_or_die(fd, buf, strlen(buf));
    
    sprintf(buf, "Content-Length: %lu\r\n\r\n", strlen(body));
    write_or_die(fd, buf, strlen(buf));
    
    // Write out the body last
    write_or_die(fd, body, strlen(body));
    
    // close the socket connection
    close_or_die(fd);
}

//
// Reads and discards everything up to an empty text line
//
void request_read_headers(int fd) {
    char buf[MAXBUF];
    
    readline_or_die(fd, buf, MAXBUF);
    while (strcmp(buf, "\r\n")) {
		readline_or_die(fd, buf, MAXBUF);
    }
    return;
}

//
// Return 1 if static, 0 if dynamic content (executable file)
// Calculates filename (and cgiargs, for dynamic) from uri
//
int request_parse_uri(char *uri, char *filename, char *cgiargs) {
    char *ptr;
    
    if (!strstr(uri, "cgi")) { 
	// static
	strcpy(cgiargs, "");
	sprintf(filename, ".%s", uri);
	if (uri[strlen(uri)-1] == '/') {
	    strcat(filename, "index.html");
	}
	return 1;
    } else { 
	// dynamic
	ptr = index(uri, '?');
	if (ptr) {
	    strcpy(cgiargs, ptr+1);
	    *ptr = '\0';
	} else {
	    strcpy(cgiargs, "");
	}
	sprintf(filename, ".%s", uri);
	return 0;
    }
}

//
// Fills in the filetype given the filename
//
void request_get_filetype(char *filename, char *filetype) {
    if (strstr(filename, ".html")) 
		strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif")) 
		strcpy(filetype, "image/gif");
    else if (strstr(filename, ".jpg")) 
		strcpy(filetype, "image/jpeg");
    else 
		strcpy(filetype, "text/plain");
}

//
// Handles requests for static content
//
void request_serve_static(int fd, char *filename, int filesize) {
    int srcfd;
    char *srcp, filetype[MAXBUF], buf[MAXBUF];
    
    request_get_filetype(filename, filetype);
    srcfd = open_or_die(filename, O_RDONLY, 0);
    
    // Rather than call read() to read the file into memory, 
    // which would require that we allocate a buffer, we memory-map the file
    srcp = mmap_or_die(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    close_or_die(srcfd);
    
    // put together response
    sprintf(buf, ""
	    "HTTP/1.0 200 OK\r\n"
	    "Server: OSTEP WebServer\r\n"
	    "Content-Length: %d\r\n"
	    "Content-Type: %s\r\n\r\n", 
	    filesize, filetype);
       
    write_or_die(fd, buf, strlen(buf));
    
    //  Writes out to the client socket the memory-mapped file 
    write_or_die(fd, srcp, filesize);
    munmap_or_die(srcp, filesize);
}

//
// Fetches the requests from the buffer and handles them (thread logic)
//
void* thread_request_serve_static(void* arg)
{
	// TODO: write code to actualy respond to HTTP requests

  //consumer code to fetch requests from buffer
  while(1){
    pthread_mutex_lock(&mutex);
    while(buffer_size == 0){
      pthread_cond_wait(&wait_buffer_empty, &mutex);
    }
    cli_req *req ;
    if (scheduling_algo == FIFO)
    {
      //FIFO 
      req= dequeue_req();

    }
    else
    {
      //SFF POP
       req = dequeue_sff_req();
    }
    
    //fetch request from buffer
    //handle request
    printf("Request for %s is removed from the buffer.Its size was %d\n",req->filename, req->filesize);
    pthread_cond_signal(&wait_buffer_full);
    pthread_mutex_unlock(&mutex);

    request_serve_static(req->fd, req->filename, req->filesize);
    close_or_die(req->fd);
  }

}

//
// Initial handling of the request
//
void request_handle(int fd) {
    int is_static;
    struct stat sbuf;
    char buf[MAXBUF], method[MAXBUF], uri[MAXBUF], version[MAXBUF];
    char filename[MAXBUF], cgiargs[MAXBUF];
    
	// get the request type, file path and HTTP version
    readline_or_die(fd, buf, MAXBUF);
    sscanf(buf, "%s %s %s", method, uri, version);
    printf("method:%s uri:%s version:%s\n", method, uri, version);

	// verify if the request type is GET or not
    if (strcasecmp(method, "GET")) {
		request_error(fd, method, "501", "Not Implemented", "server does not implement this method");
		return;
    }
    request_read_headers(fd);
    
	// check requested content type (static/dynamic)
    is_static = request_parse_uri(uri, filename, cgiargs);
    
	// get some data regarding the requested file, also check if requested file is present on server
    if (stat(filename, &sbuf) < 0) {
		request_error(fd, filename, "404", "Not found", "server could not find this file");
		return;
    }
    
	// verify if requested content is static
    if (is_static) {
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
			request_error(fd, filename, "403", "Forbidden", "server could not read this file");
			return;
		}
		
		// TODO: write code to add HTTP requests in the buffer based on the scheduling policy
    //producer code to add requests to buffer
    if (strstr(filename, ".."))
    {
      request_error(fd, filename, "403", "Forbidden", "Traversing up in filesystem is not allowed");
      return;
    }
    //producer code to add requests to buffer
    pthread_mutex_lock(&mutex);
    while( buffer_size >= buffer_max_size){
      pthread_cond_wait(&wait_buffer_full, &mutex);
    }
    //add request to buffer


    //FIFO
    if (scheduling_algo == FIFO)
    {
      //FIFO PUSH
      printf("adding request for %s to buffer\n",filename);
      enqueue(fd,sbuf.st_size,filename);
  
    }
    else
    {
      //SFF PUSH
       printf("adding request for %s to buffer\n",filename);
      enqueue_sff(fd,sbuf.st_size,filename);
    }
     printf("Request for %s is added to the buffer.\n", filename);
    pthread_cond_signal(&wait_buffer_empty); // Rule of thumb : separate condition variable for consumer.
    pthread_mutex_unlock(&mutex);

    } else {
		request_error(fd, filename, "501", "Not Implemented", "server does not serve dynamic content request");
    }
}

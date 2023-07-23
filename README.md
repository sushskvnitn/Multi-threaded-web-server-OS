
# Multi-threaded Web Server Operating System

The multi-threaded web server operating system is a project that aims to handle HTTP requests using a multi-threaded architecture. It includes both a basic single-threaded web server and a multi-threaded web server. The web server is capable of serving static content and handling dynamic content requests. Additionally, a basic single-threaded web client is provided for testing purposes.

## Project Architecture

The project follows a client-server architecture. The web server component listens for incoming HTTP requests and processes them. It can handle both static and dynamic content requests. The web client component sends HTTP requests to the server and receives and prints the HTTP response.

## Folders and Components

The project is organized into two main folders:

1. Basic single-threaded webserver:

- Makefile: Used to compile the basic single-threaded web server and web client programs.

- io_helper.c: Contains functions related to input/output operations for the web server.

- io_helper.h: Contains helper functions and macros for input/output operations.

- request.c: Implements functions for handling HTTP requests in the web server.

- request.h: Defines constants and function prototypes related to handling HTTP requests.

- wclient.c: A basic single-threaded web client that sends HTTP requests and prints the response.

- wserver.c: Contains the main code for the basic single-threaded web server.

2. Multi-threaded web server:

- Makefile: Used to compile the multi-threaded web server and web client programs.

- io_helper.c: Contains functions related to input/output operations for the multi-threaded web server.

- io_helper.h: Contains helper functions and macros for input/output operations.

- request.c: Implements functions for handling HTTP requests in the multi-threaded web server.

- request.h: Defines constants and function prototypes related to handling HTTP requests.

- wclient.c: A primitive HTTP client used for testing the multi-threaded web server.

- wserver.c: Contains the main code for the multi-threaded web server.


## Usage

To compile and run the basic single-threaded web server, navigate to the "Basic single-threaded webserver" folder and run the following commands:
make
./wserver

To compile and run the multi-threaded web server, navigate to the "multi-threaded-web server" folder and run the following commands:
make
./wserver

## Implementation
The multi-threading implementation in `request.c` of the multi-threaded web server is based on POSIX threads (pthread).

The code uses multiple threads to handle incoming HTTP requests concurrently. Each thread is responsible for processing a single HTTP request.

Here is an overview of the multi-threading implementation in `request.c`:

1. Thread Creation: When a request is received by the server, a new thread is created to handle that request. This is done using the `pthread_create` function.

2. Thread Function: Each thread executes the `thread_request_serve_static` function, which is responsible for fetching requests from the buffer and handling them.

3. Buffer Synchronization: The buffer, which holds incoming requests, is accessed by multiple threads. To ensure thread safety, the code uses mutex locks and condition variables.

- Mutex Lock: The `pthread_mutex_lock` function is used to acquire a lock on the mutex before accessing the buffer.

- Condition Variables: Two condition variables, `wait_buffer_full` and `wait_buffer_empty`, are used to control the flow of threads based on the buffer status.

- `wait_buffer_full`: This condition variable is used by the producer thread to wait when the buffer is full.

- `wait_buffer_empty`: This condition variable is used by the consumer thread to wait when the buffer is empty.

4. Buffer Operations: The code includes functions for adding requests to the buffer (`enqueue` and `enqueue_sff`) and removing requests from the buffer (`dequeue_req` and `dequeue_sff_req`).

- `enqueue`: This function adds a request to the buffer using a First-In-First-Out (FIFO) approach.

- `enqueue_sff`: This function adds a request to the buffer using a Shortest-File-First (SFF) approach.

- `dequeue_req`: This function removes and returns the first request from the buffer using the FIFO approach.

- `dequeue_sff_req`: This function removes and returns the request with the shortest file size from the buffer using the SFF approach.

5. Request Handling: Once a request is fetched from the buffer, it is handled by calling the `request_serve_static` function, which processes the request and sends the appropriate response to the client.

Overall, this multi-threading implementation allows the multi-threaded web server to handle multiple requests concurrently, improving performance and responsiveness. It uses mutex locks and condition variables to synchronize access to shared resources and ensures thread safety.

## Working 

The code in `request.c` of the multi-threaded web server implements a multi-threading approach to handle incoming HTTP requests. It uses mutex locks, condition variables, and a buffer to synchronize and manage the requests.

1. Buffer Initialization: The code defines a buffer called `q` using a linked list data structure. The buffer is used to hold incoming requests. It also initializes a variable `buffer_size` to keep track of the number of requests in the buffer.

2. Mutex and Condition Variables: The code declares a mutex variable `mutex` using `pthread_mutex_t` and initializes it with `PTHREAD_MUTEX_INITIALIZER`. It also declares two condition variables `wait_buffer_full` and `wait_buffer_empty` using `pthread_cond_t` and initializes them with `PTHREAD_COND_INITIALIZER`. These synchronization primitives are used to ensure thread safety and control the flow of threads based on the buffer status.

3. Enqueue Operations: The code provides two enqueue functions - `enqueue` and `enqueue_sff` - to add requests to the buffer. These functions are called by the producer thread.

- `enqueue`: This function adds a request to the buffer using a First-In-First-Out (FIFO) approach. It creates a new `cli_req` structure and appends it to the tail of the linked list.

- `enqueue_sff`: This function adds a request to the buffer using a Shortest-File-First (SFF) approach. It traverses the linked list and inserts the request in the appropriate position based on the file size.

4. Dequeue Operations: The code provides two dequeue functions - `dequeue_req` and `dequeue_sff_req` - to remove requests from the buffer. These functions are called by the consumer thread.

- `dequeue_req`: This function removes and returns the first request from the buffer using the FIFO approach. It updates the tail pointer and decreases the buffer size.

- `dequeue_sff_req`: This function removes and returns the request with the shortest file size from the buffer using the SFF approach. It updates the tail pointer and decreases the buffer size.

5. Thread Function: The code defines the `thread_request_serve_static` function, which is the entry point for each thread handling an HTTP request. This function contains a while loop that continuously fetches requests from the buffer and processes them.

- Buffer Synchronization: Before accessing the buffer, the thread acquires the mutex lock using `pthread_mutex_lock`. If the buffer is empty, the thread waits on the `wait_buffer_empty` condition variable using `pthread_cond_wait`. This ensures that the thread is only executed when there are requests in the buffer.

- Request Handling: Once a request is fetched from the buffer, the thread calls the `request_serve_static` function to process the request and send the appropriate response to the client.

- Buffer Update: After handling the request, the thread signals the `wait_buffer_full` condition variable using `pthread_cond_signal` to notify the producer thread that the buffer is no longer full. It then releases the mutex lock using `pthread_mutex_unlock` so that other threads can access the buffer.

6. Main Function: The main function of the web server calls the `request_handle` function to handle incoming requests. This function verifies the request type, reads headers, parses the URI, and adds the requests to the buffer based on the scheduling policy.

- Mutex Lock Usage: The main function also uses mutex locks before accessing the buffer to synchronize the addition of requests.

- Condition Variables Usage: It uses condition variables to wait when the buffer is full (`wait_buffer_full`) and to signal when the buffer is empty (`wait_buffer_empty`).

Overall, the multi-threading implementation in `request.c` enables concurrent handling of HTTP requests using multiple threads. The mutex locks and condition variables ensure thread safety and synchronized access to the shared buffer. The buffer is used to store incoming requests, and the FCFS (First-Come-First-Served) and SFS (Shortest-File-Size) scheduling policies are implemented to manage the order of request processing.



## To run the project, you will need the following tools:

1. GCC: The GNU Compiler Collection (GCC) is required to compile the C code for the web server and client programs. Make sure you have GCC installed on your system.

2. Command Line Interface: The project is designed to be run using the command line interface (CLI). You will need a terminal or command prompt to navigate to the project directory and execute the necessary commands.

 ### Here are the steps to run the project:

1. Clone the Repository: Start by cloning the project repository to your local machine using the Git command:
  - ` git clone <repository-url> `
   

2. Compile the Programs: Navigate to the "Basic single-threaded webserver" or "multi-threaded-web server" folder depending on which version of the web server you want to run. Use the `make` command to compile the web server and client programs.
  - ` 1. cd Basic single-threaded webserver`
 -  ` 2. make`
   

or
 - ` 1. cd multi-threaded-web server `
 - ` 2. make `
   

3. Run the Web Server: Once the programs are successfully compiled, you can run the web server using the following command:
- `./wserver`
   
This will start the web server and it will begin listening for incoming HTTP requests.

4. Test the Web Server: Open a web browser or use a tool like cURL to send HTTP requests to the web server. You can use the web client program provided in the project to test the server.
  - `./wclient`
   
The web client will send a sample HTTP request to the server and print the response.

That's it! You have successfully run the multi-threaded web server project. You can now explore its features and customize it as per your requirements



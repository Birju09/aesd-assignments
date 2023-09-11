#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <syslog.h>
#include <signal.h>
#include <math.h>

#define SIZE 1024
#define FILE_TO_READ "/var/tmp/aesdsocketdata"
#define PORT "9000"
#define DISCARD_AFTER 10

int create_file(const char* path){
    int fd = open(path, O_RDWR | O_CREAT | O_NONBLOCK, 0600);

    if (fd == -1){
        syslog(LOG_ERR, "Cannot open the given file");
        return -1;
    }

    return fd;
}

void sigint_handler(int signal){
    syslog(LOG_INFO, "Caught signal, exiting");
    syslog(LOG_DEBUG, "Deleting file %s", FILE_TO_READ);
    remove(FILE_TO_READ);
    exit(EXIT_SUCCESS);
}

int create_socket(const char* port){

    // setup hints
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_next = NULL;
    hints.ai_addr = NULL;
    hints.ai_canonname = NULL;

    struct addrinfo * result;
    
    int ret = getaddrinfo(NULL, port, &hints, &result);

    if (ret != 0){
        perror("getaddrinfo error");
        return -1;
    }

    struct addrinfo* it;

    int sock_fd;
    int option = 1;

    for (it = result; it != NULL; it = it->ai_next){

        sock_fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);

        if (sock_fd == -1){
            continue;
        }

        if(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &option, sizeof(option)) == -1)
        {
            syslog(LOG_ERR, "setsockopt failure");
            return -1;
        }

        int ret_value = bind(sock_fd, it->ai_addr, it->ai_addrlen);

        if (ret_value == 0){
            break;
        }

        close(sock_fd);
    }

    // free the memory allocated for result
    freeaddrinfo(result);

    if (sock_fd == -1 || !it){
        syslog(LOG_ERR, "Unable to bind socket");
        return -1;
    }

    syslog(LOG_INFO, "Socket created");

    return sock_fd;
}


char* read_all(int file_fd){
    if (file_fd < 0){
        return NULL;
    }

    int sz = lseek(file_fd, 0, SEEK_END);
    if (sz == -1){
        syslog(LOG_ERR, "lseek failed");
    }

    lseek(file_fd, 0, SEEK_SET);


    char* buffer = malloc(sz);
    memset(buffer, 0, sz + 1 );

    if (read(file_fd, buffer, sz) == -1)
    {
        buffer = NULL;
    }


    return buffer;
}

char* read_sock(int sock_fd){

    if (sock_fd < 0){
        return NULL;
    }

    char* buffer = (char * )malloc(1);
    memset((void *) buffer, '\0', 1);
    int ret_value = -1;
    size_t capacity = 0;
    size_t filled = 0;

    do {
        char byte;
        ret_value = read(sock_fd, &byte, 1);

        if (ret_value == -1){
            break;
        }
        ++filled;
        if (filled >= capacity){
            capacity += filled * 4;
            syslog(LOG_DEBUG, "Expanding buffer to %ld", capacity);
            void* new = realloc((void*)buffer, capacity);

            if (!new){
                return NULL;
            }
            buffer = (char * )new;
        }

        buffer[filled] = '\0';
        buffer[filled - 1] = byte;

        if (byte == '\n'){
            break;
        }

    } while (ret_value != 0);

    return buffer;
}

int listen_messages(int sock_fd){
    if (sock_fd == -1){
        syslog(LOG_ERR, "Invalid socket, exiting");
        return -1;
    }

    syslog(LOG_DEBUG, "Cleaning up expected test file");
    remove(FILE_TO_READ);

    struct sockaddr_storage addr;

    int return_value = 0;

    unsigned long int total_bytes_recvd = 0;

    syslog(LOG_DEBUG, "Starting to listen messages on port 9000");
    int file_fd = create_file(FILE_TO_READ);
    if (file_fd == -1){
        syslog(LOG_ERR, "Failed to open the file");
        return_value = -1;
        return return_value;
    }

    if (listen(sock_fd, DISCARD_AFTER) < 0) {
        syslog(LOG_ERR, "listen failed");
        return return_value;
    }

    for (;;){
        socklen_t addr_len = sizeof(addr);
        int new_socket = accept(sock_fd, (struct sockaddr* )&addr, &addr_len);

        if (new_socket < 0){
            syslog(LOG_ERR, "no bytes received");
            usleep(1000);
            continue;
        }


        char * buffer = read_sock(new_socket);
        if (!buffer){
            syslog(LOG_ERR, "Read socket failed");
            return_value = -1;
            break;
        }

        syslog(LOG_DEBUG, "Message received bytes = %ld", strlen(buffer));


        // to set offset from the end of the file
        lseek(file_fd, 0, SEEK_END);

        int ret_value = write(file_fd, buffer, strlen(buffer));

        if (ret_value == -1){
            syslog(LOG_ERR, "File write failed");
            return_value = -1;
            break;
        }
        total_bytes_recvd += strlen(buffer);
        syslog(LOG_INFO, "Total bytes received so far %ld", total_bytes_recvd);

        char host[NI_MAXHOST], service[NI_MAXSERV];

        int ret = getnameinfo((struct sockaddr * ) &addr, addr_len, host, 
                                  NI_MAXHOST, service, NI_MAXSERV, 
                                  NI_NUMERICSERV);

        if (ret == 0){
            syslog(LOG_INFO, "Accepted connection from %s \n", host);
        }

        if (strchr(buffer, '\n') != NULL){
            return_value = 0;

            lseek(file_fd, 0, SEEK_SET);
            char * buf = read_all(file_fd);

            if (!buf)
            {
                syslog(LOG_ERR, "read failed");
                return_value = -1;
                break;
            }


            int ret = sendto(new_socket, buf, strlen(buf), 0, (struct sockaddr * ) &addr, addr_len);

            if (ret != strlen(buf)){
                syslog(LOG_ERR, "Error sending the contents back to client");
                return_value = -1;
                break;
            }

            close(new_socket);
            free(buf);
        }
        free(buffer);
    }

    close(file_fd);
    shutdown(sock_fd, SHUT_RDWR);

    return return_value;
}


int main(int argc, char * argv[]){
    
    int sock_fd = create_socket(PORT);

    if(sock_fd == -1){
        syslog(LOG_ERR, "Socket cannot be opened");
        exit(EXIT_FAILURE);
    }

    int do_fork = 0;

    if (argc > 1 && strcmp(argv[1], "-d") == 0){
        do_fork =1;
        printf("Socket daemon created \n");
    }

    int pid = 0;

    if (do_fork){
        syslog(LOG_DEBUG, "Forking");
        pid = fork();
    }

    if (pid < 0){
        syslog(LOG_ERR, "Fork Failed");
        exit(EXIT_FAILURE);
    }

    if (pid > 0){
        syslog(LOG_INFO, "Child process id %d", pid);
        exit(EXIT_SUCCESS);
    }

    if (do_fork){
        if (setsid() < 0){
            syslog(LOG_ERR, "setsid failed");
            exit(EXIT_FAILURE);
        }
    }


    signal(SIGINT, sigint_handler);
    signal(SIGABRT, sigint_handler);

    int ret_value = listen_messages(sock_fd);

    if (ret_value != 0){
        syslog(LOG_ERR, "Read failed");
        remove(FILE_TO_READ);
        exit(EXIT_FAILURE);
    }
    


    return EXIT_SUCCESS;
}
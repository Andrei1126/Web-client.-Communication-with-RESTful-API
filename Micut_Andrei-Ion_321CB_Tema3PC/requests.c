#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.c"

char *compute_get_request(char *host, char *url, char *query_params,
                            char **cookies, int cookies_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "HOST: %s", host);
    compute_message(message, line);
    
    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    sprintf(line, "Cookie: %s", cookies[0]);
    compute_message(message, line);

    // Step 4: add final new line
    compute_message(message, "");
    return message;
}

char *compute_get_request_book(char *host, char *url, char *query_params,
                            char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "HOST: %s", host);
    compute_message(message, line);
    // Step 3 (optional): add headers and/or cookies, according to the protocol format

    // adaug in header-ul Authorization Bearer (pentru a trimite token-ul catre server, cum
    // este mentionat si in enuntul temei) 
    sprintf(line, "Authorization: Bearer %s", token);
    compute_message(message, line);


    // Step 4: add final new line
    compute_message(message, "");
    return message;
}

char *compute_delete_book(char *host, char *url, char *query_params,
                            char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    // fata de functia compute_get_request, in header, in loc de GET, voi pune DELETE
    if (query_params != NULL) {
        sprintf(line, "DELETE %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "DELETE %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "HOST: %s", host);
    compute_message(message, line);
    
    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    // adaug in header-ul Authorization Bearer (pentru a trimite token-ul catre server, cum
    // este mentionat si in enuntul temei) 
    sprintf(line, "Authorization: Bearer %s", token);
    compute_message(message, line);


    // Step 4: add final new line
    compute_message(message, "");
    return message;
}


char *compute_post_request_user(char *host, char *url, char* content_type, char **body_data,
                            int body_data_fields_count, char **cookies, int cookies_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));

    // pentru a crea un obiect de tipul json voi avea nevoie sa declar obiectul,
    // valoarea si string-ul in care sa retin ceea ce primesc (luate din parson.c)
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    // Step 2: add the host
    sprintf(line, "HOST: %s", host);
    compute_message(message, line);
    /* Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
            in order to write Content-Length you must first compute the message size
    */
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);
    int len = 0;

    // aici voi forma obiectul propriu-zis de tipul json in care voi adauga 
    // username si password
    for(int i = 0; i < body_data_fields_count; i++) {
        if(i == 0) {
            json_object_set_string(root_object, "username", body_data[i]);
        }
        else {
            json_object_set_string(root_object, "password", body_data[i]);

        }
    }

    // voi retine ceea ce se afla in obiect intr-un string seializat
    serialized_string = json_serialize_to_string_pretty(root_value);

    // voi afisa string-ul serializat
    puts(serialized_string);

    // voi copia string-ul in buffer
    strcpy(body_data_buffer, serialized_string);

    // dezaloc
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);

    len = strlen(body_data_buffer);
   
    sprintf(line, "Content-Length: %d", len);
    compute_message(message, line);

    // Step 4 (optional): add cookies
    if (cookies != NULL) {
       
    }
    // Step 5: add new line at end of header
    compute_message(message, "");
    // Step 6: add the actual payload data
    memset(line, 0, LINELEN);
    compute_message(message, body_data_buffer);

    // dezaloc
    free(line);
    return message;
}

char *compute_post_request_book(char *host, char *url, char* content_type, char **body_data,
                            int body_data_fields_count, char *token, int token_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));
    int ok = 1;

    // pentru a crea un obiect de tipul json voi avea nevoie sa declar obiectul,
    // valoarea si string-ul in care sa retin ceea ce primesc (luate din parson.c)
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    // Step 2: add the host
    sprintf(line, "HOST: %s", host);
    compute_message(message, line);
    /* Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
            in order to write Content-Length you must first compute the message size
    */
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);
    int len = 0;

    // aici voi forma obiectul propriu-zis de tipul json in care voi adauga 
    // title, author, genre, publisher, page_count
    for(int i = 0; i < body_data_fields_count; i++) {

        if(i == 0) {
            json_object_set_string(root_object, "title", body_data[i]);
        }

        else if(i == 1)
        {
            json_object_set_string(root_object, "author", body_data[i]);
        }

        else if(i == 2)
        {
            json_object_set_string(root_object, "genre", body_data[i]);
        }

        else if(i == 3)
        {
            json_object_set_string(root_object, "publisher", body_data[i]);
        }

        else if(i == 4)
        {
            if(atoi(body_data[i]) > 0 && atoi(body_data[i]) < 999999)
            {
                json_object_set_number(root_object, "page_count", atoi(body_data[i]));
            }

            else
            {
                ok = 0;
            }
        }
    }
    
    if(ok)
    {
        // voi retine ceea ce se afla in obiect intr-un string seializat
        serialized_string = json_serialize_to_string_pretty(root_value);

        // voi afisa string-ul serializat
        puts(serialized_string);

        // voi copia string-ul in buffer
        strcpy(body_data_buffer, serialized_string);
    }
    else
    {
        strcpy(body_data_buffer, "Eroare la formatare");    
    }
    // dezaloc
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);

    len = strlen(body_data_buffer);
   
    sprintf(line, "Content-Length: %d", len);
    compute_message(message, line);

    // Step 4 (optional): add cookies
    if (token != NULL) {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);      
    }
    // Step 5: add new line at end of header
    compute_message(message, "");
    // Step 6: add the actual payload data
    memset(line, 0, LINELEN);
    compute_message(message, body_data_buffer);

    // dezaloc
    free(line);
    return message;
}


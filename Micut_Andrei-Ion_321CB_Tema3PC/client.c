#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"


// realizez inregistrarea unui user 
char** register_user(char* username, char* password)
{   
    
    // voi lua un vector de stringuri pentru a retine pe prima pozitie
    // username-ul, iar pe a doua password-ul
    
    int register_fields_count = 2;
    char **register_fields = (char**)malloc(register_fields_count * sizeof(char*));
    for(int i = 0; i < register_fields_count; i++) {
        register_fields[i] = (char*)malloc(sizeof(char));
    }
    strcpy(register_fields[0], username);
    strcpy(register_fields[1], password);

    // returnez vectorul cu string-urile necesare
    return register_fields;

}

char* login(int sockfd, char* message, char* username, char* password)
{

    // realizeaza logarea daca user-ul a fost inregistrat si intorc cookie de sesiune
    // daca credentialele nu se potrivesc, atunci se va intoarce mesaj de eroare

    char *response;

    // voi retine in login_fields credentialele utilizatorului
    int login_fields_count = 2;
    char **login_fields = (char**)malloc(login_fields_count * sizeof(char*));
    for(int i = 0; i < login_fields_count; i++) {
        login_fields[i] = (char*)malloc(sizeof(char));
    }

    // realizez formatul json si astfel alcatuiesc mesajul pe care il voi trimite serverului
    login_fields = register_user(username, password);

    //mesajul trimis de catre tester server-ului
    message = compute_post_request_user("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/tema/auth/login", "application/json", login_fields, login_fields_count, NULL, 0);
    
    // se va afisa ceea ce ii trimit eu server-ului in format json
    puts(message);
    
    // se va trimite server-ului
    send_to_server(sockfd, message);
    
    // primesc raspunsul de la server
    response = receive_from_server(sockfd);
    
    // afisez raspunsul server-ului
    puts(response);

    // realizez despartirea mesajului primit de la server 
    // pentru a putea lua cookie-ul
    // intial, am folost "\n" ca delimitator, insa primem Bad request
    // si de aceea am pus "\r\n"

    const char delim[4] = "\r\n";
    char *token;
    char* cookie = malloc(500 * sizeof(char));
    token = strtok(response, delim);

    while( token != NULL ) {

        // continui sa realizez despartirea mesajului pana cand am gasit
        // sirul de caractere: "Set-Cookie:", iar ce se afla dupa acesta
        // il voi retin intr-un string nou
        if(strncmp(token, "Set-Cookie:", 11) == 0) {
            strcpy(cookie, token);
            break;
        }
        token = strtok(NULL, delim);
    }

    // dupa ce am gasit linia de cookie de care am nevoie, ma voi deplasa
    // 12 caractere mai la dreapta pentru a putea lua continutul cookie-ului
    memmove(cookie, cookie + 12, strlen(cookie));

    // dezaloc
    for(int i = 0; i < login_fields_count; i++)
        free(login_fields[i]);


    free(login_fields);
    free(response);


    return cookie;
}

char* enter_library(int sockfd, char *message, char *cookie)
{
    // realizez intrarea in biblioteca doar daca cookie-ul pe care il primesc 
    // de la autentificare este valid si intorc token-ul JWT care imi va demonstra 
    // accesul la biblioteca
    // in caz contrar intorc un mesaj de eroare primit de la server daca credentialele 
    // nu se potrivesc

    char *response;

    //mesajul trimis de catre tester server-ului
    message = compute_get_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/tema/library/access", NULL, &cookie, 1);
    
    // se va afisa ceea ce ii trimit eu server-ului in format json
    puts(message);
    
    // se va trimite server-ului
    send_to_server(sockfd, message);
    
    // primesc raspunsul de la server
    response = receive_from_server(sockfd);
    
    // afisez raspunsul server-ului
    puts(response);

    // realizez despartirea mesajului primit de la server 
    // pentru a putea lua cookie-ul
    // intial, am folost "\n" ca delimitator, insa primem Bad request
    // si de aceea am pus "\r\n"
    const char delim[4] = "\r\n";
    char *token;
    char *token_jwt = malloc(500 * sizeof(char));
    char *token_jwt1 = malloc(500 * sizeof(char));
    token = strtok(response, delim);

    while(token != NULL)
    {
        // continui despartirea pana cand gasesc sirul de carcatere:
        // "{"token":"", iar ce se afla dupa acesta il voi retine
        // intr-un string nou
        if(strncmp(token, "{\"token\":\"", 10) == 0)
        {
            strcpy(token_jwt, token);
            break;
        }
        token = strtok(NULL, delim);
    }

    // dupa ce am gasit linia de token de care am nevoie, ma voi deplasa
    // 12 caractere mai la dreapta pentru a putea lua continutul token-ului
    memmove(token_jwt1, token_jwt + 10, strlen(token_jwt));

    //refolosesc token in care retinusem initial si "{"token":""
    memset(token_jwt, '\0', strlen(token_jwt));

    // retin in token_jwt continutul token-ului fara carcacterele ""}"
    memcpy(token_jwt, token_jwt1, strlen(token_jwt1) - 2);
    
    // dezaloc
    free(response);
    free(token_jwt1);

    return token_jwt;
}

void get_books(int sockfd, char *message, char *token)
{
    // functia imi va returna o liste de liste de forma ""id": Number, "title": String",
    // o lista de obiecte json, unde id-ul reprezinta id-ul cartii cu titlul title al user-ului
    // pentru a putea vedea cartile user-ului, trebuie ca token-ul primit in urma
    // logarii sa fie corect
    // in caz contrar, se va intoarce mesaj de eroare, deoarece nu avem acces la biblioteca

    char *response;

    // redeschid conexiunea
    sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);

    //mesajul trimis de catre tester server-ului
    message = compute_get_request_book("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/tema/library/books", NULL, token);
    
    // se va afisa ceea ce ii trimit eu server-ului in format json
    puts(message);
    
    // se va trimite server-ului
    send_to_server(sockfd, message);
    
    // primesc raspunsul de la server
    response = receive_from_server(sockfd);
    
    // afisez raspunsul server-ului
    puts(response);

    // dezaloc
    //free(response);
}

void get_book(int sockfd, char *message, char *token, char *url, char *id)
{
    // functia ma va ajuta sa vizualizez detaliile despre o carte anume
    // ce are id-ul "id"
    // trebuie sa demonstrez ca am acces la biblioteca pentru a putea vizualiza 
    // detaliile
    // se va intoarce un obiect de tip json 
    // in caz contrar, se va intoarce mesaj de eroare pentru ca nu am avut acces la biblioteca
    // (token incorect) sau daca id-ul nu e unul valid

    char *response;
    char *url1;

    // citesc un id
    printf("id=");
    scanf("%s", id);

    // concatenez id-ul la ruta de acces /api/v1/tema/library/books/
    url1 = strdup(url);
    strcat(url1, id);

    // redeschid conexiunea
    sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);

    //mesajul trimis de catre tester server-ului
    message = compute_get_request_book("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", url1, NULL, token);
    
    // se va afisa ceea ce ii trimit eu server-ului in format json
    puts(message);
    
    // se va trimite server-ului
    send_to_server(sockfd, message);
    
    // primesc raspunsul de la server
    response = receive_from_server(sockfd);
    
    // afisez raspunsul server-ului
    puts(response);

    // dezaloc
    //free(response);
}

void add_book(int sockfd, char *message, char *token, char *title, char *author, char *genre, char *publisher, char *page_count)
{
    // aceasta functie imi va realiza adaugarea unei carti in lista de carti,
    // adica imi va adauga id-ul si title-ul la lista
    // trebuie sa demonstrez ca am acces la biblioteca (prin intermediul token-ului)
    // pentru a putea face acest lucru
    // in caz contrar, server-ul va intoarce eroare, iar
    // daca informatiile introduse sunt incomplete sau nu respecta formatarea, din nou,
    // mi se va intoarce mesaj de eroare de la server

    char *response;
    char aux[10];

    // redeschid conexiunea
    sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);

    // se primesc informatiile referitoare la carte
    fgets(aux, 10, stdin);
    printf("title=");
    fgets(title, 50, stdin);
    printf("author=");
    fgets(author, 50, stdin);
    printf("genre=");
    fgets(genre, 50, stdin);
    printf("publisher=");
    fgets(publisher, 50, stdin);
    printf("page_count=");
    scanf("%s", page_count);

    // aloc spatiu pentru vectorul de string-uri in care voi retine informatiile
    int contents_of_book_count = 5;
    char **contents_of_book = (char**)malloc(contents_of_book_count * sizeof(char*));
    for(int i = 0; i < contents_of_book_count; i++) 
    {
        contents_of_book[i] = (char*)malloc(sizeof(char));
    }


    // retin informatiile referitoare la carte
    strcpy(contents_of_book[0], title);
    strcpy(contents_of_book[1], author);
    strcpy(contents_of_book[2], genre);
    strcpy(contents_of_book[3], publisher);
    strcpy(contents_of_book[4], page_count);

    //mesajul trimis de catre tester server-ului
    message = compute_post_request_book("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/tema/library/books", "application/json", contents_of_book, contents_of_book_count, token, 0);
    
    // se va afisa ceea ce ii trimit eu server-ului in format json
    puts(message);
    
    // se va trimite server-ului
    send_to_server(sockfd, message);
    
    // primesc raspunsul de la server
    response = receive_from_server(sockfd);
    
    // afisez raspunsul server-ului
    puts(response);

    // dezaloc
    free(response);
}

void delete_book(int sockfd, char *message, char *token, char *url, char *id)
{
    // aceasta functie imi va sterge o anumita carte prin intermediul id-ului primit
    // din nou, trebuie sa am acces la biblioteca (prin intermediul token-ului)
    // in caz contrar, programul va intoarce eroare
    // server-ul va intoarce eroare daca id-ul pt care efectuez cererea este invalid

    char *response;
    char *url1;

    // citesc un id
    printf("id=");
    scanf("%s", id);

    url1 = strdup(url);

    // concatenez id-ul la ruta de acces /api/v1/tema/library/books/
    strcat(url1, id);

    // redeschid conexiunea
    sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);

    //mesajul trimis de catre tester server-ului
    message = compute_delete_book("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", url1, NULL, token);
    
    // se va afisa ceea ce ii trimit eu server-ului in format json
    puts(message);
    
    // se va trimite server-ului
    send_to_server(sockfd, message);
    
    // primesc raspunsul de la server
    response = receive_from_server(sockfd);
    
    // afisez raspunsul server-ului
    puts(response);

    // dezaloc
    free(response);
}

           
void logout(int sockfd, char *message, char *cookie)
{
    // voi realiza delogarea cu ajutorul acestei functii
    // delogarea va fi realizata doar daca cookie-ul pe care il primesc in urma autentificarii
    // este unul valid
    // in caz contrar, programul va intoarce eroare pentru ca nu sunt autentificat

    char *response;

    // redeschid conexiunea
    sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);

    //mesajul trimis de catre tester server-ului
    message = compute_get_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/tema/auth/logout", NULL, &cookie, 1);

    // se va afisa ceea ce ii trimit eu server-ului in format json
    puts(message);

    // se va trimite server-ului
    send_to_server(sockfd, message);

    // primesc raspunsul de la server
    response = receive_from_server(sockfd);

    // afisez raspunsul server-ului
    puts(response);

    //dezaloc
    free(response);

}

void exit_from_program()
{
    exit(1);
}

int main(int argc, char *argv[])
{
    char *message;
    char *response;
    char *cookie;
    char *token;
    char *id;
    char *url;
    int sockfd;
    int ok_login = 0, ok_enter = 0, ok_logout = 1;


    // detaliile referitoare la o carte
    char *title;
    char *author;
    char *genre;
    char *publisher;
    char *page_count;

    char command[100];
    char username[50];
    char password[50];

    // aloc spatiu pentru fiecare variabila declarata de tipul char*
    cookie = (char*)malloc(500 * sizeof(char));
    token = (char*)malloc(500 * sizeof(char));
    title = (char*)malloc(50 * sizeof(char));
    author = (char*)malloc(50 * sizeof(char));
    genre = (char*)malloc(10 * sizeof(char));
    publisher = (char*)malloc(50 * sizeof(char));
    page_count = (char*)malloc(500 * sizeof(char));
    id = (char*)malloc(10 *  sizeof(char));
    url = (char*)malloc(35 *  sizeof(char));

    // in variabila url, voi retine ruta de acces pe care o voi folosi
    // in functiile get si delete, asa cum este precizat si in enuntul temei
    strcpy(url, "/api/v1/tema/library/books/");

    // deschid conexiunea
    sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);

    // voi retine username-ul si password-ul
    int register_fields_count = 2;
    char **register_fields = (char**)malloc(register_fields_count * sizeof(char*));
    for(int i = 0; i < register_fields_count; i++) {
        register_fields[i] = (char*)malloc(sizeof(char));
    }

    // se va continua sa se introduca comenzi pana cand se va intalni comanda "exit"
    while(1)
    {
        // programul primeste comanda
        printf("Command: ");
        scanf("%s", command);
        
        // tratez cazul in care comanda este "register"
        if(ok_login == 1 && strcmp(command, "register") == 0)
        {
            write(2, "Sunteti logat deja.\n", sizeof("Sunteti logat deja.") + 1);
        }
        else
            if(strcmp(command, "register") == 0) 
             {
                // se va intoarce eroare daca username-ul este deja folosit de cineva
                
                //redeschid conexiunea
                sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);

                // programul primeste username-ul si password-ul
                printf("username=");
                scanf("%s", username);
                printf("password=");
                scanf("%s", password);

                // retin credentialele primite si mesajul trimis de catre tester server-ului
                register_fields = register_user(username, password);
                message = compute_post_request_user("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", "/api/v1/tema/auth/register", "application/json", register_fields, register_fields_count, NULL, 0);
                
                // se va afisa ceea ce ii trimit eu server-ului in format json
                puts(message);

                // se va trimite server-ului
                send_to_server(sockfd, message);

                // primesc raspunsul de la server
                response = receive_from_server(sockfd);

                // afisez raspunsul server-ului
                puts(response);
             }


         // tratez cazul in care comanda este "login"
         if(ok_login == 1 && strcmp(command, "login") == 0)
         {
            write(2, "Sunteti deja autentificat!\n", sizeof("Sunteti deja autentificat!") + 1);
         }
         else
            if(strcmp(command, "login") == 0 && ok_login == 0) 
            {
                // redeschid conexiunea
                sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);

                // programul primeste username-ul si password-ul
                printf("username=");
                scanf("%s", username);
                printf("password=");
                scanf("%s", password);

                // retin in cookie, cookie-ul intors in urma logarii
                cookie = strdup(login(sockfd, message, username, password));
                ok_login++;
                ok_logout--;
            
            }
        // tratez cazul in care comanda este "enter_library"
        if(ok_login == 0 && strcmp(command, "enter_library") == 0)
        {
            write(2, "Nu sunteti logat!\n", sizeof("Nu sunteti logat!") + 1);
        }
        else
            if(ok_enter == 1 && strcmp(command, "enter_library") == 0)
            {
                write(2, "Sunteti deja in biblioteca!\n", sizeof("Sunteti deja in biblioteca!") + 1);
            }
        else
            if(strcmp(command,"enter_library") == 0)
            {   
                // redeschid conexiunea
                sockfd = open_connection("3.8.116.10", 8080, AF_INET, SOCK_STREAM, 0);

                // retin in token, token-ul intors in urma intrarii in librarie
                token = strdup(enter_library(sockfd, message, cookie));
                ok_enter++;
            }      

        // tratez cazul in care comanda este "get_books"
        if(ok_enter == 0 && strcmp(command, "get_books") == 0)
        {
            write(2, "Nu va aflati in biblioteca, ne pare rau.\n", sizeof("Nu va aflati in biblioteca, ne pare rau.") + 1);   
        }

        else

            if(strcmp(command, "get_books") == 0)
            {
                get_books(sockfd, message, token); 
            }
           
        // tratez cazul in care comanda este "get_book"   
        if(ok_enter == 0 && strcmp(command, "get_book") == 0)
        {
            // 28 este dimensiunea string-ului. 
            write(2, "Nu aveti aceasta persmiune!\n", 28); // Daca faceam cu sizeof imi afisa aCommand in loc de command
        } 
        else                 
            if(strcmp(command, "get_book") == 0)
            {
                get_book(sockfd, message, token, url, id);
            }

        // tratez cazul in care comanda este "add_book"
        if(ok_enter == 0 && strcmp(command, "add_book") == 0)
        {
            write(2, "Nu sunteti in biblioteca!\n", sizeof("Nu sunteti in biblioteca!") + 1);
        }
        else
            if(strcmp(command, "add_book") == 0)
            {
                add_book(sockfd, message, token, title, author, genre, publisher, page_count);
            }

        // tratez cazul in care comanda este "delete_book" 
        if(ok_enter == 0 && strcmp(command, "delete_book") == 0)
        {
            // 28 este dimensiunea string-ului.
            write(2, "Nu aveti aceasta persmiune!\n", 28); // Daca faceam cu sizeof imi afisa aCommand in loc de command
        }
        else 
            if(strcmp(command, "delete_book") == 0)
            {
                delete_book(sockfd, message, token, url, id);
            } 
                        
        // tratez cazul in care comanda este "logout"

        if(strcmp(command, "logout") == 0 && ok_logout == 1)
        {
            write(2, "Nu sunteti logat. Va rugam sa nu mai incercati.\n", sizeof("Nu sunteti logat. Va rugam sa nu mai incercati.") + 1);
        }
        else
            if(strcmp(command, "logout") == 0)
            {
                logout(sockfd, message, cookie);

                // programul va retine cookie-l si token-ul intors in urma autentificarii, respectiv
                // a intrarii in biblioteca si trebuie "uitate"
                free(cookie);
                free(token); 
                ok_login--;
                ok_enter--;
                ok_logout++;
            }

        if(strcmp(command, "exit") == 0)
        {
            exit_from_program();
        }
    }
     
    // dezaloc ce am alocat la inceput
    for(int i = 0; i < register_fields_count; i++)
        free(register_fields[i]);
    
    free(register_fields);
    free(title);
    free(author);
    free(genre);
    free(publisher);
    free(page_count);
    free(id);
    free(url);

    //inchid socket-ul
    close(sockfd);
    return 0;
}


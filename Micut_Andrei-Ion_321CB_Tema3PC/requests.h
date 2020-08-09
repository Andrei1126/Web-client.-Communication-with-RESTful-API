#ifndef _REQUESTS_
#define _REQUESTS_

// computes and returns a GET request string (query_params
// and cookies can be set to NULL if not needed)
char *compute_get_request(char *host, char *url, char *query_params,
							char **cookies, int cookies_count);

char *compute_get_request_book(char *host, char *url, char *query_params,
                            char *token);

char *compute_delete_book(char *host, char *url, char *query_params,
                            char *token);

// computes and returns a POST request string (cookies can be NULL if not needed)
char *compute_post_request_user(char *host, char *url, char* content_type, char **body_data,
                            int body_data_fields_count, char **cookies, int cookies_count);

char *compute_post_request_book(char *host, char *url, char* content_type, char **body_data,
                            int body_data_fields_count, char *token, int token_count);

#endif

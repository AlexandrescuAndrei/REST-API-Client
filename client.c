#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "requests.h"
#include "helper.h"
#include "parson.h"

#define SERVER_IP "63.32.125.183"
#define SERVER_PORT 8081
#define HOST_HEADER "34.118.48.238"

char *admin_cookie = NULL;
char *user_cookie = NULL;
char *jwt_token = NULL;

void login_admin() {
    char username[BUFLEN], password[BUFLEN];

    printf("username=");
    fflush(stdout);
    fgets(username, BUFLEN, stdin);
    username[strcspn(username, "\n")] = 0;

    printf("password=");
    fflush(stdout);
    fgets(password, BUFLEN, stdin);
    password[strcspn(password, "\n")] = 0;
    
    JSON_Value *val = json_value_init_object();
    JSON_Object *obj = json_value_get_object(val);
    
    json_object_set_string(obj, "username", username);
    json_object_set_string(obj, "password", password);
    char **data = malloc(sizeof(char *));
    data[0] = json_serialize_to_string(val);

    char *message = compute_post_request(
        HOST_HEADER,
        "/api/v1/tema/admin/login",
        "application/json",
        data,
        1,
        NULL,
        0, 
        NULL, 
        0
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    char *cookie_start = strstr(response, "Set-Cookie:");
    if (cookie_start != NULL) {
        cookie_start += strlen("Set-Cookie: ");
        char *cookie_end = strchr(cookie_start, ';');
        if (cookie_end != NULL) {
            size_t len = cookie_end - cookie_start;
            if (admin_cookie) free(admin_cookie);
            admin_cookie = calloc(len + 1, sizeof(char));
            strncpy(admin_cookie, cookie_start, len);
        }
        printf("SUCCESS: Admin autentificat cu succes\n");
    } else {
        printf("ERROR: Autentificare eșuată\n");
    }

    json_value_free(val);
    free(message);
    free(response);
    close_connection(sockfd);
}

void add_user() {
    char username[BUFLEN], password[BUFLEN];

    printf("username=");
    fflush(stdout);
    fgets(username, BUFLEN, stdin);
    username[strcspn(username, "\n")] = 0;

    printf("password=");
    fflush(stdout);
    fgets(password, BUFLEN, stdin);
    password[strcspn(password, "\n")] = 0;

    if (admin_cookie == NULL) {
        printf("ERROR: Trebuie să fii logat ca admin.\n");
        return;
    }

    JSON_Value *val = json_value_init_object();
    JSON_Object *obj = json_value_get_object(val);
    json_object_set_string(obj, "username", username);
    json_object_set_string(obj, "password", password);

    char **data = malloc(sizeof(char *));
    data[0] = json_serialize_to_string(val);

    char **cookies = malloc(sizeof(char *));
    cookies[0] = admin_cookie;

    char *message = compute_post_request(
        HOST_HEADER,
        "/api/v1/tema/admin/users",
        "application/json",
        data,
        1,
        cookies,
        1,
        NULL,
        0
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "error") != NULL || strstr(response, "ERROR") != NULL) {
        printf("ERROR: %s\n", response);
    } else {
        printf("SUCCESS: Utilizator adăugat cu succes\n");
    }

    json_value_free(val);
    free(message);
    free(response);
    close_connection(sockfd);
}

void get_users() {
    if (admin_cookie == NULL) {
        printf("ERROR: Trebuie să fii logat ca admin.\n");
        return;
    }

    char **cookies = malloc(sizeof(char *));
    cookies[0] = admin_cookie;

    char *message = compute_get_request(
        HOST_HEADER,
        "/api/v1/tema/admin/users",
        NULL,
        cookies,
        1,
        NULL,
        0
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "error") || strstr(response, "ERROR")) {
        printf("ERROR: %s\n", response);
    } else {
        printf("SUCCESS: Lista utilizatorilor\n");
        char *body = strstr(response, "\r\n\r\n");
        if (body != NULL) {
            body += 4;
            JSON_Value *val = json_parse_string(body);
            if (val) {
                JSON_Object *obj = json_value_get_object(val);
                JSON_Array *users = json_object_get_array(obj, "users");

                for (size_t i = 0; i < json_array_get_count(users); i++) {
                    JSON_Object *user = json_array_get_object(users, i);
                    const char *username = json_object_get_string(user, "username");
                    const char *password = json_object_get_string(user, "password");
                    int id = json_object_get_number(user, "id");

                    printf("#%d %s:%s\n", id, username, password);
                }

                json_value_free(val);
            } else {
                printf("ERROR: Nu s-a putut parsa răspunsul JSON.\n");
            }
        } else {
            printf("ERROR: Răspuns invalid de la server.\n");
        }
    }

    free(message);
    free(response);
    close_connection(sockfd);
}

void delete_user() {
    if (admin_cookie == NULL) {
        printf("ERROR: Trebuie să fii logat ca admin.\n");
        return;
    }

    char username[BUFLEN];
    printf("username=");
    fflush(stdout);
    fgets(username, BUFLEN, stdin);
    username[strcspn(username, "\n")] = 0;

    char endpoint[BUFLEN];
    snprintf(endpoint, sizeof(endpoint), "/api/v1/tema/admin/users/%s", username);

    char **cookies = malloc(sizeof(char *));
    cookies[0] = admin_cookie;

    char *message = compute_delete_request(
        HOST_HEADER,
        endpoint,
        NULL,
        cookies,
        1,
        NULL,
        0
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "error") || strstr(response, "ERROR")) {
        printf("ERROR: %s\n", response);
    } else {
        printf("SUCCESS: Utilizator șters\n");
    }

    free(message);
    free(response);
    close_connection(sockfd);
}

void login() {
    char admin_username[BUFLEN], username[BUFLEN], password[BUFLEN];

    printf("admin_username=");
    fflush(stdout);
    fgets(admin_username, BUFLEN, stdin);
    admin_username[strcspn(admin_username, "\n")] = 0;

    printf("username=");
    fflush(stdout);
    fgets(username, BUFLEN, stdin);
    username[strcspn(username, "\n")] = 0;

    printf("password=");
    fflush(stdout);
    fgets(password, BUFLEN, stdin);
    password[strcspn(password, "\n")] = 0;

    JSON_Value *val = json_value_init_object();
    JSON_Object *obj = json_value_get_object(val);
    
    json_object_set_string(obj, "username", username);
    json_object_set_string(obj, "password", password);
    json_object_set_string(obj, "admin_username", admin_username);

    char **data = malloc(sizeof(char *));
    data[0] = json_serialize_to_string(val);

    char *message = compute_post_request(
        HOST_HEADER,
        "/api/v1/tema/user/login",
        "application/json",
        data,
        1,
        NULL,
        0,
        NULL,
        0
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    char *cookie_start = strstr(response, "Set-Cookie:");
    if (cookie_start != NULL) {
        cookie_start += strlen("Set-Cookie: ");
        char *cookie_end = strchr(cookie_start, ';');
        if (cookie_end != NULL) {
            size_t len = cookie_end - cookie_start;
            if (user_cookie) free(user_cookie);
            user_cookie = calloc(len + 1, sizeof(char));
            strncpy(user_cookie, cookie_start, len);
        }
        printf("SUCCESS: Autentificare reușită\n");
    } else {
        printf("ERROR: Autentificare eșuată\n");
    }

    json_value_free(val);
    free(message);
    free(response);
    close_connection(sockfd);
}

void logout_admin() {
    if (admin_cookie == NULL) {
        printf("ERROR: Nu ești autentificat ca admin.\n");
        return;
    }

    char **cookies = malloc(sizeof(char *));
    cookies[0] = admin_cookie;

    char *message = compute_get_request(
        HOST_HEADER,
        "/api/v1/tema/admin/logout",
        NULL,
        cookies,
        1,
        NULL,
        0
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "error") != NULL || strstr(response, "ERROR") != NULL) {
        printf("ERROR: %s\n", response);
    } else {
        printf("SUCCESS: Admin delogat\n");
        free(admin_cookie);
        admin_cookie = NULL;
    }

    free(message);
    free(response);
    close_connection(sockfd);
}

void get_access() {
    if (user_cookie == NULL) {
        printf("ERROR: Trebuie să fii logat ca utilizator.\n");
        return;
    }

    char *cookies[] = { user_cookie };
    char *headers[] = { "Accept: application/json" }; // AICI era lipsa

    char *message = compute_get_request(
        HOST_HEADER,
        "/api/v1/tema/library/access",
        NULL,
        cookies,
        1,
        headers,
        1
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "error") || strstr(response, "ERROR") || strstr(response, "401")) {
        printf("ERROR: %s\n", response);
    } else {
        printf("SUCCESS: Token JWT primit\n");

        char *body = strstr(response, "\r\n\r\n");
        if (body != NULL) {
            body += 4;
            JSON_Value *val = json_parse_string(body);
            if (val != NULL) {
                JSON_Object *obj = json_value_get_object(val);
                const char *token = json_object_get_string(obj, "token");
                if (token != NULL) {
                    if (jwt_token) free(jwt_token);
                    jwt_token = strdup(token);
                }
                json_value_free(val);
            }
        }
    }

    free(message);
    free(response);
    close_connection(sockfd);
}

void get_movies() {
    if (user_cookie == NULL || jwt_token == NULL) {
        printf("ERROR: Trebuie să fii logat și să obții token JWT (get_access).\n");
        return;
    }

    char auth_header[BUFLEN];
    snprintf(auth_header, BUFLEN, "Authorization: Bearer %s", jwt_token);
    char **headers = malloc(sizeof(char *));
    headers[0] = auth_header;

    char **cookies = malloc(sizeof(char *));
    cookies[0] = user_cookie;

    char *message = compute_get_request(
        HOST_HEADER,
        "/api/v1/tema/library/movies",
        NULL,
        cookies,
        1,
        headers,
        1
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "error") || strstr(response, "ERROR") || strstr(response, "401")) {
        printf("ERROR: %s\n", response);
    } else {
        printf("SUCCESS: Lista filmelor\n");

        char *body = strstr(response, "\r\n\r\n");
        if (body != NULL) {
            body += 4;

            JSON_Value *val = json_parse_string(body);
            if (val != NULL && json_value_get_type(val) == JSONObject) {
                JSON_Object *obj = json_value_get_object(val);
                JSON_Array *movies = json_object_get_array(obj, "movies");

                if (movies == NULL) {
                    printf("ERROR: Nu s-a găsit câmpul 'movies' în JSON.\n");
                } else {
                    for (size_t i = 0; i < json_array_get_count(movies); i++) {
                        JSON_Object *movie = json_array_get_object(movies, i);
                        const char *title = json_object_get_string(movie, "title");
                        int id = json_object_get_number(movie, "id");

                        printf("#%d %s\n", id, title);
                    }
                }

                json_value_free(val);
            } else {
                printf("ERROR: Răspunsul JSON nu este un obiect valid.\n");
            }
        } else {
            printf("ERROR: Răspuns invalid de la server.\n");
        }
    }

    free(message);
    free(response);
    close_connection(sockfd);
}

void add_movie() {
    if (user_cookie == NULL || jwt_token == NULL) {
        printf("ERROR: Trebuie să fii logat și să ai token JWT.\n");
        return;
    }

    char title[BUFLEN], year[BUFLEN], description[BUFLEN], rating[BUFLEN];

    printf("title=");
    fflush(stdout);
    fgets(title, BUFLEN, stdin);
    title[strcspn(title, "\n")] = 0;

    printf("year=");
    fflush(stdout);
    fgets(year, BUFLEN, stdin);
    year[strcspn(year, "\n")] = 0;

    printf("description=");
    fflush(stdout);
    fgets(description, BUFLEN, stdin);
    description[strcspn(description, "\n")] = 0;

    printf("rating=");
    fflush(stdout);
    fgets(rating, BUFLEN, stdin);
    rating[strcspn(rating, "\n")] = 0;

    JSON_Value *val = json_value_init_object();
    JSON_Object *obj = json_value_get_object(val);
    json_object_set_string(obj, "title", title);
    json_object_set_number(obj, "year", atoi(year));
    json_object_set_string(obj, "description", description);
    json_object_set_number(obj, "rating", atof(rating));
    char **body_data = malloc(sizeof(char *));
    body_data[0] = json_serialize_to_string(val);

    char auth_header[BUFLEN];
    snprintf(auth_header, BUFLEN, "Authorization: Bearer %s", jwt_token);
    char **headers = malloc(sizeof(char *));
    headers[0] = auth_header;

    char **cookies = malloc(sizeof(char *));
    cookies[0] = user_cookie;

    char *message = compute_post_request(
        HOST_HEADER,
        "/api/v1/tema/library/movies",
        "application/json",
        body_data,
        1,
        cookies,
        1,
        headers,
        1
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "error") || strstr(response, "ERROR")) {
        printf("ERROR: %s\n", response);
    } else {
        printf("SUCCESS: Film adăugat\n");
    }

    json_value_free(val);
    free(message);
    free(response);
    close_connection(sockfd);
}

void get_movie() {
    if (user_cookie == NULL || jwt_token == NULL) {
        printf("ERROR: Trebuie să fii logat și să obții token JWT (get_access).\n");
        return;
    }

    char id_str[BUFLEN];
    printf("id=");
    fflush(stdout);
    fgets(id_str, BUFLEN, stdin);
    id_str[strcspn(id_str, "\n")] = 0;

    char endpoint[BUFLEN];
    snprintf(endpoint, BUFLEN, "/api/v1/tema/library/movies/%s", id_str);

    char auth_header[BUFLEN];
    snprintf(auth_header, BUFLEN, "Authorization: Bearer %s", jwt_token);
    char *headers[] = { auth_header };

    char **cookies = malloc(sizeof(char *));
    cookies[0] = user_cookie;

    char *message = compute_get_request(
        HOST_HEADER,
        endpoint,
        NULL,
        cookies,
        1,
        headers,
        1
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "error") || strstr(response, "ERROR") || strstr(response, "404") || strstr(response, "401")) {
        printf("ERROR: %s\n", response);
    } else {
        printf("SUCCESS: Film găsit\n");

        char *body = strstr(response, "\r\n\r\n");
        if (body != NULL) {
            body += 4;

            JSON_Value *val = json_parse_string(body);
            if (val != NULL) {
                JSON_Object *movie = json_value_get_object(val);
                printf("id=%d\n", (int)json_object_get_number(movie, "id"));
                printf("title=%s\n", json_object_get_string(movie, "title"));
                printf("year=%d\n", (int)json_object_get_number(movie, "year"));
                printf("description=%s\n", json_object_get_string(movie, "description"));
                printf("rating=%s\n", json_object_get_string(movie, "rating"));

                json_value_free(val);
            } else {
                printf("ERROR: Nu s-a putut parsa JSON-ul.\n");
            }
        } else {
            printf("ERROR: Răspuns invalid de la server.\n");
        }
    }

    free(message);
    free(response);
    close_connection(sockfd);
}

void update_movie() {
    if (user_cookie == NULL || jwt_token == NULL) {
        printf("ERROR: Trebuie să fii logat și să ai token JWT.\n");
        return;
    }

    char id_str[BUFLEN], title[BUFLEN], year_str[BUFLEN], description[BUFLEN], rating_str[BUFLEN];

    printf("id=");
    fflush(stdout);
    fgets(id_str, BUFLEN, stdin); id_str[strcspn(id_str, "\n")] = 0;

    printf("title=");
    fflush(stdout);
    fgets(title, BUFLEN, stdin); title[strcspn(title, "\n")] = 0;

    printf("year=");
    fflush(stdout);
    fgets(year_str, BUFLEN, stdin); year_str[strcspn(year_str, "\n")] = 0;

    printf("description=");
    fflush(stdout);
    fgets(description, BUFLEN, stdin); description[strcspn(description, "\n")] = 0;

    printf("rating=");
    fflush(stdout);
    fgets(rating_str, BUFLEN, stdin); rating_str[strcspn(rating_str, "\n")] = 0;

    JSON_Value *val = json_value_init_object();
    JSON_Object *obj = json_value_get_object(val);

    json_object_set_string(obj, "title", title);
    json_object_set_number(obj, "year", atoi(year_str));
    json_object_set_string(obj, "description", description);
    json_object_set_number(obj, "rating", atof(rating_str));

    char *body = json_serialize_to_string(val);
    printf("%s\n", body);
    char *body_data[] = { body };

    char jwt_header[BUFLEN];
    snprintf(jwt_header, BUFLEN, "Authorization: Bearer %s", jwt_token);
    char *headers[] = { jwt_header };

    char endpoint[BUFLEN];
    snprintf(endpoint, BUFLEN, "/api/v1/tema/library/movies/%s", id_str);

    char *message = compute_put_request(
        HOST_HEADER,
        endpoint,
        "application/json",
        body_data,
        1,
        NULL,
        0,
        headers,
        1
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "error") || strstr(response, "ERROR") || strstr(response, "401")) {
        printf("ERROR: %s\n", response);
    } else {
        printf("SUCCESS: Film actualizat\n");
    }

    json_free_serialized_string(body);
    json_value_free(val);
    free(message);
    free(response);
    close_connection(sockfd);
}

void delete_movie() {
    if (user_cookie == NULL || jwt_token == NULL) {
        printf("ERROR: Trebuie să fii logat și să ai token JWT.\n");
        return;
    }

    char id_str[BUFLEN];
    printf("id=");
    fflush(stdout);
    fgets(id_str, BUFLEN, stdin);
    id_str[strcspn(id_str, "\n")] = 0;

    char endpoint[BUFLEN];
    snprintf(endpoint, BUFLEN, "/api/v1/tema/library/movies/%s", id_str);

    char jwt_header[BUFLEN];
    snprintf(jwt_header, BUFLEN, "Authorization: Bearer %s", jwt_token);
    char *headers[] = { jwt_header };

    char *cookies[] = { user_cookie };

    char *message = compute_delete_request(
        HOST_HEADER,
        endpoint,
        NULL,
        cookies,
        1,
        headers,
        1
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "error") || strstr(response, "ERROR") || strstr(response, "401")) {
        printf("ERROR: %s\n", response);
    } else {
        printf("SUCCESS: Film șters\n");
    }

    free(message);
    free(response);
    close_connection(sockfd);
}

void add_collection() {
    if (!jwt_token) {
        puts("ERROR: Trebuie să fii autentificat.");
        return;
    }

    char titlu[BUFLEN];
    int nr_filme = 0;

    printf("title=");
    fgets(titlu, BUFLEN, stdin);
    titlu[strcspn(titlu, "\n")] = 0;

    printf("num_movies=");
    scanf("%d%*c", &nr_filme);

    if (nr_filme <= 0) {
        puts("ERROR: Numar invalid de filme.");
        return;
    }

    int filme[nr_filme];
    for (int k = 0; k < nr_filme; ++k) {
        printf("movie_id[%d]=", k);
        scanf("%d%*c", &filme[k]);
    }

    JSON_Value *json_title_val = json_value_init_object();
    JSON_Object *json_title_obj = json_value_get_object(json_title_val);
    json_object_set_string(json_title_obj, "title", titlu);
    char **payload = malloc(sizeof(char *));
    payload[0] = json_serialize_to_string(json_title_val);

    char auth[BUFLEN];
    snprintf(auth, BUFLEN, "Authorization: Bearer %s", jwt_token);
    char *headers[] = { auth };

    char *req = compute_post_request(SERVER_IP,
                                     "/api/v1/tema/library/collections",
                                     "application/json",
                                     payload, 1,
                                     NULL, 0,
                                     headers, 1);
    int sock = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sock, req);
    char *resp = receive_from_server(sock);
    close_connection(sock);
    free(req);
    json_value_free(json_title_val);

    char *json_ptr = strstr(resp, "{");
    if (!json_ptr) {
        puts("ERROR: Răspuns invalid de la server.");
        free(resp);
        return;
    }

    JSON_Value *response_val = json_parse_string(json_ptr);
    if (!response_val) {
        puts("ERROR: Nu s-a putut parsa JSON-ul.");
        free(resp);
        return;
    }

    JSON_Object *response_obj = json_value_get_object(response_val);
    if (!response_obj) {
        json_value_free(response_val);
        free(resp);
        return;
    }

    int id_colectie = (int)json_object_get_number(response_obj, "id");
    json_value_free(response_val);

    if (id_colectie <= 0) {
        puts("ERROR: ID colecție invalid.");
        free(resp);
        return;
    }

    puts("SUCCESS: Colecție adăugată");
    for (int j = 0; j < nr_filme; ++j) {
        char ruta[BUFLEN];
        snprintf(ruta, BUFLEN, "/api/v1/tema/library/collections/%d/movies", id_colectie);

        JSON_Value *val = json_value_init_object();
        JSON_Object *obj = json_value_get_object(val);
        json_object_set_number(obj, "id", filme[j]);

        char **movie_payload = malloc(sizeof(char *));
        movie_payload[0] = json_serialize_to_string(val);

        char *movie_req = compute_post_request(SERVER_IP, ruta,
                                               "application/json",
                                               movie_payload, 1,
                                               NULL, 0,
                                               headers, 1);

        int sfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
        send_to_server(sfd, movie_req);
        char *movie_resp = receive_from_server(sfd);
        close_connection(sfd);

        if (!strstr(movie_resp, "200 OK") && !strstr(movie_resp, "201 CREATED")) {
            puts("ERROR: Film invalid, adăugare oprită.");
            free(movie_resp);
            free(movie_req);
            json_value_free(val);
            free(resp);
            return;
        }

        free(movie_resp);
        free(movie_req);
        json_value_free(val);
    }

    free(resp);
}

void get_collections() {
    if (user_cookie == NULL || jwt_token == NULL) {
        printf("ERROR: Trebuie să fii logat și să ai token JWT.\n");
        return;
    }

    char jwt_header[BUFLEN];
    snprintf(jwt_header, BUFLEN, "Authorization: Bearer %s", jwt_token);
    char **headers = malloc(sizeof(char *));
    headers[0] = jwt_header;

    char **cookies = malloc(sizeof(char *));
    cookies[0] = user_cookie;

    char *message = compute_get_request(
        HOST_HEADER,
        "/api/v1/tema/library/collections",
        NULL,
        cookies,
        1,
        headers,
        1
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "error") || strstr(response, "ERROR") || strstr(response, "401")) {
        printf("ERROR: %s\n", response);
    } else {
        printf("SUCCESS: Lista colecțiilor\n");

        char *body = strstr(response, "\r\n\r\n");
        if (body != NULL) {
            body += 4;

            JSON_Value *val = json_parse_string(body);
            if (val != NULL) {
                JSON_Object *obj = json_value_get_object(val);
                JSON_Array *collections = json_object_get_array(obj, "collections");

                if (collections == NULL) {
                    printf("ERROR: Nu s-a găsit câmpul 'collections' în JSON.\n");
                } else {
                    for (size_t i = 0; i < json_array_get_count(collections); i++) {
                        JSON_Object *collection = json_array_get_object(collections, i);
                        int id = (int)json_object_get_number(collection, "id");
                        const char *title = json_object_get_string(collection, "title");

                        printf("#%d %s\n", id, title);
                    }
                }

                json_value_free(val);
            } else {
                printf("ERROR: Nu s-a putut parsa JSON-ul colecțiilor.\n");
            }
        } else {
            printf("ERROR: Răspuns invalid de la server.\n");
        }
    }

    free(message);
    free(response);
    close_connection(sockfd);
}

void get_collection() {
    if (user_cookie == NULL || jwt_token == NULL) {
        printf("ERROR: Trebuie să fii logat și să ai token JWT.\n");
        return;
    }

    int id;
    printf("id=");
    scanf("%d", &id);

    char jwt_header[BUFLEN];
    snprintf(jwt_header, BUFLEN, "Authorization: Bearer %s", jwt_token);
    char *headers[] = { jwt_header };

    char *cookies[] = { user_cookie };

    char endpoint[BUFLEN];
    snprintf(endpoint, BUFLEN, "/api/v1/tema/library/collections/%d", id);

    char *message = compute_get_request(
        HOST_HEADER,
        endpoint,
        NULL,
        cookies,
        1,
        headers,
        1
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "error") || strstr(response, "ERROR") || strstr(response, "401")) {
        printf("ERROR: %s\n", response);
    } else {
        char *body = strstr(response, "\r\n\r\n");
        if (body != NULL) {
            body += 4;

            JSON_Value *val = json_parse_string(body);
            if (val != NULL) {
                JSON_Object *obj = json_value_get_object(val);
                JSON_Array *movies = json_object_get_array(obj, "movies");
                const char *title = json_object_get_string(obj, "title");
                const char *owner = json_object_get_string(obj, "owner");

                if (!title || !owner || !movies) {
                    printf("ERROR: Lipsește unul dintre câmpuri.\n");
                } else {
                    printf("SUCCESS: Detalii colectie\n");
                    printf("title: %s\n", title);
                    printf("owner: %s\n", owner);

                    for (size_t i = 0; i < json_array_get_count(movies); i++) {
                        JSON_Object *movie = json_array_get_object(movies, i);
                        const char *movie_title = json_object_get_string(movie, "title");
                        int movie_id = json_object_get_number(movie, "id");
                        
                        printf("#%d: %s\n", movie_id, movie_title);
                    }
                }
            } else {
                printf("ERROR: Nu s-a putut parsa JSON-ul colecției.\n");
            }
        } else {
            printf("ERROR: Răspuns invalid de la server.\n");
        }
    }

    free(message);
    free(response);
    close_connection(sockfd);
}

void delete_collection() {
    if (user_cookie == NULL || jwt_token == NULL) {
        printf("ERROR: Trebuie să fii logat și să ai token JWT.\n");
        return;
    }

    int id;
    printf("id=");
    fflush(stdout);
    scanf("%d", &id);

    char jwt_header[BUFLEN];
    snprintf(jwt_header, BUFLEN, "Authorization: Bearer %s", jwt_token);
    char *headers[] = { jwt_header };

    char *cookies[] = { user_cookie };

    char endpoint[BUFLEN];
    snprintf(endpoint, BUFLEN, "/api/v1/tema/library/collections/%d", id);

    char *message = compute_delete_request(
        HOST_HEADER,
        endpoint,
        NULL,
        cookies,
        1,
        headers,
        1
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "error") || strstr(response, "ERROR") || strstr(response, "404")) {
        printf("ERROR: %s\n", response);
    } else {
        printf("SUCCESS: Colectie stearsa\n");
    }

    free(message);
    free(response);
    close_connection(sockfd);
}

void add_movie_to_collection() {
    if (user_cookie == NULL || jwt_token == NULL) {
        printf("ERROR: Trebuie să fii logat și să ai token JWT.\n");
        return;
    }

    int collection_id, movie_id;
    printf("collection_id=");
    fflush(stdout);
    scanf("%d%*c", &collection_id);

    printf("movie_id=");
    fflush(stdout);
    scanf("%d%*c", &movie_id);

    char jwt_header[BUFLEN];
    snprintf(jwt_header, BUFLEN, "Authorization: Bearer %s", jwt_token);
    char **headers = malloc(sizeof(char *));
    headers[0] = jwt_header;

    char **cookies = malloc(sizeof(char *));
    cookies[0] = user_cookie;

    JSON_Value *val = json_value_init_object();
    JSON_Object *obj = json_value_get_object(val);

    json_object_set_number(obj, "id", movie_id);

    char **data = malloc(sizeof(char *));
    data[0] = json_serialize_to_string(val);

    char endpoint[BUFLEN];
    snprintf(endpoint, BUFLEN, "/api/v1/tema/library/collections/%d/movies", collection_id);

    char *message = compute_post_request(
        HOST_HEADER,
        endpoint,
        "application/json",
        data,
        1,
        cookies,
        1,
        headers,
        1
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "error") || strstr(response, "ERROR") || strstr(response, "400") || strstr(response, "404")) {
        printf("ERROR: %s\n", response);
    } else {
        printf("SUCCESS: Film adaugat in colectie\n");
    }

    json_value_free(val);
    free(message);
    free(response);
    close_connection(sockfd);
}

void delete_movie_from_collection() {
    if (user_cookie == NULL || jwt_token == NULL) {
        printf("ERROR: Trebuie să fii logat și să ai token JWT.\n");
        return;
    }

    int collection_id, movie_id;
    printf("collection_id=");
    fflush(stdout);
    scanf("%d%*c", &collection_id);

    printf("movie_id=");
    fflush(stdout);
    scanf("%d%*c", &movie_id);

    char jwt_header[BUFLEN];
    snprintf(jwt_header, BUFLEN, "Authorization: Bearer %s", jwt_token);
    char *headers[] = { jwt_header };

    char *cookies[] = { user_cookie };

    char endpoint[BUFLEN];
    snprintf(endpoint, BUFLEN,
             "/api/v1/tema/library/collections/%d/movies/%d",
             collection_id, movie_id);

    char *message = compute_delete_request(
        HOST_HEADER,
        endpoint,
        NULL,
        cookies,
        1,
        headers,
        1
    );

    int sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    if (strstr(response, "error") || strstr(response, "ERROR") ||
        strstr(response, "404") || strstr(response, "400")) {
        printf("ERROR: %s\n", response);
    } else {
        printf("SUCCESS: Film sters din colectie\n");
    }

    free(message);
    free(response);
    close_connection(sockfd);
}

int main() {
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    char command[BUFLEN];

    while (1) {
        fgets(command, BUFLEN, stdin);
        command[strcspn(command, "\n")] = 0;
        fflush(stdout);

        if (strcmp(command, "exit") == 0) break;
        else if (strcmp(command, "login_admin") == 0) login_admin();
        else if (strcmp(command, "add_user") == 0) add_user();
        else if (strcmp(command, "get_users") == 0) get_users();
        else if (strcmp(command, "delete_user") == 0) delete_user();
        else if (strcmp(command, "logout_admin") == 0) logout_admin();
        else if (strcmp(command, "login") == 0) login();
        else if (strcmp(command, "get_access") == 0) get_access();
        else if (strcmp(command, "get_movies") == 0) get_movies();
        else if (strcmp(command, "add_movie") == 0) add_movie();
        else if (strcmp(command, "get_movie") == 0) get_movie();
        else if (strcmp(command, "update_movie") == 0) update_movie();
        else if (strcmp(command, "delete_movie") == 0) delete_movie();
        else if (strcmp(command, "add_collection") == 0) add_collection();
        else if (strcmp(command, "get_collections") == 0) get_collections();
        else if (strcmp(command, "get_collection") == 0) get_collection();
        else if (strcmp(command, "delete_collection") == 0) delete_collection();
        else if (strcmp(command, "add_movie_to_collection") == 0) add_movie_to_collection();
        else if (strcmp(command, "delete_movie_from_collection") == 0) delete_movie_from_collection();
    }

    if (admin_cookie) free(admin_cookie);
    if (jwt_token) free(jwt_token);
    return 0;
}

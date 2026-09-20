#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "helper.h"
#include "requests.h"

char *compute_get_request(char *host, char *url, char *query_params,
                          char **cookies, int cookies_count, 
                          char **headers, int headers_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Line: GET /url?params HTTP/1.1
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }
    compute_message(message, line);

    // Host header
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Custom headers (‼️ Asta lipsea!)
    if (headers != NULL && headers_count > 0) {
        for (int i = 0; i < headers_count; i++) {
            compute_message(message, headers[i]);
        }
    }

    // Cookies (optional)
    if (cookies != NULL && cookies_count > 0) {
        memset(line, 0, LINELEN);
        strcat(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i != cookies_count - 1) {
                strcat(line, "; ");
            }
        }
        compute_message(message, line);
    }

    // Final empty line
    compute_message(message, "");

    free(line);
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type,
                           char **body_data, int body_data_fields_count,
                           char **cookies, int cookies_count,
                           char **headers, int headers_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(BUFLEN, sizeof(char));

    // Step 1: POST line
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    // Step 2: Host header
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Step 3: Content-Type header
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    // Step 4: Body content
    body_data_buffer[0] = '\0';
    for (int i = 0; i < body_data_fields_count; i++) {
        strcat(body_data_buffer, body_data[i]);
    }

    // Step 5: Content-Length header
    sprintf(line, "Content-Length: %ld", strlen(body_data_buffer));
    compute_message(message, line);

    // Step 6: Cookies (optional)
    if (cookies != NULL && cookies_count > 0) {
        memset(line, 0, LINELEN);
        strcat(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i != cookies_count - 1) {
                strcat(line, "; ");
            }
        }
        compute_message(message, line);
    }

    // Step 7: Custom headers (ex: Authorization)
    if (headers != NULL && headers_count > 0) {
        for (int i = 0; i < headers_count; i++) {
            compute_message(message, headers[i]);
        }
    }

    // Step 8: End of headers
    compute_message(message, "");

    // Step 9: Body data
    compute_message(message, body_data_buffer);

    free(line);
    free(body_data_buffer);
    return message;
}

char *compute_delete_request(char *host, char *url, char *query_params,
                             char **cookies, int cookies_count,
                             char **headers, int headers_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // DELETE line
    if (query_params != NULL) {
        sprintf(line, "DELETE %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "DELETE %s HTTP/1.1", url);
    }
    compute_message(message, line);

    // Host header
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Custom headers (ex: Authorization: Bearer ...)
    if (headers != NULL && headers_count > 0) {
        for (int i = 0; i < headers_count; i++) {
            compute_message(message, headers[i]);
        }
    }

    // Cookies
    if (cookies != NULL && cookies_count > 0) {
        memset(line, 0, LINELEN);
        strcat(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i != cookies_count - 1) {
                strcat(line, "; ");
            }
        }
        compute_message(message, line);
    }

    // Final newline
    compute_message(message, "");

    free(line);
    return message;
}

char *compute_put_request(const char *host, const char *url,
                          const char *content_type, char **body_data,
                          int body_data_fields_count,
                          char **cookies, int cookies_count,
                          char **headers, int headers_count) {
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(BUFLEN, sizeof(char));

    // Start line
    sprintf(line, "PUT %s HTTP/1.1", url);
    compute_message(message, line);

    // Host header
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    // Other headers
    if (headers != NULL)
        for (int i = 0; i < headers_count; i++)
            compute_message(message, headers[i]);

    // Content-Type header
    if (content_type != NULL) {
        sprintf(line, "Content-Type: %s", content_type);
        compute_message(message, line);
    }

    // Build body data
    if (body_data != NULL) {
        for (int i = 0; i < body_data_fields_count; i++) {
            strcat(body_data_buffer, body_data[i]);
        }
    }

    // Content-Length header
    sprintf(line, "Content-Length: %ld", strlen(body_data_buffer));
    compute_message(message, line);

    // Cookies
    if (cookies != NULL) {
        strcpy(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++) {
            strcat(line, cookies[i]);
            if (i < cookies_count - 1) {
                strcat(line, "; ");
            }
        }
        compute_message(message, line);
    }

    // End headers
    compute_message(message, "");

    // Body data
    if (body_data != NULL) {
        strcat(message, body_data_buffer);
    }

    free(line);
    free(body_data_buffer);
    return message;
}



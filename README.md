# REST-API-Client

A command-line **HTTP/1.1 REST API client implemented in C**, built directly on top of TCP sockets and designed to interact with a remote movie-library service.

The application manually constructs HTTP requests, sends them over TCP connections, receives complete HTTP responses, parses JSON payloads, and manages authentication state through both session cookies and JWT bearer tokens.

The client supports two different roles. Administrators can authenticate and manage users, while regular users can authenticate, obtain library access, manage movies, and organize movies into collections.

Instead of using a high-level HTTP library, the project implements the main protocol logic directly: request lines, HTTP headers, cookies, authorization headers, content lengths, JSON request bodies, socket connections, response buffering, and parsing of server responses.

## HTTP Client Architecture

The project is organized into several modules with different responsibilities.

`client.c` contains the command-line interface and the higher-level application logic.

It handles authentication, authorization state, user input, movie operations, collection operations, and interpretation of server responses.

`requests.c` contains the HTTP request builders.

It constructs requests for:

- GET
- POST
- PUT
- DELETE

These requests include the HTTP method, endpoint, host header, cookies, custom headers, content type, content length, and optional request body.

`helper.c` handles the lower-level TCP communication.

It opens socket connections, sends complete messages, receives HTTP responses, and determines when the full response body has been received.

JSON serialization and parsing are handled using the included **Parson** library.

This modular structure separates the application-level commands from HTTP serialization and socket-level communication.

## Manual HTTP/1.1 Requests

One of the central parts of the project is the manual construction of HTTP requests.

A request is built as a textual message containing the request line followed by the necessary headers.

For example, a GET request begins with the selected endpoint and the HTTP version.

The `Host` header is then included, followed by optional authorization headers and cookies.

POST and PUT requests additionally include:

- `Content-Type`
- `Content-Length`
- a JSON request body

DELETE requests can also include authentication information when required.

This means that the application does not delegate protocol generation to an external HTTP client library.

Instead, the request structure is explicitly created according to the requirements of HTTP/1.1.

## TCP Communication

Communication with the REST server is performed using POSIX TCP sockets.

For each request, the application opens a connection to the configured server using:

- `socket`
- `connect`

The completed HTTP request is then written to the socket.

`send_to_server` ensures that the entire request is transmitted even if a single `write` operation sends only part of the buffer.

After the response has been received, the TCP connection is closed.

This provides direct experience with the relationship between an application-layer protocol such as HTTP and the underlying TCP transport connection.

## Complete HTTP Response Reception

TCP is a stream protocol, so a complete HTTP response is not guaranteed to arrive in one `read` call.

The project therefore implements response buffering rather than assuming that one socket read contains the complete message.

Incoming bytes are accumulated inside a dynamic buffer.

The implementation first searches for the HTTP header terminator:

`\r\n\r\n`

Once the complete header has arrived, it searches for the `Content-Length` field.

The value of `Content-Length` determines how many bytes of response body are expected.

The client continues reading from the socket until the buffer contains both the complete header and the declared response body.

This avoids incorrectly processing partial HTTP responses and demonstrates one of the important details of building an HTTP client directly over TCP.

## JSON Processing

The REST API exchanges structured data using JSON.

The project uses the included **Parson** library for both serialization and parsing.

When sending credentials, movie information, collection information, or movie identifiers, the client creates JSON objects and serializes them into strings.

These strings are then used as HTTP request bodies with:

`Content-Type: application/json`

When the server responds with JSON, the application separates the HTTP body from the headers and parses the resulting JSON representation.

This allows fields such as identifiers, titles, usernames, tokens, owners, movies, and collections to be extracted from the response.

The combination of manual HTTP processing and a dedicated JSON parser keeps the networking layer explicit while still providing structured data handling.

## Administrator Authentication

The application supports a dedicated administrator authentication flow.

The `login_admin` command asks for:

- username
- password

These credentials are encoded into a JSON object and sent through a POST request to:

`/api/v1/tema/admin/login`

When authentication succeeds, the server returns a `Set-Cookie` header.

The client extracts the session cookie from the HTTP response and stores it in memory.

This cookie is then attached to subsequent administrator requests.

The administrator session remains active until the corresponding logout command succeeds or the program exits.

## Administrator Session Cookies

HTTP cookies are used to maintain administrator session state.

After a successful administrator login, the client searches the raw HTTP response for the `Set-Cookie` header.

The relevant cookie value is copied into dynamically allocated memory and stored in `admin_cookie`.

Later administrator operations create a `Cookie` header containing this value.

The project therefore implements session management directly at the HTTP level instead of relying on an HTTP library's cookie manager.

The `logout_admin` command sends an authenticated request to the administrator logout endpoint and clears the locally stored cookie when the operation succeeds.

## User Management

Once authenticated as an administrator, the client can manage regular users.

The supported operations include:

- adding a user
- listing users
- deleting a user

`add_user` sends a POST request containing a username and password encoded as JSON.

`get_users` sends a GET request and parses the returned JSON list.

The response is used to display information associated with each returned user.

`delete_user` creates an endpoint containing the selected username and sends a DELETE request.

All of these operations require the stored administrator session cookie.

The client checks for authentication state before attempting the protected operations.

## Regular User Authentication

Regular users have a separate authentication flow.

The `login` command asks for:

- administrator username
- user username
- user password

These values are encoded into JSON and sent to the user login endpoint.

When authentication succeeds, another session cookie is extracted from the server's `Set-Cookie` header and stored as `user_cookie`.

This cookie represents the authenticated user session.

However, accessing the library functionality requires an additional authorization step.

## JWT Access Token

After user login, the client can execute the `get_access` command.

This sends an authenticated GET request to:

`/api/v1/tema/library/access`

The request includes the user session cookie.

The server response contains a JSON object with a JWT access token.

The client parses the JSON body, extracts the `token` field, and stores it in `jwt_token`.

Protected library requests then construct an HTTP header of the form:

`Authorization: Bearer <token>`

The project therefore uses two different authentication mechanisms together:

- a session cookie for authenticated user state
- a JWT bearer token for library authorization

The client explicitly manages both values and attaches them to the requests that require them.

## Movie Management

Authenticated users with a valid JWT token can interact with the movie library.

The project implements the complete set of movie operations:

- list movies
- retrieve a movie
- add a movie
- update a movie
- delete a movie

`get_movies` sends a GET request to the movie endpoint and parses the returned JSON array.

Each returned movie can then be displayed using its identifier and title.

`get_movie` targets an endpoint containing a specific movie ID and displays the details returned by the server.

## Adding Movies

The `add_movie` command creates a new movie.

The client reads the required fields from standard input, including information such as:

- title
- year
- description
- rating

A JSON object is created from the supplied values.

The object is serialized and sent inside a POST request.

The request includes both the session cookie and the JWT authorization header.

This operation combines most of the important components of the project:

- interactive CLI input
- JSON serialization
- HTTP request construction
- cookies
- JWT authentication
- TCP communication
- HTTP response handling

## Updating and Deleting Movies

Existing movies can also be modified.

`update_movie` sends a PUT request to an endpoint containing the movie ID.

The new movie fields are encoded in a JSON request body and transmitted with the appropriate authentication data.

`delete_movie` sends an authenticated DELETE request for the selected movie ID.

The project therefore implements all four main REST-style CRUD operations:

- Create through POST
- Read through GET
- Update through PUT
- Delete through DELETE

These methods are constructed manually in `requests.c`.

## Movie Collections

The application also supports collections containing multiple movies.

Collections have their own endpoints and authorization requirements.

Users can:

- create collections
- list collections
- inspect one collection
- delete collections
- add movies to collections
- remove movies from collections

This extends the client beyond simple single-resource CRUD operations and introduces relationships between REST resources.

## Creating Collections

The `add_collection` operation creates a collection from a title and a list of movie identifiers.

The client first sends a POST request that creates the collection itself.

After the collection exists, additional POST requests are used to associate the requested movies with it.

This requires multiple dependent HTTP operations.

The application must obtain the result of the initial collection creation and then use the corresponding collection identifier in subsequent endpoints.

The implementation also handles failures that may occur while individual movies are being added.

## Retrieving Collections

`get_collections` requests the collections associated with the authenticated user.

The JSON response is parsed and used to display the available collection identifiers and titles.

`get_collection` targets one collection by ID.

Its response contains additional information including:

- collection title
- owner
- list of movies

The application iterates through the JSON movie array and prints the identifier and title of each included movie.

This demonstrates parsing nested structured JSON responses rather than only simple single-value objects.

## Modifying Collection Contents

Movies can be associated with an existing collection using `add_movie_to_collection`.

The client creates a JSON object containing the movie identifier and sends it through an authenticated POST request to the collection's movie endpoint.

Movies can also be removed through `delete_movie_from_collection`.

In this case, both the collection ID and movie ID become part of the URL used by the DELETE request.

This follows the resource-oriented structure of the REST API and provides practice constructing dynamic endpoints from application data.

## Session and Authorization State

The client maintains three important pieces of authentication state:

- `admin_cookie`
- `user_cookie`
- `jwt_token`

These values determine which operations can be executed.

Administrator functions check for a valid administrator cookie.

User library functions require both a user cookie and JWT token.

When the required credentials are missing, the client reports the problem without sending an invalid request.

The state exists for the lifetime of the CLI process and is reused across commands.

This makes the application behave like a persistent interactive HTTP client even though individual REST operations open their own TCP connections.

## HTTP Headers

The request-building functions support several different categories of headers.

The `Host` header is added to all requests.

Requests containing JSON bodies include:

`Content-Type: application/json`

and an appropriate `Content-Length`.

Cookies are combined into a standard:

`Cookie: ...`

header.

JWT-authenticated requests add:

`Authorization: Bearer ...`

The request helpers also accept arbitrary custom headers, which allows authentication and other protocol metadata to remain separate from the generic request-building logic.

## GET Requests

`compute_get_request` constructs HTTP GET messages.

It supports optional query parameters, cookies, and custom headers.

The request builder creates the request line, host header, additional headers, cookie header, and the final empty line separating HTTP headers from the optional body area.

GET is used by several operations including retrieving users, obtaining library access, listing movies, retrieving movie details, and reading collections.

## POST Requests

`compute_post_request` builds POST messages containing request bodies.

It creates the request line and adds:

- Host
- Content-Type
- Content-Length
- cookies when provided
- additional headers when provided

The JSON body is appended after the empty line terminating the headers.

POST is used for operations such as authentication, user creation, movie creation, collection creation, and adding a movie to a collection.

## PUT Requests

The client implements PUT requests through `compute_put_request`.

The structure is similar to POST.

The method and URL are written first, followed by authorization headers, content type, content length, cookies, and the serialized body.

PUT is used to update existing movie information.

Providing a dedicated request builder keeps method-specific HTTP generation outside the higher-level command functions.

## DELETE Requests

`compute_delete_request` constructs DELETE requests.

It supports query parameters, authentication headers, and cookies.

DELETE requests are used to remove users, movies, collections, and movies from collections.

Together, the four request builders provide the client with the HTTP methods necessary for the REST operations supported by the remote service.

## Interactive CLI

The application runs as an interactive command-line program.

The main loop continuously reads command names from standard input.

Supported commands include:

- `login_admin`
- `add_user`
- `get_users`
- `delete_user`
- `logout_admin`
- `login`
- `get_access`
- `get_movies`
- `get_movie`
- `add_movie`
- `update_movie`
- `delete_movie`
- `add_collection`
- `get_collections`
- `get_collection`
- `delete_collection`
- `add_movie_to_collection`
- `delete_movie_from_collection`
- `exit`

Each command is mapped to a dedicated function.

This keeps the main command-dispatch loop relatively simple while the individual functions contain the logic for each REST operation.

## Error Handling

The client checks both local application state and server responses.

Operations requiring authentication verify that the expected cookie or token is available before sending a request.

Responses are inspected for error indications and HTTP status information relevant to the requested operation.

The implementation handles cases such as unauthorized requests, invalid resources, missing fields, and unsuccessful authentication.

When a valid JSON body is expected, the client also verifies that parsing succeeds before attempting to access the corresponding object or array fields.

## Dynamic Response Buffer

The socket helper uses a reusable dynamic buffer implementation from `buffer.c`.

Bytes received from the server are appended as they arrive.

Utility operations search the buffer for protocol markers such as the end of the HTTP header and the `Content-Length` field.

Once the expected message size has been reached, a string terminator is appended and the complete response is returned to the higher-level client code.

This is necessary because HTTP responses can be larger than the temporary socket-read buffer and may be split across several TCP reads.

## Project Structure

The repository contains:

- `client.c` — interactive CLI, authentication state, administrator operations, movie operations, collection operations, and response parsing
- `requests.c` / `requests.h` — manual HTTP GET, POST, PUT, and DELETE request builders
- `helper.c` / `helper.h` — TCP connections, complete request transmission, complete HTTP response reception, and HTTP message utilities
- `buffer.c` / `buffer.h` — dynamic response-buffer operations
- `parson.c` / `parson.h` — external JSON parsing and serialization library
- `Makefile` — compilation rules for the client
- `README.md` — original project documentation

The application logic and networking code are separated into small modules rather than being placed entirely inside one source file.

## Build and Run

The included `Makefile` builds the application using GCC.

Running:

`make`

compiles:

- `client.c`
- `helper.c`
- `requests.c`
- `parson.c`
- `buffer.c`

and links them into an executable named:

`client`

The application is then started with:

`./client`

Commands are entered interactively through standard input.

The remote server address, port, and HTTP host value used by the project are configured directly in `client.c`, so running the client requires access to the corresponding server environment.

The build artifacts can be removed using:

`make clean`

## Technologies and Concepts

- C
- Computer networks
- HTTP/1.1
- REST APIs
- REST clients
- TCP sockets
- POSIX sockets
- Manual HTTP request construction
- HTTP GET
- HTTP POST
- HTTP PUT
- HTTP DELETE
- HTTP headers
- Host headers
- Content-Type
- Content-Length
- Cookies
- Session authentication
- JWT
- Bearer authentication
- Authorization headers
- JSON
- Parson
- JSON serialization
- JSON parsing
- CRUD operations
- Resource-oriented APIs
- TCP stream processing
- Partial reads
- Partial writes
- Dynamic buffering
- Response parsing
- Command-line interfaces
- Authentication state
- Administrator operations
- User management
- Movie management
- Collection management
- Dynamic endpoints
- Error handling
- Dynamic memory allocation
- GCC
- Makefiles

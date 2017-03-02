# LISOD HTTP Server

## How it works

  This server is capable of handling pipelined HTTP 1.1 requests from multiple clients concurrently. The implementation contains two layers. 
  The first layer handles client connections. This entails listening for connecitons, multiplexint on events, and closing connections. This layer (and all others in the server) is single threaded. It achieves concurrency by multiplexing network events from clients. Each client is identified by a socket file descriptor. The multiplexing function `select` takes in a set of file descriptors to observe for events. The function call blocks until there is an event or multiple events on the observered sockets. Then the server accepts incoming connections or forwards the received data to another layer of the server that deals at a higher level of abstraction: HTTP. 
  The HTTP server takes the incoming data and first determines if the data constitutes a valid request. If the data is invalid the server responds with a 400 immediately. If the request is incomplete, the server caches the data and associates it with the socket file descriptor. On future requests the server checks for existing data with which to prepend the incoming data. In the case that the received data resembles a complete request the parser attempts to convert it to a Request struct. At this point the request can be serviced and responded to. Before returning the response to the sockets layer of the application, the native Response object is marshaled into a char array that can be sent to the client.
  In the process of servicing the request the server first checks the headers including HTTP version of the request to ensure the client is using 1.1. If this is the case the server is then interested in if the client wants to keep the connection open for subsequent requests (this is the only information the HTTP layer needs to communicate to the connection layer aside from response data and metadata). Next the server switches on the request's method to determine which resource to return to the client. At this point the server assembles a response to return to the connection layer.
  
## Bugs (failure to implement)

  1. partial requests: we stubbed logic that determines if the data received from the client is a valid request. it always answers in the affirmative, but this should be a function of the data in question.
  1. pipelining: We close the client ocnnection after returning data for a single request. We need to implement another function that allows the HTTP layer to indicate when the connection should be kept alive for subsequent requests.
  1. second requests: the server crashes on the second request!

## Vulnerabilities

  1. slow loris attacks: There is no timeout on a client connection. The client could open a connection and leave it open without sending a request. This would eventually exhaust our socket file descriptors.
  1. large request header fields: we do not dynamically size buffers to read the incoming header field data. It would be trivial to crash the server with lengthy header field value.
  1. large request bodies: ditto
  
  
  
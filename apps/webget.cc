#include "socket.hh"

#include <cstdlib>
#include <iostream>
#include <span>
#include <string>

void get_URL( const std::string& host, const std::string& path )
{
  TCPSocket sock;
  Address addr( host, "http" );

  sock.connect( addr );

  std::string request = "GET " + path + " HTTP/1.1\r\n" + 
  			"Host: " + host + "\r\n" + 
  			"Connection: close\r\n\r\n";
  			
  sock.write( request );

  std::string buffer;
  while ( !sock.eof() ) 
  {
    sock.read( buffer );
    std::cout << buffer;
  }

  sock.close();

  std::cerr << "Function called: get_URL(" << host << ", " << path << ")\n";
  std::cerr << "Warning: get_URL() has not been implemented yet.\n";
}

int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = std::span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      std::cerr << "Usage: " << args.front() << " HOST PATH\n";
      std::cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const std::string host { args[1] };
    const std::string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );

  }

  catch ( const std::exception& e ) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

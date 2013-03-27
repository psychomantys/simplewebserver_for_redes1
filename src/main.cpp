/*
 * =====================================================================================
 *
 *       Filename:  main.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/26/2013 01:19:51 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include	<iostream>
#include	<string>
#include	<signal.h>
#include	<boost/program_options.hpp>
#include	<boost/bind.hpp>
#include	<boost/function.hpp>

#include	<server.hpp>
#include	<config.h>

namespace po = boost::program_options;

using namespace std;

server_socket *s;
void cleanUp( int dummy ){
	s->shutdown(true);
}          

int main(int argc, char* argv[]){
	try{
		std::string host;
		uint32_t port;
		std::string doc_root;

		po::options_description desc("Allowed options");
		desc.add_options()
			("help", "This help message")
			("host,h", po::value<string>(&host)->default_value("0.0.0.0"),"address of web server")
			("port,p", po::value<uint32_t>(&port)->default_value(80),"port of web server")
			("doc-root,r", po::value<string>(&doc_root)->default_value("."),"Path of document root")
			("dumpversion","Print version")
			;

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if( vm.count("dumpversion") ){
			cout << SWSR1_VERSION_MAJOR  <<"."<< SWSR1_VERSION_MINOR <<"."<< SWSR1_VERSION_MINOR_FIX << "\n";
			return 0;
		}
		if( vm.count("help") ){
			cout << argv[0] <<"(" << SWSR1_VERSION_MAJOR  <<"."<< SWSR1_VERSION_MINOR <<"."<< SWSR1_VERSION_MINOR_FIX << "\n";
			cout << desc << "\n";
			return 1;
		}

		server_socket server(host,port,20,doc_root);

		s=&server;

		signal( SIGTERM, cleanUp );
		signal( SIGINT, cleanUp );
		signal( SIGQUIT, cleanUp );
		signal( SIGHUP, cleanUp );
		signal( SIGKILL, cleanUp );

		server();

	}catch( std::exception &e ){
		std::cerr << "exception: " << e.what() << "\n";
	}
	return 0;
}


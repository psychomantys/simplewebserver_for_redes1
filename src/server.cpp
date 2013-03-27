/*
 * =====================================================================================
 *
 *       Filename:  server.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/26/2013 03:08:07 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include	<server.hpp>
#include	<reply.hpp>
#include	<mime_types.hpp>

#include	<iostream>
#include	<sstream>
#include	<fstream>


#include	<string.h>         /* bzero */
#include	<sys/wait.h>       /* waitpid */
#include	<unistd.h>         /* exit, fork */
#include	<signal.h>         /* signal */
#include	<time.h>           /* TIME, time_t */
#include	<pthread.h>        /* pthread_t, pthread_create */
#include	<sys/stat.h>       /* lstat() */
#include	<sys/types.h>      /* mode_t */
#include	<stdlib.h>
#include	<pthread.h>

using http::server::reply;
//using http::server::status_strings;

/*define variaveis constantes e seus respectivos valores */
#define BUFFSIZE 2000

server_socket::server_socket( std::string &ip, uint32_t &port, const int &max_requests, const std::string& doc_root){
//	this->ip( htonl( atoi( ip.c_str() )) );
//	this->port( htons(port) );
	this->shutdown(false);
	this->ip( atoi( ip.c_str() ) );
	this->port( port );
	this->doc_root(doc_root);

	this->MAX_NUM_REQUESTS(max_requests);

	cria_socket();
}

void server_socket::cria_socket(){
	listen_fd( socket(AF_INET, SOCK_STREAM, 0) );
	if( listen_fd()<0 ){
		throw std::runtime_error("socket error");
	}

	/*populando os dados do servidor*/
	bzero(&serv_addr, sizeof(serv_addr) ); /*zera a estrutura que armazenarah os dados do servidor */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl( ip() ); /* Aceita qualquer faixa de IP que a maquina possa responder. */
	serv_addr.sin_port = htons( this->port() ); /* define a porta */

	/* vincula um socket a um endereco */
	if( bind( listen_fd(),(struct sockaddr*)(&serv_addr), sizeof(serv_addr) ) < 0){
		throw std::runtime_error("Falha ao observar o socket do servidor");
	}

	/* Estipula a fila para o Servidor */
	if( listen(listen_fd(), MAX_NUM_REQUESTS() )<0 )
		throw std::runtime_error("Falha ao tentar escutar o socket do servidor");

}

bool url_decode(const std::string& in, std::string& out){
	out.clear();
	out.reserve(in.size());
	for (std::size_t i = 0; i < in.size(); ++i){
		if (in[i] == '%'){
			if (i + 3 <= in.size()){
				int value = 0;
				std::istringstream is(in.substr(i + 1, 2));
				if (is >> std::hex >> value){
					out += static_cast<char>(value);
					i += 2;
				}else{
					return false;
				}
			}else{
				return false;
			}
		}else{
			if (in[i] == '+'){
				out+=' ';
			}else{
				out+=in[i];
			}
		}
	}
	return true;
}

void server_socket::exec(int connfd){
	char buffer[BUFFSIZE];
	int n;

	/*Rebendo Protocolo http do cliente*/
	if( (n=recv(connfd, buffer, BUFFSIZE,0))<0 ){
		std::cerr << "Falhou ao receber os dados iniciais do cliente"<<n<<"\n" ;
		return;
	}

	std::stringstream client_in;
	std::stringstream client_out;
	client_in.write(buffer, n);

	std::string method, uri, HTTP_version;

	client_in >> method >> uri >> HTTP_version;

	// Decode url to path.
	std::string request_path;

	if( !url_decode(uri, request_path) ){
		reply::stock_reply(reply::bad_request, client_out);
	}else if( request_path.empty() || request_path[0] != '/'
	// Request path must be absolute and not contain "..".
		|| request_path.find("..") != std::string::npos
	){
		reply::stock_reply(reply::bad_request, client_out);
	}else{
	
		if( request_path[request_path.size()-1] == '/' ){
		// If path ends in slash (i.e. is a directory) then add "index.html".
			request_path += "index.html";
		}

		// Open the file to send back.
		std::string full_path = doc_root() + request_path;
		std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
		if( !is ){
			reply::stock_reply(reply::not_found, client_out);
		}else{
			try{
				std::size_t last_slash_pos = request_path.find_last_of("/");
				std::size_t last_dot_pos = request_path.find_last_of(".");
				std::string extension;
				if( last_dot_pos!=std::string::npos && last_dot_pos>last_slash_pos ){
					extension=request_path.substr(last_dot_pos + 1);
				}
					reply::stock_reply(reply::ok, client_out, extension);
				{
					char aux[1000];
					int clen;
					client_out.read(aux, sizeof(aux) );
					clen=client_out.gcount();
					while( clen!=0 ){
						send(connfd, aux, clen, 0);
						client_out.read(aux, sizeof(aux) );
						clen=client_out.gcount();
					}
					is.read(aux, sizeof(aux) );
					clen=is.gcount();
					while( clen!=0 ){
						send(connfd, aux, clen, 0);
						is.read(aux, sizeof(aux) );
						clen=is.gcount();
					}
				}
			}catch( std::exception &e ){
				std::cout << e.what() << std::endl ;
			}catch(...){
				std::clog << "Connection handle worst exception\n" ;
			}
		}
	}

	{
		char aux[1000];
		int clen;
		client_out.read(aux, sizeof(aux) );
		clen=client_out.gcount();
		while( clen!=0 ){
			send(connfd, aux, clen, 0);
			client_out.read(aux, sizeof(aux) );
			clen=client_out.gcount();
		}
	}
	return;
}

static void *execucao_thread(void *arg){
	server_socket *s=(server_socket*)(arg);

	struct sockaddr_in client; /* define um socket para o cliente */
	socklen_t clientlen;

	pthread_detach(pthread_self());

	while( not s->shutdown() ){
		int connfd=accept(s->listen_fd(), (struct sockaddr *)(&client), &clientlen); /* iptr aceita a escuta do cliente */
		
		s->exec(connfd);
		close(connfd);
	}
	return NULL;
}

void server_socket::operator()(){
	struct sockaddr_in client; /* define um socket para o cliente */
	pthread_t tid[3];

//	int lfd=listen_fd();

	for( int i=0 ; i<sizeof(tid) ; ++i ){
//		pthread_create(&tid[i], NULL, &execucao_thread, &lfd );
		pthread_create(&tid[i], NULL, &execucao_thread, this );
	}
	for( int i=0 ; i<sizeof(tid) ; ++i ){
		pthread_join(tid[i],NULL);
	}
	return;
}


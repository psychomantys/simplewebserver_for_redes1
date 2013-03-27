#ifndef  server_INC
#define  server_INC

#include	<utils.hpp>

#include	<stdexcept>
#include	<stdint.h>
#include	<string>

#include	<sys/socket.h>     /* struct sockaddr, socket, listen, bind, accept, recv, send */
#include	<arpa/inet.h>      /* struct sockaddr */

using std::string;

class server_socket{
	private:
		declare_a(int,listen_fd);

		declare_a(uint32_t,ip);
		declare_a(uint32_t,port);

		declare_a(int,MAX_NUM_REQUESTS);
		declare_am(bool,shutdown);
		declare_a(string,doc_root);

		struct sockaddr_in serv_addr;

		void cria_socket();
	public:
//		server_socket( std::string &ip, uint32_t &port);
		server_socket( std::string &ip, uint32_t &port, const int &max_requests, const std::string& doc_root);
		void exec(int connfd);

		void operator()();
};

#endif   /* ----- #ifndef server_INC  ----- */


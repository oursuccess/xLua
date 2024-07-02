#pragma once
#include "uv.h"
#include "transporter.h"

namespace CPL
{
	class CPLDEBUGGER_API SocketServerTransporter : public Transporter {
	public:
		SocketServerTransporter();
		~SocketServerTransporter();
		void OnNewConnection(uv_stream_t* server, int status);
		bool Listen(const std::string& host, int port, std::string& err);
		void Send(const char* data, size_t len);
	private:
		virtual int Stop() override;
		virtual void Send(int cmd, const char* data, size_t len) override;
		virtual void OnDisconnect() override;

		uv_tcp_t uvServer;
		uv_stream_t* uvClient;
	};
}

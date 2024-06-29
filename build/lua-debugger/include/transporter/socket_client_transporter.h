#pragma once

#include "util/util.h"

#include <mutex>
#include <condition_variable>
#include "uv.h"
#include "transporter/transporter.h"

namespace CPL
{
	class SocketClientTransporter : public Transporter {
	public:
		SocketClientTransporter();
		~SocketClientTransporter();

		bool Connect(const std::string& host, int port, std::string& err);
		virtual int Stop() override;
		virtual void Send(int cmd, const char* data, size_t len) override;
		void OnConnection(uv_connect_t* req, int status);
	private:
		uv_tcp_t uvClient;
		uv_connect_t connect_req;
		std::mutex mutex;
		std::condition_variable cv;
		int connectionStatus;
	};
}
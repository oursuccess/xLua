#pragma once

#include "util/util.h"

#include <thread>
#include "uv.h"
#include "nlohmann/json_fwd.hpp"

namespace CPL
{
	class DebuggerFacade;

	enum class MessageCMD {
		Unknown,

		InitReq,	//request
		InitRsp,	//respond
		ReadyReq,
		ReadyRsp,

		AddBreakPointReq,
		AddBreakPointRsp,
		RemoveBreakPointReq,
		RemoveBreakPointRsp,

		ActionReq,
		ActionRsp,
		EvalReq,
		EvalRsp,

		BreakNotify,
		AttachedNotify,
		StartHookReq,
		StartHookRsq,

		LogNotify,
	};

	class CPLDEBUGGER_API Transporter {
	public:
		Transporter(bool server);
		virtual ~Transporter();
		virtual int Stop();
		bool IsConnected() const;
		bool IsServerMode() const;
		void Send(int cmd, const nlohmann::json doc);
		void OnAfterRead(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf);

	protected:
		virtual void Send(int cmd, const char* data, size_t len) = 0;
		void Send(uv_stream_t* handler, int cmd, const char* data, size_t len);
		void Send(uv_stream_t* handler, const char* data, size_t len);
		void Receive(const char* data, size_t len);
		void OnReceiveMessage(const nlohmann::json doc);
		void StartEventLoop();
		void Run();
		virtual void OnDisconnect();
		virtual void OnConnect(bool suc);
		static bool ParseSocketAddress(const std::string& host, int port, sockaddr_storage* addr, std::string& err);

		uv_loop_t* loop;

	private:
		std::thread thread;
		char* buf;
		size_t bufSize;
		size_t receiveSize;
		bool readHead;
		bool running;
		bool connected;
		bool serverMode;
	};
}

#include "transporter/socket_client_transporter.h"

namespace CPL
{
#pragma region Private Static
	void OnConnectCB(uv_connect_t* req, int status) {
		const auto transporter = (SocketClientTransporter*)req->data;
		transporter->OnConnection(req, status);
	}

	static void EchoAlloc(uv_handle_t* handle, size_t expectSize, uv_buf_t* buf) {
		buf->base = static_cast<char*>(malloc(expectSize));
		buf->len = expectSize;
	}

	static void AfterRead(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf) {
		auto transporter = static_cast<Transporter*>(handle->data);
		transporter->OnAfterRead(handle, nread, buf);
	}
#pragma endregion

	SocketClientTransporter::SocketClientTransporter() : Transporter(false), uvClient({}),
		connect_req({}), connectionStatus(0)
	{
	}

	SocketClientTransporter::~SocketClientTransporter()
	{
	}

	bool SocketClientTransporter::Connect(const std::string& host, int port, std::string& err)
	{
		uvClient.data = this;
		uv_tcp_init(loop, &uvClient);
		struct sockaddr_storage addr;
		bool addr_suc = ParseSocketAddress(host, port, &addr, err);
		if (!addr_suc) return false;

		connect_req.data = this;
		const int r = uv_tcp_connect(&connect_req, &uvClient, reinterpret_cast<const sockaddr*>(&addr), OnConnectCB);
		if (r) {
			err = uv_strerror(r);
			return false;
		}

		StartEventLoop();
		std::unique_lock<std::mutex> lock(mutex);
		cv.wait(lock);
		if (this->connectionStatus < 0) err = uv_strerror(this->connectionStatus);

		return IsConnected();
	}

	int SocketClientTransporter::Stop()
	{
		Transporter::Stop();
		if (IsConnected()) {
			uv_read_stop((uv_stream_t*)&uvClient);
			uv_close((uv_handle_t*)&uvClient, nullptr);
		}
		cv.notify_all();
		return 0;
	}

	void SocketClientTransporter::Send(int cmd, const char* data, size_t len)
	{
		Transporter::Send((uv_stream_t*)&uvClient, cmd, data, len);
	}

	void SocketClientTransporter::OnConnection(uv_connect_t* req, int status)
	{
		this->connectionStatus = status;
		if (status >= 0) {
			OnConnect(true);
			uv_read_start((uv_stream_t*)&uvClient, EchoAlloc, AfterRead);
		}
		else {
			Stop();
			OnConnect(false);
		}
		cv.notify_all();
	}
}
// tcp.cpp

#include "tools/boost_test/Common.h"

#include <framework/network/NetName.h>
#include <framework/logger/Logger.h>
#include <framework/logger/StreamRecord.h>
using namespace framework::configure;
using namespace framework::network;

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
using namespace boost::system;
using namespace boost::asio;
using namespace boost::asio::ip;

typedef unsigned long UInt32;

FRAMEWORK_LOGGER_DECLARE_MODULE_LEVEL("TestAsioTcp", framework::logger::Debug);

static inline void print_error(char const * title, error_code const & ec)
{
	LOG_ERROR(title << ": " << ec.message());
}

static inline void print_error(char const * title, system_error const & err)
{
	LOG_ERROR(title << ": " << err.what());
}

static void test_asio_tcp_client(
	io_service & io_svc, 
	NetName const & addr)
{
	std::string input = "test_asio_tcp";

	{
		LOG_INFO("client: input:" << input);
	}

	streambuf buf;
	{
		std::ostream os(&buf);
		//UInt32 plen = 0;
		//os.write((const char *)&plen, sizeof(plen));
		os << input;
	}
	try {
		tcp::socket fd(io_svc);
		fd.open(tcp::v4());
		fd.connect(tcp::endpoint(address::from_string(addr.host()), addr.port()));
		LOG_INFO("client: connect: ok");
		UInt32 plen = buf.size();
		fd.send(buffer(&plen, sizeof(UInt32)));
		LOG_INFO("client: send len: ok");
		fd.send(buf.data());
		LOG_INFO("client: send data: ok");
		buf.consume(buf.size());
		fd.receive(buffer(&plen, sizeof(UInt32)));
		LOG_INFO("client: receive len: ok");
		size_t len = fd.receive(buf.prepare(plen));
		LOG_INFO("client: receive data: ok");
		buf.commit(len);
	} catch (boost::system::system_error const & err) {
		LOG_ERROR("tcp_client: " << err.what());
		return;
	} catch (...) {
		LOG_ERROR("tcp_client: unknwon error");
		return;
	}

	std::string output;
	{
		std::istream is(&buf);
		is >> output;
	}

	{
		LOG_INFO("client: output:" << output);
	}
};

static void test_asio_tcp_server(
	io_service & io_svc, 
	NetName const & addr)
{
	try {
		tcp::acceptor fdl(io_svc);
		fdl.open(tcp::v4());
		fdl.bind(tcp::endpoint(address::from_string(addr.host()), addr.port()));
		LOG_INFO("server: bind: ok");
		tcp::socket fd(io_svc);
		fdl.listen();
		LOG_INFO("server: listen: ok");
		fdl.accept(fd);
		LOG_INFO("server: accept: ok");
		streambuf buf;
		UInt32 plen = 0;
		fd.receive(buffer(&plen, sizeof(UInt32)));
		LOG_INFO("server: receive len: ok");
		size_t len = fd.receive(buf.prepare(plen));
		LOG_INFO("server: receive data: ok");
		buf.commit(len);
		fd.send(buffer(&plen, sizeof(UInt32)));
		LOG_INFO("server: send len: ok");
		fd.send(buf.data());
		LOG_INFO("server: send data: ok");
		buf.consume(buf.size());
	} catch (boost::system::system_error const & err) {
		LOG_ERROR("tcp_server: " << err.what());
		return;
	} catch (...) {
		LOG_ERROR("tcp_server: unknwon error");
		return;
	}
}


struct test_asio_async_tcp_client_t
{
	test_asio_async_tcp_client_t(
		io_service & io_svc)
		: fd(io_svc)
	{
	}

	void start(
		NetName const & addr)
	{
		std::string input = "test_asio_tcp";
		LOG_ERROR("input:" << input);

		{
			UInt32 & plen = *buffer_cast<UInt32 *>(buf.prepare(sizeof(UInt32)));
			buf.commit(sizeof(UInt32));
			std::ostream os(&buf);
			os << input;
			plen = buf.size() - sizeof(UInt32);
		}

		error_code ec;
		fd.open(tcp::v4(), ec);
		if (ec) {
			handle_error(ec);
		} else {
			fd.async_connect(tcp::endpoint(address::from_string(addr.host()), addr.port()), 
				boost::bind(&test_asio_async_tcp_client_t::handle_connect, this, _1));
		}
	}

private:
	void handle_connect(
		error_code const & ec)
	{
		if (ec) {
			handle_error(ec);
		} else {
			LOG_INFO("client: connect: ok");
			fd.async_send(buf.data(), 
				boost::bind(&test_asio_async_tcp_client_t::handle_send, this, _1, _2));
		}
	}

	void handle_send(
		error_code const & ec, 
		size_t len)
	{
		if (ec) {
			handle_error(ec);
		} else {
			buf.consume(len);
			LOG_INFO("client: send: ok");
			fd.async_receive(buf.prepare(sizeof(UInt32)), 
				boost::bind(&test_asio_async_tcp_client_t::handle_receive_head, this, _1, _2));
		}
	}

	void handle_receive_head(
		error_code const & ec, 
		size_t len)
	{
		if (ec) {
			handle_error(ec);
		} else {
			LOG_INFO("client: receive len: ok");
			buf.commit(len);
			len = *buffer_cast<UInt32 const *>(buf.data());
			if (len + sizeof(UInt32) > buf.size()) {
				fd.async_receive(buf.prepare(len + sizeof(UInt32) - buf.size()), 
					boost::bind(&test_asio_async_tcp_client_t::handle_receive_body, this, _1, _2));
			} else {
				len = 0;
				handle_receive_body(ec, len);
			}
		}
	}

	void handle_receive_body(
		error_code const & ec, 
		size_t len)
	{
		if (ec) {
			handle_error(ec);
		} else {
			LOG_INFO("client: receive data: ok");
			buf.commit(len);
			buf.consume(sizeof(UInt32));
			std::string output;
			{
				std::istream is(&buf);
				is >> output;
			}
			LOG_INFO("output:" << output);
			delete this;
		}
	}

	void handle_error(
		error_code const & ec)
	{
		print_error("async_tcp_client", ec);
		delete this;
	}

private:
	tcp::socket fd;
	streambuf buf;
};

class test_asio_async_tcp_server_t
{
public:
	test_asio_async_tcp_server_t(
		io_service & io_svc)
		: fdl(io_svc)
		, fd(io_svc)
	{
	}

	void start( 
		NetName const & addr)
	{
		error_code ec;
		fdl.open(tcp::v4(), ec);
		if (!ec)
			fdl.bind(tcp::endpoint(address::from_string(addr.host()), addr.port()), ec);
		if (!ec) {
			LOG_INFO("server: bind: ok");
			fdl.listen(1, ec);
		}
		if (ec) {
			handle_error(ec);
		} else {
			LOG_INFO("server: listen: ok");
			fdl.async_accept(fd, 
				boost::bind(&test_asio_async_tcp_server_t::handle_accept, this, _1));
		}
	}

private:
	void handle_accept(
		error_code const & ec)
	{
		if (ec) {
			handle_error(ec);
		} else {
			LOG_INFO("server: accept: ok");
			fd.async_receive(buf.prepare(sizeof(UInt32)), 
				boost::bind(&test_asio_async_tcp_server_t::handle_receive_head, this, _1, _2));
		}
	}

	void handle_receive_head(
		error_code const & ec, 
		size_t len)
	{
		if (ec) {
			handle_error(ec);
		} else {
			LOG_INFO("server: receive len: ok");
			buf.commit(len);
			len = *buffer_cast<UInt32 const *>(buf.data());
			if (len + sizeof(UInt32) > buf.size()) {
				fd.async_receive(buf.prepare(len + sizeof(UInt32) - buf.size()), 
					boost::bind(&test_asio_async_tcp_server_t::handle_receive_body, this, _1, _2));
			} else {
				len = 0;
				handle_receive_body(ec, len);
			}
		}
	}

	void handle_receive_body(
		error_code const & ec, 
		size_t len)
	{
		if (ec) {
			handle_error(ec);
		} else {
			LOG_INFO("server: receive data: ok");
			buf.commit(len);
			fd.async_send(buf.data(), 
				boost::bind(&test_asio_async_tcp_server_t::handle_send, this, _1, _2));
		}
	}

	void handle_send(
		error_code const & ec, 
		size_t len)
	{
		if (ec) {
			handle_error(ec);
		} else {
			LOG_INFO("server: send: ok");
			buf.consume(len);
			delete this;
		}
	}

	void handle_error(
		error_code const & ec)
	{
		print_error("async_tcp_server", ec);
		delete this;
	}

private:
	tcp::acceptor fdl;
	tcp::socket fd;
	streambuf buf;
};


struct test_asio_async_tcp_client_1
{
	test_asio_async_tcp_client_1(
		io_service & io_svc, int i)
		: fd(io_svc)
		, ios_(io_svc)
		, i_(i)
		, is_running_(true)
	{
		timer = new boost::asio::deadline_timer(ios_);
	}

	void start(NetName const & addr)
	{        
		{
			UInt32 & plen = *buffer_cast<UInt32 *>(buf.prepare(sizeof(UInt32)));
			buf.commit(sizeof(UInt32));
			std::ostream os(&buf);
			os << "Hello,World";
			plen = buf.size() - sizeof(UInt32);
		}

		error_code ec;
		fd.open(tcp::v4(), ec);
		if (ec) {
			handle_error(ec);
		} else {
			fd.async_connect(tcp::endpoint(address::from_string(addr.host()), addr.port()), 
				boost::bind(&test_asio_async_tcp_client_1::handle_connect, this, _1));
		}
	}
	void stop(){is_running_ = false;}

private:
	void handle_connect(
		error_code const & ec)
	{
		if (!is_running_)
		{
			ios_.post(boost::bind(&test_asio_async_tcp_client_1::delete_this, this));
			return;
		}
		if (ec) {
			handle_error(ec);
		} else {
			fd.async_send(buf.data(), 
				boost::bind(&test_asio_async_tcp_client_1::handle_send, this, _1, _2));
		}
	}

	void handle_send(
		error_code const & ec, 
		size_t len)
	{
		if (!is_running_)
		{
			ios_.post(boost::bind(&test_asio_async_tcp_client_1::delete_this, this));
			return;
		}
		if (ec) {
			handle_error(ec);
		} else {
			buf.consume(len);
			fd.async_receive(buf.prepare(sizeof(UInt32)), 
				boost::bind(&test_asio_async_tcp_client_1::handle_receive_head, this, _1, _2));
		}
	}

	void handle_receive_head(
		error_code const & ec, 
		size_t len)
	{
		if (!is_running_)
		{
			ios_.post(boost::bind(&test_asio_async_tcp_client_1::delete_this, this));
			return;
		}
		if (ec) {
			handle_error(ec);
		} else {
			assert(len >= sizeof(UInt32));
			buf.commit(len);
			len = *buffer_cast<UInt32 const *>(buf.data());
			assert(len == 11);
			if (len + sizeof(UInt32) > buf.size()) {
				fd.async_receive(buf.prepare(len + sizeof(UInt32) - buf.size()), 
					boost::bind(&test_asio_async_tcp_client_1::handle_receive_body, this, _1, _2));
			} else {
				len = 0;
				handle_receive_body(ec, len);
			}
		}
	}

	void handle_receive_body(
		error_code const & ec, 
		size_t len)
	{
		if (!is_running_)
		{
			ios_.post(boost::bind(&test_asio_async_tcp_client_1::delete_this, this));
			return;
		}
		if (ec) {
			handle_error(ec);
		} else {
			buf.commit(len);
			len = *buffer_cast<UInt32 const *>(buf.data());
			assert(buf.size() == len + sizeof(UInt32));
			assert(len == 11);
			buf.consume(sizeof(UInt32));
			std::string output;
			{
				std::istream is(&buf);
				is >> output;
			}
			LOG_ERROR("output:" << output << " " << i_);
			assert(buf.size() == 0);
			timer->expires_from_now(boost::posix_time::seconds(1));
			timer->async_wait(boost::bind(&test_asio_async_tcp_client_1::on_timer, this, _1));

			//delete this;
		}
	}

	void on_timer(boost::system::error_code const & ec)
	{
		if (!is_running_)
		{
			ios_.post(boost::bind(&test_asio_async_tcp_client_1::delete_this, this));
			return;
		}
		if (ec)
			print_error("timer", ec);
		else
		{
			buf.prepare(sizeof(UInt32));
			buf.commit(sizeof(UInt32));
			std::ostream os(&buf);
			os << "Hello,World";
			UInt32 & plen = const_cast<UInt32 &>(*buffer_cast<UInt32 const *>(buf.data()));
			plen = buf.size() - sizeof(UInt32);
			assert(plen == 11);
			fd.async_send(buf.data(), 
				boost::bind(&test_asio_async_tcp_client_1::handle_send, this, _1, _2));
		}
	}

	void handle_error(
		error_code const & ec)
	{
		print_error("async_tcp_client", ec);
		//LOG_ERROR("async_tcp_client" << " " << ec.message());
		//delete this;
	}

	void delete_this()
	{
		fd.close();
		//delete this;
	}

private:
	tcp::socket fd;
	streambuf buf;
	boost::asio::deadline_timer *timer;
	io_service &ios_;
	int i_;
	bool is_running_;
};

class test_asio_async_tcp_server_1
{
public:
	test_asio_async_tcp_server_1(
		io_service & io_svc)
		: fdl(io_svc)
		, ios_(io_svc)
		, is_running_(true)
	{
		max_fd = 0;
	}

	void start(NetName const &addr)
	{
		error_code ec;
		fdl.open(tcp::v4(), ec);
		if (!ec)
			fdl.bind(tcp::endpoint(address::from_string(addr.host()), addr.port()), ec);
		if (!ec)
			fdl.listen(1, ec);
		if (ec) {
			handle_error(ec);
		} 
		else
		{
			ios_.post(boost::bind(&test_asio_async_tcp_server_1::start_accept, this));
		}
	}

	void stop(){is_running_ = false;}

private:
	void start_accept()
	{
		if (!is_running_)
		{
			return;
		}
		if (max_fd < 10)
		{
			fd[max_fd] = new tcp::socket(ios_);
			fdl.async_accept(*(fd[max_fd]), 
				boost::bind(&test_asio_async_tcp_server_1::handle_accept, this, _1, max_fd));
			max_fd++;
			ios_.post(boost::bind(&test_asio_async_tcp_server_1::start_accept, this));
		}
	}

	void handle_accept(
		error_code const & ec,
		int i)
	{
		if (!is_running_)
		{
			ios_.post(boost::bind(&test_asio_async_tcp_server_1::delete_this, this, i));
			return;
		}
		if (ec) {
			handle_error(ec);
		} else {
			fd[i]->async_receive(buf[i].prepare(sizeof(UInt32)), 
				boost::bind(&test_asio_async_tcp_server_1::handle_receive_head, this, _1, _2, i));
		}
	}

	void handle_receive_head(
		error_code const & ec, 
		size_t len,
		int i)
	{
		if (!is_running_)
		{
			ios_.post(boost::bind(&test_asio_async_tcp_server_1::delete_this, this, i));
			return;
		}
		if (ec) {
			handle_error(ec);
		} else {
			assert(len >= sizeof(UInt32));
			buf[i].commit(len);
			len = *buffer_cast<UInt32 const *>(buf[i].data());
			assert(len == 11);
			if (len + sizeof(UInt32) > buf[i].size()) {
				fd[i]->async_receive(buf[i].prepare(len + sizeof(UInt32) - buf[i].size()), 
					boost::bind(&test_asio_async_tcp_server_1::handle_receive_body, this, _1, _2, i));
			} else {
				len = 0;
				handle_receive_body(ec, len, i);
			}
		}
	}

	void handle_receive_body(
		error_code const & ec, 
		size_t len,
		int i)
	{
		if (!is_running_)
		{
			ios_.post(boost::bind(&test_asio_async_tcp_server_1::delete_this, this, i));
			return;
		}
		if (ec) {
			handle_error(ec);
		} else {
			buf[i].commit(len);
			len = *buffer_cast<UInt32 const *>(buf[i].data());
			assert(buf[i].size() == len + sizeof(UInt32));
			assert(len == 11);
			fd[i]->async_send(buf[i].data(), 
				boost::bind(&test_asio_async_tcp_server_1::handle_send, this, _1, _2, i));
		}
	}

	void handle_send(
		error_code const & ec, 
		size_t len,
		int i)
	{
		if (!is_running_)
		{
			ios_.post(boost::bind(&test_asio_async_tcp_server_1::delete_this, this, i));
			return;
		}
		if (ec) {
			handle_error(ec);
		} else {
			buf[i].consume(len);
			assert(buf[i].size() == 0);
			len = 0;
			fd[i]->async_receive(buf[i].prepare(sizeof(UInt32)), 
				boost::bind(&test_asio_async_tcp_server_1::handle_receive_head, this, _1, _2, i));
			//delete this;
		}
	}

	void handle_error(
		error_code const & ec)
	{
		print_error("async_tcp_server", ec);
		//LOG_ERROR("async_tcp_server" << " " << ec.message());
		//delete this;
	}

	void delete_this(int i)
	{
		fd[i]->close();
		//delete this;
	}
private:
	tcp::acceptor fdl;
	tcp::socket *fd[10];
	io_service &ios_;
	int max_fd;
	streambuf buf[10];
	bool is_running_;
};


test_asio_async_tcp_client_1 *clients[10];
test_asio_async_tcp_server_1 *server;
boost::asio::deadline_timer *timer;
void OnTimer2(boost::system::error_code const & ec);
void OnTimer1(boost::system::error_code const & ec);
static void test_asio_async_tcp_server(
	io_service & io_svc, 
	NetName const & addr)
{
	(new test_asio_async_tcp_server_t(io_svc))->start(addr);
}

static void test_asio_async_tcp_client(
	io_service & io_svc
	, NetName const & addr)
{
	for (int i = 0; i < 1; i++)
	{
		(new test_asio_async_tcp_client_t(io_svc))->start(addr);
	}
	//timer = new boost::asio::deadline_timer(io_svc);
	//timer->expires_from_now(boost::posix_time::seconds(10));
	//timer->async_wait(boost::bind(OnTimer1, _1));
}

void OnTimer1(boost::system::error_code const & ec)
{
	for (int i = 0; i < 5; i++)
	{
		clients[i]->stop();
	}
	timer->expires_from_now(boost::posix_time::seconds(10));
	timer->async_wait(boost::bind(OnTimer2, _1));
}

void OnTimer2(boost::system::error_code const & ec)
{
	server->stop();
}

static void test_asio_non_block_tcp_client(
	NetName const & addr)
{
	io_service io_svc;
	error_code ec;
	tcp::socket fd(io_svc);
	fd.open(tcp::v4(), ec);
	LOG_ERROR("open: " << ec.message());
	boost::asio::socket_base::non_blocking_io cmd(true);
	fd.io_control(cmd, ec);
	LOG_ERROR("io_control: " << ec.message());
	fd.connect(tcp::endpoint(address::from_string(addr.host()), addr.port()), ec);
	LOG_ERROR("connect: " << ec.message());
	while (ec == error::already_started || 
		ec == error::would_block || 
		ec == error::in_progress) {
			fd.connect(tcp::endpoint(address::from_string(addr.host()), addr.port()), ec);
			LOG_ERROR("connect: " << ec.message());
	}
}

void close_socket(
	boost::weak_ptr<void> token, 
	tcp::socket & fd)
{
	boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	if (!token.expired()) {
		LOG_ERROR("shutdown socket");
		error_code ec;
#ifndef WIN32   // linux 需要shutdown套接字才能取消阻塞操作
		fd.shutdown(boost::asio::socket_base::shutdown_both, ec);
		LOG_ERROR("shutdown socket: " << ec.message());
#else           // win32 shutdown套接字会发生错误
#endif
		fd.close(ec);
		LOG_ERROR("close socket: " << ec.message());
	}
}

void nop_delete(void *) {}

void test_asio_tcp_cancel(
	NetName const & addr)
{
	std::string input = "test_asio_tcp";
	{
		LOG_ERROR("input:" << input);
	}
	streambuf buf;
	{
		std::ostream os(&buf);
		UInt32 plen = 0;
		os.write((const char *)&plen, sizeof(plen));
		os << input;
	}
	boost::thread * th = NULL;
	try {
		io_service io_svc;
		error_code ec;
		tcp::socket fd(io_svc);
		fd.open(tcp::v4());
		boost::shared_ptr<void> token((void *)NULL, nop_delete);
		th = new boost::thread(boost::bind(close_socket, boost::weak_ptr<void>(token), boost::ref(fd)));
		LOG_ERROR("connect");
		fd.connect(tcp::endpoint(address::from_string(addr.host()), addr.port()));
		UInt32 plen = buf.size();
		LOG_ERROR("send");
		fd.send(buffer(&plen, sizeof(UInt32)));
		fd.send(buf.data());
		buf.consume(buf.size());
		LOG_ERROR("receive");
		fd.receive(buffer(&plen, sizeof(UInt32)));
		size_t len = fd.receive(buf.prepare(plen));
		buf.commit(len);
	} catch (boost::system::system_error const & err) {
		LOG_ERROR("tcp_client: " << err.what());
		return;
	}

	std::string output;
	{
		std::istream is(&buf);
		is >> output;
	}
	{
		LOG_ERROR("output:" << output);
	}
}

void test_asio_tcp(Config & conf)
{
	int mode = 12; // 1 - sync server, 2 - sync client
	// 4 - async server, 8 - async client
	NetName addr("127.0.0.1", 1111);

	conf.register_module("TestAsioTcp") << CONFIG_PARAM_RDWR(mode) << CONFIG_PARAM_RDWR(addr);

	if (mode & 3) {
		LOG_SECTION();
		LOG_DEBUG(("[tcp] start"));
		io_service io_svc;
		boost::thread * threads[2] = {NULL};
		if (mode & 1) {
			NetName addr("0.0.0.0", "1111+");
			threads[0] = new boost::thread(boost::bind(test_asio_tcp_server, boost::ref(io_svc), addr));
		}
		if (mode & 2) {
			threads[1] = new boost::thread(boost::bind(test_asio_tcp_client, boost::ref(io_svc), addr));
		}
		if (mode & 1) {
			threads[0]->join();
			delete threads[0];
		}
		if (mode & 2) {
			threads[1]->join();
			delete threads[1];
		}
		LOG_DEBUG(("[tcp] end"));
	}

	if (mode & 12) {
		LOG_SECTION();
		LOG_DEBUG(("[async_tcp] start"));
		io_service io_svc;
		NetName addr2("0.0.0.0", "1111+");
		if (mode & 4) {
			test_asio_async_tcp_server(io_svc, addr2);
		}
		if (mode & 8) {
			test_asio_async_tcp_client(io_svc, addr);
		}
		io_svc.run();
		LOG_DEBUG(("[async_tcp] end"));
	}

	if (mode & 48) {
		test_asio_non_block_tcp_client(addr);
	}

	if (mode & 192) {
		test_asio_tcp_cancel(addr);
	}
}

static TestRegister tr("asio_tcp", test_asio_tcp);

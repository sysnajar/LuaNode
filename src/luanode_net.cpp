#include "StdAfx.h"
#include "luanode.h"
#include "luanode_net.h"
#include "blogger.h"

#include <boost/bind.hpp>
#include "Network/shared_const_buffer.h"


// this is needed for tcp_keepalive definition
#ifdef _WIN32
#include <winsock2.h>
#endif

#ifdef _WIN32
#if defined(_MSC_VER)
#include "MSTCPiP.h"
#elif defined(__MINGW32__)
// afanado de mstcpip.h
/* Argument structure for SIO_KEEPALIVE_VALS */

struct tcp_keepalive {
	u_long  onoff;
	u_long  keepalivetime;
	u_long  keepaliveinterval;
};

// New WSAIoctl Options

#define SIO_RCVALL            _WSAIOW(IOC_VENDOR,1)
#define SIO_RCVALL_MCAST      _WSAIOW(IOC_VENDOR,2)
#define SIO_RCVALL_IGMPMCAST  _WSAIOW(IOC_VENDOR,3)
#define SIO_KEEPALIVE_VALS    _WSAIOW(IOC_VENDOR,4)
#define SIO_ABSORB_RTRALERT   _WSAIOW(IOC_VENDOR,5)
#define SIO_UCAST_IF          _WSAIOW(IOC_VENDOR,6)
#define SIO_LIMIT_BROADCASTS  _WSAIOW(IOC_VENDOR,7)
#define SIO_INDEX_BIND        _WSAIOW(IOC_VENDOR,8)
#define SIO_INDEX_MCASTIF     _WSAIOW(IOC_VENDOR,9)
#define SIO_INDEX_ADD_MCAST   _WSAIOW(IOC_VENDOR,10)
#define SIO_INDEX_DEL_MCAST   _WSAIOW(IOC_VENDOR,11)

// Values for use with SIO_RCVALL* options
#define RCVALL_OFF             0
#define RCVALL_ON              1
#define RCVALL_SOCKETLEVELONLY 2

#else
#error Unknown compiler.
#endif
#elif defined(__GNUC__)
#include <sys/socket.h>
#include <arpa/inet.h>
#endif



using namespace LuaNode::Net;

static int BoostErrorCodeToLua(lua_State* L, const boost::system::error_code& ec) {
	if(ec) {
		lua_pushboolean(L, false);
		lua_pushstring(L, ec.message().c_str());
		return 2;
	}
	lua_pushboolean(L, true);
	return 1;
}

void LuaNode::Net::RegisterFunctions(lua_State* L) {
	luaL_Reg methods[] = {
		{ "isIP", LuaNode::Net::IsIP },
		{ 0, 0 }
	};
	luaL_register(L, "Net", methods);
	lua_pop(L, 1);
}

//////////////////////////////////////////////////////////////////////////
/// 
/*static*/ int LuaNode::Net::IsIP(lua_State* L) {
	std::string address = luaL_checkstring(L, 1);

	boost::system::error_code ec;
	
	boost::asio::ip::address_v4::from_string(address, ec);
	if(!ec) {
		lua_pushboolean(L, true);
		lua_pushinteger(L, 4);
	}
	else {
		boost::asio::ip::address_v6::from_string(address, ec);
		if(!ec) {
			lua_pushboolean(L, true);
			lua_pushinteger(L, 6);
		}
		else {
			lua_pushboolean(L, false);
			lua_pushstring(L, ec.message().c_str());
		}
	}
	return 2;
}

// mas que Net esto es el equivalente a IOWatcher
/*const char* Net::className = "Net";
const Net::RegType Net::methods[] = {
	{"accept", &Net::Accept},
	{"stop", &Net::Stop},
	{"again", &Net::Again},
	{0}
};

const Net::RegType Net::setters[] = {
	LCB_ADD_SET(Net, repeat),
	{0}
};

const Net::RegType Net::getters[] = {
	LCB_ADD_GET(Net, timeout),
	LCB_ADD_GET(Net, repeat),
	{0}
};

Net::Net(lua_State* L) : 
	m_L(L),
	m_acceptor(GetIoService())
{
	LogDebug("Constructing Net (%p)", this);
}

Net::~Net(void)
{
	LogDebug("Destructing Net (%p)", this);
}

LCB_IMPL_GET(Net, timeout) {
	return 1;
}

LCB_IMPL_SET(Net, repeat) {
	return 0;
}
LCB_IMPL_GET(Net, repeat) {
	return 1;
}

//////////////////////////////////////////////////////////////////////////
/// 
int Net::CreateSocket(lua_State* L) {
	//m_acceptor
	return 0;
}

//////////////////////////////////////////////////////////////////////////
/// 
void Net::StartAccept() {
	boost::shared_ptr< boost::asio::ip::tcp::socket > socket(new boost::asio::ip::tcp::socket( GetIoService() ));
	
	m_acceptor.async_accept(
		*socket,
		boost::bind(&Net::HandleAccept, this, socket, boost::asio::placeholders::error)
	);
}

//////////////////////////////////////////////////////////////////////////
/// 
int Net::Accept(lua_State* L) {
	LogDebug("Net::Accept");

	SOCKET s = (SOCKET)luaL_checknumber(L, 2);

	m_acceptor.assign( boost::asio::ip::tcp::v4(), s );

	StartAccept();
	return 0;
}


//////////////////////////////////////////////////////////////////////////
/// 
void Net::HandleAccept(boost::shared_ptr< boost::asio::ip::tcp::socket > socket, const boost::system::error_code& error) {
	LogDebug("Net::HandleAccept (%p)", socket.get());
	if(!error) {
		// Get ready for another connection
		StartAccept();	// TODO: Esto lo voy a hacer desde Lua
		boost::asio::ip::address address = socket->remote_endpoint().address();
		//OnConnectionEstablished(socket, address);
		lua_State* L = m_L;

		GetSelf(L);

		boost::shared_ptr< boost::asio::ip::tcp::socket > socket2( new boost::asio::ip::tcp::socket( GetIoService() ) );

		boost::system::error_code ec;
		int v = socket->native();
		socket2->assign( boost::asio::ip::tcp::v4(), socket->native(), ec );
		if (ec) {
			LogError("%s", ec.message().c_str()); 
		}

		lua_getfield(L, 1, "callback");
		if(lua_type(L, 2) == LUA_TFUNCTION) {
			lua_newtable(L);
			int peer = lua_gettop(L);

			lua_pushstring(L, "fd");
			lua_pushnumber(L, socket->native());
			lua_rawset(L, peer);

			const std::string& sAddress = address.to_string();
			lua_pushstring(L, "address");
			lua_pushlstring(L, sAddress.c_str(), sAddress.length());
			lua_rawset(L, peer);

			lua_pushstring(L, "port");
			lua_pushnumber(L, socket->remote_endpoint().port());
			lua_rawset(L, peer);

			LuaNode::GetLuaEval().call(1, LUA_MULTRET);
		}
		else {
			// do nothing?
		}
		lua_settop(L, 0);
	}
	else {
		if(error != boost::asio::error::operation_aborted) {
			LogError("Net::HandleAccept (%p) - %s", socket.get(), error.message().c_str());
		}
		////Pm_allocator.ReleaseSocket(socket);
		// we don't call OnSocketReleased, because a connection had not been established
	}
}

//////////////////////////////////////////////////////////////////////////
/// 
int Net::Stop(lua_State* L) {
	LogDebug("Timer::Stop");
	
	return 0;
}

//////////////////////////////////////////////////////////////////////////
/// 
int Net::Again(lua_State* L) {
	LogDebug("Timer::Again");
	return 0;
}*/






const char* Socket::className = "Socket";
const Socket::RegType Socket::methods[] = {
	{"setoption", &Socket::SetOption},
	{"bind", &Socket::Bind},
	{"close", &Socket::Close},
	{"shutdown", &Socket::Shutdown},
	{"write", &Socket::Write},
	{"read", &Socket::Read},
	{"connect", &Socket::Connect},
	{"getsockname", &Socket::GetLocalAddress},
	{"getpeername", &Socket::GetRemoteAddress},
	{0}
};

const Socket::RegType Socket::setters[] = {
	{0}
};

const Socket::RegType Socket::getters[] = {
	{0}
};

long s_socketCount = 0;

Socket::Socket(lua_State* L) : 
	m_L(L)
{
	s_socketCount++;
	LogDebug("Constructing Socket (%p). Current socket count = %d", this, s_socketCount);

	const char* kind = luaL_checkstring(L, 1);
	LogDebug("Socket::Socket(%s)", kind);
	if(strcmp(kind, "tcp4") == 0) {
		m_socket.reset( new boost::asio::ip::tcp::socket( GetIoService(), boost::asio::ip::tcp::v4() ) );
	}
	else if(strcmp(kind, "tcp6") == 0) {
		m_socket.reset( new boost::asio::ip::tcp::socket( GetIoService(), boost::asio::ip::tcp::v6() ) );
	}
}

Socket::Socket(lua_State* L, boost::asio::ip::tcp::socket* socket) :
	m_L(L),
	m_socket(socket)
{
	s_socketCount++;
	LogDebug("Constructing Socket (%p). Current socket count = %d", this, s_socketCount);
}

Socket::~Socket(void)
{
	s_socketCount--;
	LogDebug("Destructing Socket (%p). Current socket count = %d", this, s_socketCount);
}

//////////////////////////////////////////////////////////////////////////
/// 
int Socket::GetRemoteAddress(lua_State* L) {
	const boost::asio::ip::tcp::endpoint& endpoint = m_socket->remote_endpoint();

	lua_pushstring(L, endpoint.address().to_string().c_str());
	lua_pushinteger(L, endpoint.port());
	return 2;
}

//////////////////////////////////////////////////////////////////////////
/// 
int Socket::GetLocalAddress(lua_State* L) {
	const boost::asio::ip::tcp::endpoint& endpoint = m_socket->local_endpoint();

	lua_pushstring(L, endpoint.address().to_string().c_str());
	lua_pushinteger(L, endpoint.port());
	return 2;
}

//////////////////////////////////////////////////////////////////////////
/// 
int Socket::SetOption(lua_State* L) {
	const char* options[] = {
		"reuseaddr",
		"nodelay",
		"keepalive",
		"no_option",
		NULL
	};
	const char* option = luaL_checkstring(L, 2);
	LogDebug("Socket::SetOption - %s", option);

	int chosen_option = luaL_checkoption(L, 2, "no_option", options);
	switch(chosen_option) {
		case 0: {	// reuseaddr
			bool value = lua_toboolean(L, 3) != 0;
			m_socket->set_option( boost::asio::socket_base::reuse_address(value) );
		break; }

		case 1: {	// nodelay
			bool value = lua_toboolean(L, 3) != 0;
			m_socket->set_option( boost::asio::ip::tcp::no_delay(value) );
		break; }

		case 2: {	// keepalive
			bool value = lua_toboolean(L, 3) != 0;
			m_socket->set_option( boost::asio::socket_base::keep_alive(value) );
			int time = lua_isnumber(L, 4) ? lua_tointeger(L, 4) : 0;
			int interval = lua_isnumber(L, 5) ? lua_tointeger(L, 5) : 0;
			if(value && time) {
#ifdef _WIN32
				DWORD dwBytes = 0;
				tcp_keepalive kaSettings = {0}, sReturned = {0};
				kaSettings.onoff = 1;
				kaSettings.keepalivetime = time;
				kaSettings.keepaliveinterval = interval;
				if (::WSAIoctl(m_socket->native(), SIO_KEEPALIVE_VALS, &kaSettings, sizeof(kaSettings), &sReturned, sizeof(sReturned), &dwBytes, NULL, NULL) != 0)
				{
					luaL_error(L, "Socket::SetOption (%p) - Failed to set keepalive on win32 native socket - %d", this, WSAGetLastError());
				}
#else // TODO: que ifdef ir�a ac� cuando winkler me preste la mac?
				if( setsockopt(m_socket->native(), SOL_TCP, TCP_KEEPIDLE, (void *)&time, sizeof(time)) ) {
					luaL_error(L, "Socket::SetOption (%p) - Failed to set keepalive (TCP_KEEPIDLE) on win32 native socket - %d", this, errno);
				}
				if( setsockopt(m_socket->native(), SOL_TCP, TCP_KEEPINTVL, (void *)&interval, sizeof(interval)) ) {
					luaL_error(L, "Socket::SetOption (%p) - Failed to set keepalive (TCP_KEEPINTVL) on win32 native socket - %d", this, errno);
				}
#endif
			}
		break; }
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
/// 
int Socket::Bind(lua_State* L) {
	const char* ip = luaL_checkstring(L, 2);
	unsigned short port = luaL_checkinteger(L, 3);

	boost::asio::ip::tcp::endpoint endpoint( boost::asio::ip::address::from_string(ip), port );
	
	boost::system::error_code ec;
	m_socket->bind(endpoint, ec);
	return BoostErrorCodeToLua(L, ec);
}

//////////////////////////////////////////////////////////////////////////
/// 
int Socket::Close(lua_State* L) {
	LogDebug("Socket::Close - Socket (%p)", this);
	boost::system::error_code ec;
	m_socket->close(ec);
	return BoostErrorCodeToLua(L, ec);
}

//////////////////////////////////////////////////////////////////////////
/// 
int Socket::Shutdown(lua_State* L) {
	static const char* options[] = {
		"read",
		"write",
		"both",
		"no_option",
		NULL
	};

	boost::system::error_code ec;

	if(lua_type(L, 2) == LUA_TSTRING) {
		const char* option = luaL_checkstring(L, 2);
		LogDebug("Socket::Shutdown - %s", option);
		
		int chosen_option = luaL_checkoption(L, 2, "no_option", options);
		switch(chosen_option) {
			case 0:	// read
				m_socket->shutdown(boost::asio::socket_base::shutdown_receive, ec);
			break;
	
			case 1:	// write
				m_socket->shutdown(boost::asio::socket_base::shutdown_send, ec);
			break;
	
			case 2:	// both
				m_socket->shutdown(boost::asio::socket_base::shutdown_both, ec);
			break;
	
			default:
			break;
		}
	}
	else {
		LogDebug("Socket::Shutdown - both");
		m_socket->shutdown(boost::asio::socket_base::shutdown_both, ec);
	}
	return BoostErrorCodeToLua(L, ec);
}

//////////////////////////////////////////////////////////////////////////
/// 
int Socket::Write(lua_State* L) {
	if(lua_type(L, 2) == LUA_TSTRING) {
		const char* data = lua_tostring(L, 2);
		size_t length = lua_objlen(L, 2);
		std::string d(data, length);
		shared_const_buffer buffer(d);

		LogDebug("Socket::Write (%p) - Length=%d, \r\n'%s'", this, length, data);
	
		boost::asio::async_write(*m_socket, buffer,
			boost::bind(&Socket::HandleWrite, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
		);
	}
	else {
		luaL_error(L, "Socket::Write, unhandled type '%s'", luaL_typename(L, 2));
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
/// 
void Socket::HandleWrite(const boost::system::error_code& error, size_t bytes_transferred) {
	if(!error) {
		LogInfo("Socket::HandleWrite (%p) - Bytes Transferred (%d)", this, bytes_transferred);
		
		lua_State* L = m_L;
		GetSelf(L);
		lua_getfield(L, 1, "write_callback");
		if(lua_type(L, 2) == LUA_TFUNCTION) {
			lua_pushvalue(L, 1);
			LuaNode::GetLuaEval().call(1, LUA_MULTRET);
		}
		else {
			// do nothing?
		}
		lua_settop(L, 0);
	}
	else {
		LogDebug("Socket::HandleWrite with error (%p) - %s", this, error.message().c_str());
		//m_callback.OnWriteCompletionError(shared_from_this(), bytes_transferred, error);
		lua_State* L = m_L;
		GetSelf(L);
		lua_getfield(L, 1, "write_callback");
		if(lua_type(L, 2) == LUA_TFUNCTION) {
			lua_pushvalue(L, 1);
			lua_pushnil(L);

			switch(error.value()) {
			case boost::asio::error::eof:
				lua_pushliteral(L, "eof");
				break;
#ifdef _WIN32
			case ERROR_CONNECTION_ABORTED:
#endif
			case boost::asio::error::connection_aborted:
				lua_pushliteral(L, "aborted");
				break;

			case boost::asio::error::operation_aborted:
				lua_pushliteral(L, "aborted");
				break;

			case boost::asio::error::connection_reset:
				lua_pushliteral(L, "reset");
				break;

			default:
				lua_pushstring(L, error.message().c_str());
				break;
			}

			LuaNode::GetLuaEval().call(3, LUA_MULTRET);
			m_inputBuffer.consume(m_inputBuffer.size());
		}
		else {
			LogError("Socket::HandleWrite with error (%p) - %s", this, error.message().c_str());
		}
		lua_settop(L, 0);
	}
}

//////////////////////////////////////////////////////////////////////////
/// 
int Socket::Read(lua_State* L) {
	if(lua_isnoneornil(L, 2)) {
		m_socket->async_read_some(
			boost::asio::buffer(m_inputArray), 
			boost::bind(&Socket::HandleReadSome, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
		);
	}
	else if(!lua_isnumber(L, 2)) {
		const char* p = luaL_optstring(L, 2, "*l");
		std::string delimiter = "\r\n";

		boost::asio::async_read_until(
			*m_socket, 
			m_inputBuffer, 
			delimiter,
			boost::bind(&Socket::HandleRead, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
		);
	}

	/*boost::asio::async_read(*m_socket, buffer,
		boost::bind(&Socket::HandleWrite, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
		);*/
	return 0;
}

//////////////////////////////////////////////////////////////////////////
/// 
void Socket::HandleRead(const boost::system::error_code& error, size_t bytes_transferred) {
	if(!error) {
		LogInfo("Socket::HandleRead (%p) - Bytes Transferred (%d)", this, bytes_transferred);
		lua_State* L = m_L;
		GetSelf(L);
		lua_getfield(L, 1, "read_callback");
		if(lua_type(L, 2) == LUA_TFUNCTION) {
			lua_pushvalue(L, 1);
			const char* data = (const char*)buffer_cast_helper(m_inputBuffer.data());
			lua_pushlstring(L, data, m_inputBuffer.size());
			m_inputBuffer.consume(m_inputBuffer.size());	// its safe to consume, the string has already been interned
			LuaNode::GetLuaEval().call(2, LUA_MULTRET);
		}
		else {
			// do nothing?
		}
		lua_settop(L, 0);
	}
	else {
		LogDebug("Socket::HandleRead with error (%p) - %s", this, error.message().c_str());

		lua_State* L = m_L;
		GetSelf(L);
		lua_getfield(L, 1, "read_callback");
		if(lua_type(L, 2) == LUA_TFUNCTION) {
			lua_pushvalue(L, 1);
			lua_pushnil(L);

			switch(error.value()) {
			case boost::asio::error::eof:
				lua_pushliteral(L, "eof");
				break;
#ifdef _WIN32
			case ERROR_CONNECTION_ABORTED:
#endif
			case boost::asio::error::connection_aborted:
				lua_pushliteral(L, "aborted");
				break;

			case boost::asio::error::operation_aborted:
				lua_pushliteral(L, "aborted");
				break;

			case boost::asio::error::connection_reset:
				lua_pushliteral(L, "reset");
				break;

			default:
				lua_pushstring(L, error.message().c_str());
				break;
			}

			LuaNode::GetLuaEval().call(3, LUA_MULTRET);
			m_inputBuffer.consume(m_inputBuffer.size());
		}
		else {
			LogError("Socket::HandleRead with error (%p) - %s", this, error.message().c_str());
		}
		lua_settop(L, 0);
	}
}

//////////////////////////////////////////////////////////////////////////
/// 
void Socket::HandleReadSome(const boost::system::error_code& error, size_t bytes_transferred) {
	const char* data = m_inputArray.c_array();
	if(!error) {
		LogInfo("Socket::HandleReadSome (%p) - Bytes Transferred (%d)", this, bytes_transferred);
		lua_State* L = m_L;
		GetSelf(L);
		lua_getfield(L, 1, "read_callback");
		if(lua_type(L, 2) == LUA_TFUNCTION) {
			lua_pushvalue(L, 1);
			const char* data = m_inputArray.c_array();
			lua_pushlstring(L, data, bytes_transferred);
			//m_inputBuffer.consume(m_inputBuffer.size());	// its safe to consume, the string has already been interned
			LuaNode::GetLuaEval().call(2, LUA_MULTRET);
		}
		else {
			// do nothing?
		}
		lua_settop(L, 0);
	}
	else {
		lua_State* L = m_L;
		GetSelf(L);
		lua_getfield(L, 1, "read_callback");
		if(lua_type(L, 2) == LUA_TFUNCTION) {
			lua_pushvalue(L, 1);
			lua_pushnil(L);

			switch(error.value()) {
			case boost::asio::error::eof:
				lua_pushliteral(L, "eof");
				break;
#ifdef _WIN32
			case ERROR_CONNECTION_ABORTED:
#endif
			case boost::asio::error::connection_aborted:
				lua_pushliteral(L, "aborted");
				break;

			case boost::asio::error::operation_aborted:
				lua_pushliteral(L, "aborted");
				break;

			case boost::asio::error::connection_reset:
				lua_pushliteral(L, "reset");
				break;

			default:
				lua_pushstring(L, error.message().c_str());
				break;
			}

			LuaNode::GetLuaEval().call(3, LUA_MULTRET);
			m_inputBuffer.consume(m_inputBuffer.size());
		}
		else {
			LogError("Socket::HandleRead with error (%p) - %s", this, error.message().c_str());
		}
		lua_settop(L, 0);
	}
}

//////////////////////////////////////////////////////////////////////////
/// 
int Socket::Connect(lua_State* L) {
	const char* ip = luaL_checkstring(L, 2);
	unsigned short port = luaL_checkinteger(L, 3);

	LogDebug("Socket::Connect(%s,%d)", ip, port);

	boost::asio::ip::tcp::endpoint endpoint( boost::asio::ip::address::from_string(ip), port );

	//boost::system::error_code ec;
	m_socket->async_connect(endpoint, 
		boost::bind(&Socket::HandleConnect, this, boost::asio::placeholders::error)
	);
	lua_pushboolean(L, true);
	return 1;

	//return BoostErrorCodeToLua(L, ec);
}

//////////////////////////////////////////////////////////////////////////
/// 
void Socket::HandleConnect(const boost::system::error_code& error) {
	if(!error) {
		LogInfo("Socket::HandleConnect (%p)", this);

		lua_State* L = m_L;
		GetSelf(L);
		lua_getfield(L, 1, "connect_callback");
		if(lua_type(L, 2) == LUA_TFUNCTION) {
			lua_pushvalue(L, 1);
			lua_pushboolean(L, true);
			LuaNode::GetLuaEval().call(2, LUA_MULTRET);
		}
		else {
			// do nothing?
		}
		lua_settop(L, 0);
	}
	else {	// este else pide refactor a gritos...
		lua_State* L = m_L;
		GetSelf(L);
		lua_getfield(L, 1, "connect_callback");
		if(lua_type(L, 2) == LUA_TFUNCTION) {
			lua_pushvalue(L, 1);
			lua_pushboolean(L, false);
			lua_pushstring(L, error.message().c_str());

			LuaNode::GetLuaEval().call(3, LUA_MULTRET);
			m_inputBuffer.consume(m_inputBuffer.size());
		}
		else {
			LogError("Socket::HandleConnect with error (%p) - %s", this, error.message().c_str());
		}
		lua_settop(L, 0);
	}
}








// mas que Net esto es el equivalente a IOWatcher
const char* Acceptor::className = "Acceptor";
const Acceptor::RegType Acceptor::methods[] = {
	{"open", &Acceptor::Open},
	{"close", &Acceptor::Close},
	{"setoption", &Acceptor::SetOption},
	{"getsockname", &Acceptor::GetLocalAddress},
	{"bind", &Acceptor::Bind},
	{"listen", &Acceptor::Listen},
	{"accept", &Acceptor::Accept},
	{0}
};

const Acceptor::RegType Acceptor::setters[] = {
	{0}
};

const Acceptor::RegType Acceptor::getters[] = {
	{0}
};

long s_acceptorCount = 0;

Acceptor::Acceptor(lua_State* L) : 
	m_L(L),
	m_acceptor(GetIoService())
{
	s_acceptorCount++;
	LogDebug("Constructing Acceptor (%p). Current acceptor count = %d", this, s_acceptorCount);
}

Acceptor::~Acceptor(void)
{
	s_acceptorCount--;
	LogDebug("Destructing Acceptor (%p). Current acceptor count = %d", this, s_acceptorCount);
}

//////////////////////////////////////////////////////////////////////////
/// Open the acceptor using the specified protocol
int Acceptor::Open(lua_State* L) {
	const char* kind = luaL_checkstring(L, 2);
	LogDebug("Acceptor::Open(%s)", kind);

	boost::system::error_code ec;
	if(strcmp(kind, "tcp4") == 0) {
		m_acceptor.open( boost::asio::ip::tcp::v4(), ec );
	}
	else if(strcmp(kind, "tcp6") == 0) {
		m_acceptor.open( boost::asio::ip::tcp::v6(), ec );
	}
	return BoostErrorCodeToLua(L, ec);
}

//////////////////////////////////////////////////////////////////////////
/// 
int Acceptor::Close(lua_State* L) {
	boost::system::error_code ec;
	m_acceptor.close(ec);
	return BoostErrorCodeToLua(L, ec);
}

//////////////////////////////////////////////////////////////////////////
/// 
int Acceptor::SetOption(lua_State* L) {
	const char* option = luaL_checkstring(L, 2);
	LogDebug("Acceptor::SetOption - %s", option);

	if(strcmp(option, "reuseaddr") == 0) {
		bool value = lua_toboolean(L, 3) != 0;
		m_acceptor.set_option( boost::asio::socket_base::reuse_address(value) );
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
/// 
int Acceptor::GetLocalAddress(lua_State* L) {
	const boost::asio::ip::tcp::endpoint& endpoint = m_acceptor.local_endpoint();

	lua_pushstring(L, endpoint.address().to_string().c_str());
	lua_pushinteger(L, endpoint.port());
	return 2;
}

//////////////////////////////////////////////////////////////////////////
/// 
int Acceptor::Bind(lua_State* L) {
	const char* ip = luaL_checkstring(L, 2);
	unsigned short port = luaL_checkinteger(L, 3);

	LogDebug("Acceptor::Bind(%s,%d)", ip, port);

	boost::system::error_code ec;
	boost::asio::ip::address address = boost::asio::ip::address::from_string(ip, ec);
	if(ec) {
		return BoostErrorCodeToLua(L, ec);
	}

	boost::asio::ip::tcp::endpoint endpoint( address, port );
	m_acceptor.bind(endpoint, ec);
	return BoostErrorCodeToLua(L, ec);
}

//////////////////////////////////////////////////////////////////////////
/// 
int Acceptor::Listen(lua_State* L) {
	int backlog = luaL_optinteger(L, 2, boost::asio::socket_base::max_connections);
	LogDebug("Acceptor::Listen(%d)", backlog);

	boost::system::error_code ec;
	m_acceptor.listen(backlog, ec);
	return BoostErrorCodeToLua(L, ec);
}

//////////////////////////////////////////////////////////////////////////
/// 
int Acceptor::Accept(lua_State* L) {
	LogDebug("Acceptor::Accept");

	boost::asio::ip::tcp::socket* socket = new boost::asio::ip::tcp::socket( GetIoService() );

	m_acceptor.async_accept(
		*socket,
		boost::bind(&Acceptor::HandleAccept, this, socket, boost::asio::placeholders::error)
	);
	return 0;
}

//////////////////////////////////////////////////////////////////////////
/// 
void Acceptor::HandleAccept(boost::asio::ip::tcp::socket* socket, const boost::system::error_code& error) {
	LogDebug("Acceptor::HandleAccept (new socket %p) (%p)", socket, this);
	if(!error) {
		// Get ready for another connection
		//StartAccept();	// TODO: Esto lo voy a hacer desde Lua
		boost::asio::ip::address address = socket->remote_endpoint().address();
		//OnConnectionEstablished(socket, address);
		lua_State* L = m_L;

		GetSelf(L);

		lua_getfield(L, 1, "callback");
		if(lua_type(L, 2) == LUA_TFUNCTION) {
			lua_newtable(L);
			int peer = lua_gettop(L);

			lua_pushstring(L, "socket");
			Socket* luasocket = new Socket(L, socket);
			//Socket::push(L, luasocket, false/*true*/);	// now Socket is the owner
			Socket::push(L, luasocket, true);	// now Socket is the owner
			if(Socket::s_trackingEnabled) {
				luasocket->KeepTrack(L);
			}
			lua_rawset(L, peer);

			const std::string& sAddress = address.to_string();
			lua_pushstring(L, "address");
			lua_pushlstring(L, sAddress.c_str(), sAddress.length());
			lua_rawset(L, peer);

			lua_pushstring(L, "port");
			lua_pushnumber(L, socket->remote_endpoint().port());
			lua_rawset(L, peer);

			LuaNode::GetLuaEval().call(1, LUA_MULTRET);
		}
		else {
			// do nothing?
		}
		lua_settop(L, 0);
	}
	else {
		if(error != boost::asio::error::operation_aborted) {
			LogError("Acceptor::HandleAccept (new socket %p) (%p) - %s", socket, this, error.message().c_str());
		}
		////Pm_allocator.ReleaseSocket(socket);
		// we don't call OnSocketReleased, because a connection had not been established
	}
}
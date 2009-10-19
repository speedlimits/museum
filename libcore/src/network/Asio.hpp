/*  Sirikata Network Utilities
 *  Asio.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SIRIKATA_ASIO_HPP_
#define _SIRIKATA_ASIO_HPP_

/** Note: All the classes provided in this file are thin wrappers around
 *  the corresponding classes in ASIO.  These are provided because ASIO
 *  has problems allocating these objects from different threads on some
 *  platforms.
 */

#include <boost/asio.hpp>

namespace Sirikata {
namespace Network {

typedef boost::asio::ip::tcp::socket InternalTCPSocket;
typedef boost::asio::ip::tcp::acceptor InternalTCPAcceptor;
typedef boost::asio::ip::tcp::resolver InternalTCPResolver;

/** Simple wrapper around Boost.Asio's tcp::socket, allowing for safe,
 *  cross-platform allocation and use.
 */
class SIRIKATA_EXPORT TCPSocket: public InternalTCPSocket {
  public:
    TCPSocket(IOService&io);
};

/** Simple wrapper around Boost.Asio's tcp::acceptor, allowing for safe,
 *  cross-platform allocation and use.
 */
class SIRIKATA_EXPORT TCPListener :public InternalTCPAcceptor {
public:
    TCPListener(IOService&io, const boost::asio::ip::tcp::endpoint&);
    void async_accept(TCPSocket&socket,
                      const std::tr1::function<void(const boost::system::error_code& ) > &cb);
};

/** Simple wrapper around Boost.Asio's tcp::resolver, allowing for safe,
 *  cross-platform allocation and use.
 */
class SIRIKATA_EXPORT TCPResolver : public InternalTCPResolver {
  public:
    TCPResolver(IOService&io);
};




typedef boost::asio::ip::udp::socket InternalUDPSocket;
typedef boost::asio::ip::udp::resolver InternalUDPResolver;

/** Simple wrapper around Boost.Asio's udp::socket, allowing for safe,
 *  cross-platform allocation and use.
 */
class SIRIKATA_EXPORT UDPSocket: public InternalUDPSocket {
  public:
    UDPSocket(IOService&io);
};

/** Simple wrapper around Boost.Asio's udp::resolver, allowing for safe,
 *  cross-platform allocation and use.
 */
class SIRIKATA_EXPORT UDPResolver : public InternalUDPResolver {
  public:
    UDPResolver(IOService&io);
};

/** Simple wrapper around Boost.Asio's deadline_timer, allowing for safe,
 *  cross-platform allocation and use.  If you just want a timer that works
 *  with IOService, see IOTimer.
 */
class DeadlineTimer : public boost::asio::deadline_timer {
public:
    DeadlineTimer(IOService& io);
};

#define TCPSSTLOG(thisname,extension,buffer,buffersize,error)
// #define TCPSSTLOG(thisname,extension,buffer,buffersize,error)  if (!error) {Sirikata::Network::ASIOLogBuffer(thisname,extension,(buffersize)?(buffer):NULL,buffersize);}

} // namespace Network
} // namespace Sirikata

#endif //_SIRIKATA_ASIO_HPP_
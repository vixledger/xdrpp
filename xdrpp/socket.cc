#include <cstring>
#include <string>
#include <xdrpp/socket.h>

namespace xdr {

using std::string;

#if 0
string
addrinfo_to_string(const addrinfo *ai)
{
  std::ostringstream os;
  bool first{true};
  while (ai) {
    if (first)
      first = false;
    else
      os << ", ";
    string h, p;
    get_numinfo(ai->ai_addr, ai->ai_addrlen, &h, &p);
    if (h.find(':') == string::npos)
      os << h << ':' << p;
    else
      os << '[' << h << "]:" << p;
    ai = ai -> ai_next;
  }
  return os.str();
}
#endif

namespace{
struct gai_category_impl : public std::error_category {
  const char *name() const noexcept override { return "DNS"; }
  string message(int err) const override { return gai_strerror(err); }
};
}

const std::error_category &
gai_category()
{
  static gai_category_impl cat;
  return cat;
}

namespace {
string
cat_host_service(const char *host, const char *service)
{
  string target;
  if (host) {
    if (std::strchr(host, ':')) {
      target += "[";
      target += host;
      target += "]";
    }
    else
      target = host;
  }
  else
    target += "NULL";		// NULL should give localhost
  if (service) {
    target += ":";
    target += service;
  }
  return target;
}
} // namespace

unique_addrinfo get_addrinfo(const char *host, int socktype,
			     const char *service, int family)
{
  addrinfo hints, *res;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = socktype;
  hints.ai_family = family;
  hints.ai_flags = AI_ADDRCONFIG;
  int err = getaddrinfo(host, service, &hints, &res);
  if (err)
    throw std::system_error(err, gai_category(),
			    cat_host_service(host, service));
  return unique_addrinfo(res);
}

void get_numinfo(const sockaddr *sa, socklen_t salen,
		 string *host, string *serv)
{
  char hostbuf[NI_MAXHOST];
  char servbuf[NI_MAXSERV];
  int err = getnameinfo(sa, salen, hostbuf, sizeof(hostbuf),
			servbuf, sizeof(servbuf),
			NI_NUMERICHOST|NI_NUMERICSERV);
  if (err)
    throw std::system_error(err, gai_category(), "getnameinfo");
  if (host)
    *host = hostbuf;
  if (serv)
    *serv = servbuf;
}

unique_sock
tcp_connect1(const addrinfo *ai, bool ndelay)
{
  unique_sock s(socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol));
  if (!s)
    throw_sockerr("socket");
  if (ndelay)
    set_nonblock(s.get());
  if (connect(s.get().fd_, ai->ai_addr, ai->ai_addrlen) == -1
      && errno != EINPROGRESS)
    s.clear();
  return s;
}

unique_sock
tcp_connect(const addrinfo *ai)
{
  unique_sock s;
  errno = EADDRNOTAVAIL;
  for (; ai && !s; ai = ai->ai_next)
    if ((s = tcp_connect1(ai)))
      return s;
  throw_sockerr("connect");
}

unique_sock
tcp_connect(const char *host, const char *service, int family)
{
  return tcp_connect(get_addrinfo(host, SOCK_STREAM, service, family));
}

unique_sock
tcp_listen(const char *service, int family, int backlog)
{
  addrinfo hints, *res;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = family;
  hints.ai_flags = AI_ADDRCONFIG | AI_PASSIVE;
  int err = getaddrinfo(nullptr, service ? service : "0", &hints, &res);
  if (err)
    throw std::system_error(err, gai_category(), "AI_PASSIVE");
  unique_addrinfo ai{res};

  // XXX
  //std::cerr << "listening at " << addrinfo_to_string(ai.get()) << std::endl;

  unique_sock s(sock_t(socket(ai->ai_family, ai->ai_socktype,
			      ai->ai_protocol)));
  if (!s)
    throw_sockerr("socket");
  if (bind(s.get().fd_, ai->ai_addr, ai->ai_addrlen) == -1)
    throw_sockerr("bind");
  if (listen (s.get().fd_, backlog) == -1)
    throw_sockerr("listen");
  return s;
}




}

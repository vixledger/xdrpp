
#include <xdrpp/marshal.h>

namespace xdr {

msg_ptr
message_t::alloc(std::size_t size)
{
  // In RPC (see RFC5531 section 11), the high bit means this is the
  // last record fragment in a record.  If the high bit is clear, it
  // means another fragment follows.  We don't currently implement
  // continuation fragments, and instead always set the last-record
  // bit to produce a single-fragment record.
  assert(size < 0x80000000);
  void *raw = operator new(offsetof(message_t, buf_[size + 4]));
  if (!raw)
    throw std::bad_alloc();
  message_t *m = new (raw) message_t (size);
  *reinterpret_cast<std::uint32_t *>(m->raw_data()) =
    swap32le(size32(size) | 0x80000000);
  return msg_ptr(m);
}

void
marshal_base::get_bytes(const std::uint32_t *&pr, void *buf, std::size_t len)
{
  const char *p = reinterpret_cast<const char *>(pr);
  std::memcpy(buf, p, len);
  p += len;
  while (len & 3) {
    ++len;
    if (*p++ != '\0')
      throw xdr_should_be_zero("Non-zero padding bytes encountered");
  }
  pr = reinterpret_cast<const std::uint32_t *>(p);
}

void
marshal_base::put_bytes(std::uint32_t *&pr, const void *buf, std::size_t len)
{
  char *p = reinterpret_cast<char *>(pr);
  std::memcpy(p, buf, len);
  p += len;
  while (len & 3) {
    ++len;
    *p++ = '\0';
  }
  pr = reinterpret_cast<std::uint32_t *>(p);
}

}

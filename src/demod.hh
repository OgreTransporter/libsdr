#ifndef __SDR_DEMOD_HH__
#define __SDR_DEMOD_HH__

#include "node.hh"
#include "traits.hh"
#include "config.hh"
#include "combine.hh"
#include "logger.hh"


namespace sdr {


/** Amplitude modulation (AM) demodulator from an I/Q signal. */
template <class Scalar>
class AMDemod
    : public Sink< std::complex<Scalar> >, public Source
{
public:
  /** Constructor. */
  AMDemod() : Sink< std::complex<Scalar> >(), Source()
  {
    // pass...
  }

  /** Destructor. */
  virtual ~AMDemod() {
    // pass...
  }

  /** Configures the AM demod. */
  virtual void config(const Config &src_cfg) {
    // Requires type & buffer size
    if (!src_cfg.hasType() || !src_cfg.hasBufferSize()) { return; }
    // Check if buffer type matches template
    if (Config::typeId< std::complex<Scalar> >() != src_cfg.type()) {
      ConfigError err;
      err << "Can not configure AMDemod: Invalid type " << src_cfg.type()
          << ", expected " << Config::typeId< std::complex<Scalar> >();
      throw err;
    }
    // Allocate buffer
    _buffer =  Buffer<Scalar>(src_cfg.bufferSize());

    LogMessage msg(LOG_DEBUG);
    msg << "Configure AMDemod:" << std::endl
        << " input type: " << Traits< std::complex<Scalar> >::scalarId << std::endl
        << " output type: " << Traits<Scalar>::scalarId << std::endl
        << " sample rate: " << src_cfg.sampleRate() << std::endl
        << " buffer size: " << src_cfg.bufferSize();
    Logger::get().log(msg);

    // Propergate config
    this->setConfig(Config(Config::typeId<Scalar>(), src_cfg.sampleRate(),
                           src_cfg.bufferSize(), src_cfg.numBuffers()));
  }

  /** Handles the I/Q input buffer. */
  virtual void process(const Buffer<std::complex<Scalar> > &buffer, bool allow_overwrite)
  {
    // Drop buffer if output buffer is still in use:
    if (! _buffer.isUnused()) {
#ifdef SDR_DEBUG
      LogMessage msg(LOG_WARNING);
      msg << __FILE__ << ": Output buffer still in use: Drop received buffer...";
      Logger::get().log(msg);
      return;
#endif
    }

    Buffer<Scalar> out_buffer;
    // If source allow to overwrite the buffer, use it otherwise rely on own buffer
    if (allow_overwrite) { out_buffer = Buffer<Scalar>(buffer); }
    else { out_buffer = _buffer; }

    // Perform demodulation
    for (size_t i=0; i<buffer.size(); i++) {
      out_buffer[i] = std::sqrt(buffer[i].real()*buffer[i].real() +
                                buffer[i].imag()*buffer[i].imag());
    }

    // If the source allowed to overwrite the buffer, this source will allow it too.
    // If this source used the internal buffer (_buffer), it allows to overwrite it anyway.
    this->send(out_buffer.head(buffer.size()), true);
  }

protected:
  /** The output buffer. */
  Buffer<Scalar> _buffer;
};


/** SSB upper side band (USB) demodulator from an I/Q signal. */
template <class Scalar>
class USBDemod
    : public Sink< std::complex<Scalar> >, public Source
{
public:
  /** The complex input scalar. */
  typedef std::complex<Scalar> CScalar;
  /** The real compute scalar. */
  typedef typename Traits<Scalar>::SScalar SScalar;

public:
  /** Constructor. */
  USBDemod() : Sink<CScalar>(), Source()
  {
    // pass...
  }
  /** Destructor. */
  virtual ~USBDemod() {
    // pass...
  }

  /** Configures the USB demodulator. */
  virtual void config(const Config &src_cfg) {
    // Requires type & buffer size
    if (!src_cfg.hasType() || !src_cfg.hasBufferSize()) { return; }
    // Check if buffer type matches template
    if (Config::typeId<CScalar>() != src_cfg.type()) {
      ConfigError err;
      err << "Can not configure USBDemod: Invalid type " << src_cfg.type()
          << ", expected " << Config::typeId<CScalar>();
      throw err;
    }
    // Allocate buffer
    _buffer =  Buffer<Scalar>(src_cfg.bufferSize());

    LogMessage msg(LOG_DEBUG);
    msg << "Configure USBDemod:" << std::endl
        << " input type: " << Traits< std::complex<Scalar> >::scalarId << std::endl
        << " output type: " << Traits<Scalar>::scalarId << std::endl
        << " sample rate: " << src_cfg.sampleRate() << std::endl
        << " buffer size: " << src_cfg.bufferSize();
    Logger::get().log(msg);

    // Propergate config
    this->setConfig(Config(Config::typeId<Scalar>(), src_cfg.sampleRate(),
                           src_cfg.bufferSize(), 1));
  }

  /** Performs the demodulation. */
  virtual void process(const Buffer<CScalar> &buffer, bool allow_overwrite) {
    if (allow_overwrite) {
      // Process in-place
      _process(buffer, Buffer<Scalar>(buffer));
    } else if (_buffer.isUnused()) {
      // Store result in buffer
      _process(buffer, _buffer);
    } else {
      // Drop buffer
#ifdef SDR_DEBUG
      LogMessage msg(LOG_WARNING);
      msg << "SSBDemod: Drop buffer.";
      Logger::get().log(msg);
#endif
    }
  }

protected:
  /** The actual demodulation. */
  void _process(const Buffer< std::complex<Scalar> > &in, const Buffer< Scalar> &out) {
    for (size_t i=0; i<in.size(); i++) {
      out[i] = (SScalar(std::real(in[i])) + SScalar(std::imag(in[i])))/2;
    }
    this->send(out.head(in.size()));
  }

protected:
  /** The output buffer. */
  Buffer<Scalar> _buffer;
};



/** Demodulates FM from an I/Q signal. */
template <class iScalar, class oScalar=iScalar>
class FMDemod: public Sink< std::complex<iScalar> >, public Source
{
public:
  /** The super scalar. */
  typedef typename Traits<iScalar>::SScalar SScalar;

public:
  /** Constructor. */
  FMDemod():
    Sink< std::complex<iScalar> >(), Source(), _shift(0), _can_overwrite(false)
  {
    _shift = 8*(sizeof(oScalar)-sizeof(iScalar));
  }

  /** Destructor. */
  virtual ~FMDemod() {
    // pass...
  }

  /** Configures the FM demodulator. */
  virtual void config(const Config &src_cfg) {
    // Requires type & buffer size
    if (!src_cfg.hasType() || !src_cfg.hasBufferSize()) { return; }
    // Check if buffer type matches template
    if (Config::typeId< std::complex<iScalar> >() != src_cfg.type()) {
      ConfigError err;
      err << "Can not configure FMDemod: Invalid type " << src_cfg.type()
          << ", expected " << Config::typeId< std::complex<iScalar> >();
      throw err;
    }
    // Unreference buffer if non-empty
    if (! _buffer.isEmpty()) { _buffer.unref(); }
    // Allocate buffer
    _buffer =  Buffer<oScalar>(src_cfg.bufferSize());
    // reset last value
    _last_value = 0;
    // Check if FM demod can be performed in-place
    _can_overwrite = (sizeof(std::complex<iScalar>) >= sizeof(oScalar));

    LogMessage msg(LOG_DEBUG);
    msg << "Configured FMDemod node:" << std::endl
        << " sample-rate: " << src_cfg.sampleRate() << std::endl
        << " in-type / out-type: " << src_cfg.type()
        << " / " << Config::typeId<oScalar>() << std::endl
        << " in-place: " << (_can_overwrite ? "true" : "false") << std::endl
        << " output scale: 2^" << _shift;
    Logger::get().log(msg);

    // Propergate config
    this->setConfig(Config(Config::typeId<oScalar>(), src_cfg.sampleRate(),
                           src_cfg.bufferSize(), 1));
  }

  /** Performs the FM demodulation. */
  virtual void process(const Buffer<std::complex<iScalar> > &buffer, bool allow_overwrite)
  {
    if (0 == buffer.size()) { return; }

    if (allow_overwrite && _can_overwrite) {
      _process(buffer, Buffer<oScalar>(buffer));
    } else if (_buffer.isUnused()) {
      _process(buffer, _buffer);
    } else {
#ifdef SDR_DEBUG
      LogMessage msg(LOG_WARNING);
      msg << "FMDemod: Drop buffer: Output buffer still in use.";
      Logger::get().log(msg);
#endif
    }
  }

protected:
  /** A fast approximative implementation of the std::atan2() on integers. */
  inline SScalar _fast_atan2(SScalar a, SScalar b) {
    const SScalar pi4 = (1<<(Traits<oScalar>::shift-4));
    const SScalar pi34 = 3*(1<<(Traits<oScalar>::shift-4));
    a >>= (9-_shift); b >>= (9-_shift);
    SScalar aabs, angle;
    if ((0 == a) && (0 == b)) { return 0; }
    aabs = (a >= 0) ? a : -a;
    if (b >= 0) { angle = pi4 - pi4*(b-aabs) / (b+aabs); }
    else { angle = pi34 - pi4*(b+aabs) / (aabs-b); }
    return (a >= 0) ? angle : -angle;
  }

  /** The actual demodulation. */
  void _process(const Buffer< std::complex<iScalar> > &in, const Buffer<oScalar> &out)
  {
    // The last input value
    std::complex<iScalar> last_value = _last_value;
    // calc first value
    SScalar a = SScalar(in[0].real())*SScalar(last_value.real())
        + SScalar(in[0].imag())*SScalar(last_value.imag());
    SScalar b = SScalar(in[0].imag())*SScalar(last_value.real())
        - SScalar(in[0].real())*SScalar(last_value.imag());

    // update last value
    last_value = in[0];
    // calc output (prob. overwriting the last value)
    out[0] = _fast_atan2(a, b);

    // Calc remaining values
    for (size_t i=1; i<in.size(); i++) {
      a = SScalar(in[i].real())*SScalar(last_value.real())
          + SScalar(in[i].imag())*SScalar(last_value.imag());
      b = SScalar(in[i].imag())*SScalar(last_value.real())
          - SScalar(in[i].real())*SScalar(last_value.imag());
      last_value = in[i];
      out[i] = _fast_atan2(a, b);
    }

    // Store last value
    _last_value = last_value;
    // propergate result
    this->send(out.head(in.size()));
  }


protected:
  /** Output rescaling. */
  int _shift;
  /** The last input value. */
  std::complex<iScalar> _last_value;
  /** If true, in-place demodulation is poissible. */
  bool _can_overwrite;
  /** The output buffer, unused if demodulation is performed in-place. */
  Buffer<oScalar> _buffer;
};


/** Binary phase shift demodulation with carrier from an I/Q signal. */
template <class Scalar>
class BPSKDemod : public Combine< std::complex<Scalar> >, public Source
{
public:
  /** Complex input scalar type. */
  typedef std::complex<Scalar> CScalar;
  /** Real super scalar. */
  typedef typename Traits<Scalar>::SScalar SScalar;
  /** Complex super scalar. */
  typedef std::complex<SScalar> CSScalar;

public:
  /** Constructor. */
  BPSKDemod()
    : Combine<CScalar>(2), Source(), _buffer(0), _last(0)
  {
    // pass...
  }

  /** Destructor. */
  virtual ~BPSKDemod() {
    _buffer.unref();
  }

  /** The signal input sink. */
  inline Sink<CScalar> *signal() { return Combine<CScalar>::_sinks[0]; }
  /** The reference/carrier input sink. */
  inline Sink<CScalar> *reference() { return Combine<CScalar>::_sinks[1]; }

  /** Configures the BPSK demodulator. */
  virtual void config(const Config &cfg) {
    // Requires type and buffer size
    if(!cfg.hasType() || !cfg.hasBufferSize()) { return; }
    // Check type
    if (Config::typeId<CScalar>() != cfg.type()) {
      ConfigError err;
      err << "Can not configure BPSKDemod node: Invalid type " << cfg.type()
          << ", expected " << Config::typeId<CScalar>();
      throw err;
    }
    // Allocate buffer
    _buffer = Buffer<uint8_t>(cfg.bufferSize());
    // Propergate config
    this->setConfig(Config(Config::Type_u8, cfg.sampleRate(), _buffer.size(), 1));
  }

  /** Performs the BPSK demodulation. */
  virtual void process(std::vector< RingBuffer< std::complex<Scalar> > > &buffers, size_t N)
  {
    // If there is nothing -> done
    if (0 == N) { return; }
    // If buffer is still in use, drop input
    if (!_buffer.isUnused()) {
#ifdef SDR_DEBUG
      LogMessage msg(LOG_WARNING);
      msg << "BPSKDemod: Output buffer still in use. Drop input.";
      Logger::get().log(msg);
#endif

      for (size_t i=0; i<Combine<CScalar>::_sinks.size(); i++) {
        Combine<CScalar>::_buffers[i].drop(N);
      }
      return;
    }
    // Process
    N = std::min(N, _buffer.size());
    // Compute first value:
    CSScalar tmp = (CSScalar(buffers[0][0])*CSScalar(std::conj(buffers[1][0])));
    SScalar a = _last.real()*tmp.real() + _last.imag()*tmp.imag();
    SScalar b = _last.real()*tmp.imag() - _last.imag()*tmp.real();
    _buffer[0] = (std::abs(std::atan2(a,b))>M_PI/2); _last = tmp;
    for (size_t i=0; i<N; i++) {
      tmp = (CSScalar(buffers[0][i])*CSScalar(std::conj(buffers[1][i])));
      a = _last.real()*tmp.real() + _last.imag()*tmp.imag();
      b = _last.real()*tmp.imag() - _last.imag()*tmp.real();
      _buffer[i] = (std::abs(std::atan2(a,b))>M_PI/2); _last = tmp;
    }
    // Consume elements
    for (size_t i=0; i<Combine<CScalar>::_sinks.size(); i++) {
        Combine<CScalar>::_buffers[i].drop(N);
    }
    // Send buffer
    this->send(_buffer.head(N));
  }

protected:
  /** Output buffer (bits). */
  Buffer<uint8_t> _buffer;
  /** The last input value (signal). */
  CSScalar _last;
};

}

#endif // __SDR_DEMOD_HH__
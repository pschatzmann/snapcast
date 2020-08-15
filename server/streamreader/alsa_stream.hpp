/***
    This file is part of snapcast
    Copyright (C) 2014-2020  Johannes Pohl

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
***/

#ifndef ALSA_STREAM_HPP
#define ALSA_STREAM_HPP

#include "pcm_stream.hpp"
#include <alsa/asoundlib.h>
#include <boost/asio.hpp>

namespace streamreader
{


/// Reads and decodes PCM data from an alsa audio device device
/**
 * Reads PCM from an alsa audio device device and passes the data to an encoder.
 * Implements EncoderListener to get the encoded data.
 * Data is passed to the PcmListener
 */
class AlsaStream : public PcmStream
{
public:
    /// ctor. Encoded PCM data is passed to the PipeListener
    AlsaStream(PcmListener* pcmListener, boost::asio::io_context& ioc, const StreamUri& uri);

    void start() override;
    void stop() override;

protected:
    void do_read();
    void initAlsa();
    void uninitAlsa();

    snd_pcm_t* handle_;
    std::unique_ptr<msg::PcmChunk> chunk_;
    bool first_;
    std::chrono::time_point<std::chrono::steady_clock> nextTick_;
    boost::asio::steady_timer read_timer_;
    std::string device_;
    std::vector<char> silent_chunk_;
    std::chrono::microseconds silence_;
    std::string lastException_;
};

} // namespace streamreader

#endif

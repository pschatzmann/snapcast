/***
    This file is part of snapcast
    Copyright (C) 2015  Johannes Pohl

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

#include <boost/lexical_cast.hpp>
#include <iostream>
#include <mutex>
#include "controlSession.h"
#include "common/log.h"
#include "message/pcmChunk.h"

using namespace std;



ControlSession::ControlSession(ControlMessageReceiver* receiver, std::shared_ptr<tcp::socket> socket) : messageReceiver_(receiver)
{
	socket_ = socket;
}


ControlSession::~ControlSession()
{
	stop();
}


void ControlSession::start()
{
	active_ = true;
	readerThread_ = new thread(&ControlSession::reader, this);
	writerThread_ = new thread(&ControlSession::writer, this);
}


void ControlSession::stop()
{
	std::unique_lock<std::mutex> mlock(mutex_);
	active_ = false;
	try
	{
		boost::system::error_code ec;
		if (socket_)
		{
			socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			if (ec) logE << "Error in socket shutdown: " << ec << "\n";
			socket_->close(ec);
			if (ec) logE << "Error in socket close: " << ec << "\n";
		}
		if (readerThread_)
		{
			logD << "joining readerThread\n";
			readerThread_->join();
			delete readerThread_;
		}
		if (writerThread_)
		{
			logD << "joining writerThread\n";
			writerThread_->join();
			delete writerThread_;
		}
	}
	catch(...)
	{
	}
	readerThread_ = NULL;
	writerThread_ = NULL;
	socket_ = NULL;
	logD << "ControlSession stopped\n";
}



void ControlSession::add(const std::string& message)
{
	messages_.push(message);
}


bool ControlSession::send(const std::string& message) const
{
//	logO << "send: " << message->type << ", size: " << message->size << ", id: " << message->id << ", refers: " << message->refersTo << "\n";
	std::unique_lock<std::mutex> mlock(mutex_);
	if (!socket_ || !active_)
		return false;
	boost::asio::streambuf streambuf;
	std::ostream request_stream(&streambuf);
	request_stream << message << "\r\n";
	boost::asio::write(*socket_.get(), streambuf);
//	logO << "done: " << message->type << ", size: " << message->size << ", id: " << message->id << ", refers: " << message->refersTo << "\n";
	return true;
}


void ControlSession::reader()
{
	active_ = true;
	try
	{
		while (active_)
		{
			boost::asio::streambuf response;
			boost::asio::read_until(*socket_, response, "\r\n");
			std::istream response_stream(&response);
			std::string strResponse;
			response_stream >> strResponse;
			if (messageReceiver_ != NULL)
				messageReceiver_->onMessageReceived(this, strResponse);
		}
	}
	catch (const std::exception& e)
	{
		logS(kLogErr) << "Exception in ControlSession::reader(): " << e.what() << endl;
	}
	active_ = false;
}


void ControlSession::writer()
{
	try
	{
		boost::asio::streambuf streambuf;
		std::ostream stream(&streambuf);
		string message;
		while (active_)
		{
			if (messages_.try_pop(message, std::chrono::milliseconds(500)))
				send(message);
		}
	}
	catch (const std::exception& e)
	{
		logS(kLogErr) << "Exception in ControlSession::writer(): " << e.what() << endl;
	}
	active_ = false;
}


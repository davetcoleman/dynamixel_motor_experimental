/*
 * GearBox Project: Peer-Reviewed Open-Source Libraries for Robotics
 *               http://gearbox.sf.net/
 * Copyright (c) 2004-2010 Alex Brooks
 *
 * This distribution is licensed to you under the terms described in
 * the LICENSE file included in this distribution.
 *
 */

#ifndef SICK_ACFR_SERIALHANDLER_H
#define SICK_ACFR_SERIALHANDLER_H

#include <gbxsickacfr/messages.h>
#include <gbxserialacfr/serial.h>
#include <gbxsickacfr/gbxiceutilacfr/thread.h>
#include <gbxsickacfr/gbxiceutilacfr/buffer.h>
#include <gbxsickacfr/gbxserialdeviceacfr/serialdevicehandler.h>
#include <gbxsickacfr/gbxserialdeviceacfr/stickinbuffercallback.h>

namespace gbxsickacfr {

class RxMsgParser : public gbxserialdeviceacfr::RxMsgParser 
{
public:
    gbxserialdeviceacfr::RxMsgPtr parseBuffer( const std::vector<char>       &buffer,
                                               int                           &numBytesParsed )
        {
            LmsRxMsgPtr lmsRxMsg = parseBufferForRxMsgs( (const uChar*)&(buffer[0]),
                                                         buffer.size(),
                                                         numBytesParsed );
            return lmsRxMsg;
        }

};

// LmsRxMsg plus a timeStamp
class TimedLmsRxMsg {
public:
    TimedLmsRxMsg() {}
    TimedLmsRxMsg( int s, int us, const LmsRxMsgPtr &r )
        : timeStampSec(s), timeStampUsec(us), msg(r) {}

    int timeStampSec;
    int timeStampUsec;
    LmsRxMsgPtr msg;
};

//
// @brief Handles the serial port.
//
// Read in this separate loop so we can hopefully grab the messages
// as soon as they arrive, without relying on having Driver::read()
// called by an external thread which may be doing other stuff.
// This will hopefully give us more accurate timestamps.
//
// @author Alex Brooks
//
class SerialHandler
{

public: 

    SerialHandler( const std::string   &dev,
                   gbxutilacfr::Tracer &tracer,
                   gbxutilacfr::Status &status );
    ~SerialHandler();

    void send( const std::vector<uChar> &telegram )
        { serialDeviceHandler_->send( (const char*)&(telegram[0]), telegram.size() ); }

    void setBaudRate( int baudRate )
        { serialDeviceHandler_->setBaudRate( baudRate ); }

    // waits up to maxWaitMs for a RxMsg
    // return codes same as gbxiceutilacfr::Buffer
    int getNextRxMsg( TimedLmsRxMsg &timedRxMsg, int maxWaitMs )
        { 
            gbxserialdeviceacfr::TimedRxMsg genericTimedRxMsg;
            int ret = bufferCallback_.rxMsgBuffer().getAndPopWithTimeout( genericTimedRxMsg, maxWaitMs );
            if ( ret == 0 )
            {
                timedRxMsg.timeStampSec = genericTimedRxMsg.timeStampSec;
                timedRxMsg.timeStampUsec = genericTimedRxMsg.timeStampUsec;

                // This cast is safe becuase the rxMsg had to have been generated by RxMsgParser
                timedRxMsg.msg = LmsRxMsgPtr::dynamicCast( genericTimedRxMsg.msg );
            }
            return ret;
        }

private: 

    RxMsgParser                               rxMsgParser_;
    gbxserialdeviceacfr::StickInBufferCallback bufferCallback_;
    gbxserialacfr::Serial                     serialPort_;
    gbxserialdeviceacfr::SerialDeviceHandler *serialDeviceHandler_;
    // Keep a smart pointer to the SerialDeviceHandler as a thread, for stop/start purposes
    gbxiceutilacfr::ThreadPtr                 serialDeviceHandlerThreadPtr_;
};

}

#endif
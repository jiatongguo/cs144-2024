#include "tcp_receiver.hh"

void TCPReceiver::receive( TCPSenderMessage message )
{
    if ( writer().has_error() )
    {
        return;
    }

    if ( message.RST == true )
    {
        reader().set_error();
        return;
    }

    //set the initial sequence number
    if ( not zero_point_.has_value() )
    {
        if ( message.SYN == false )
        {
            return;
        }

        zero_point_.emplace( message.seqno );
    }

    const uint64_t check_point = writer().bytes_pushed();
    const uint64_t abs_seq = message.seqno.unwrap(zero_point_.value(), check_point );
    const uint64_t stream_index = ( message.SYN == true ) ? 0 : abs_seq - 1;

    reassembler_.insert( stream_index, message.payload, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
    if ( reader().has_error() )
    {
        return { std::nullopt, 0, true };
    }

    TCPReceiverMessage message;

    message.window_size = ( writer().available_capacity() > UINT16_MAX ) ? UINT16_MAX : writer().available_capacity();

    if ( zero_point_.has_value() )
    {
        const uint64_t ack = writer().bytes_pushed() + 1 + static_cast<uint64_t> ( writer().is_closed() );
        message.ackno = Wrap32::wrap( ack, zero_point_.value() );
    }

    return message;
}

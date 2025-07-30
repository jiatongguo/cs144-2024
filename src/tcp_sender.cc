#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return outstanding_bytes_;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.
  (void)transmit;
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return { Wrap32::wrap(next_abs_seqno_, isn_), false, {}, false, input_.has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  (void)msg;
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if (timer_running_) 
  {
    timer_elapsed_ += ms_since_last_tick;
  }

  if (timer_elapsed_ >= RTO_ms_) // 超时
  {
    if (outstanding_segments_.empty())  
    {
      return;
    }

    transmit(outstanding_segments_.front());  // 重传

    if (window_size_ != 0)
    {
      ++consecutive_retransmissions_;
      RTO_ms_ *= 2;
    }

    timer_elapsed_ = 0; // 重置计时器
  }
}

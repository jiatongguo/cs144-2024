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
  if (!msg.ackno.has_value())
  {
    return;
  }

  window_size_ = msg.window_size; // 更新窗口

  uint64_t abs_ackno {msg.ackno.value().unwrap(isn_, next_abs_seqno_)};

  while (!outstanding_segments_.empty())
  {
    TCPSenderMessage seg { outstanding_segments_.front() };

    if (seg.seqno.unwrap(isn_, next_abs_seqno_) + seg.sequence_length() <= abs_ackno) // 已ack, pop
    {
      outstanding_bytes_ -= seg.sequence_length();
      outstanding_segments_.pop();
    }

    else
    {
      break;
    }
  }

  RTO_ms_ = initial_RTO_ms_;
  timer_elapsed_ = 0;
  consecutive_retransmissions_ = 0;

  if (outstanding_segments_.empty())
  {
    timer_running_ = false;
    timer_elapsed_ = 0;
  }
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

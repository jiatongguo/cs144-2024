#include "tcp_sender.hh"
#include "tcp_config.hh"
#include <algorithm>

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
  while (true)
  {
    if (window_size_ == 0) // 发送窗口为0时置1
    {
      window_size_ = 1;  
    }

    if (outstanding_bytes_ > window_size_)
    {
      return;
    }

    TCPSenderMessage msg { make_empty_message() };
    if (!SYN_sent_)  // 发syn包
    {
        msg.SYN = true;
        SYN_sent_ = true;
    }

    size_t max_payload = std::min(TCPConfig::MAX_PAYLOAD_SIZE, 
                            window_size_ - outstanding_bytes_ - (msg.SYN ? 1 : 0));

    std::string payload;
    while (reader().bytes_buffered() != 0 && max_payload) // 缓冲区有数据且有空间
    { 
          std::string_view view = reader().peek();
          view = view.substr(0, max_payload);
          payload += view;
          max_payload -= payload.size();
          input_.reader().pop(view.size()); // 从缓冲区删除
    }

    msg.payload = payload;

    if (!FIN_sent_&& (window_size_ > outstanding_bytes_ + msg.sequence_length()) && reader().is_finished() )
    {
        FIN_sent_ = true;
        msg.FIN  = true;
    }
    
    if (msg.sequence_length() == 0) // 没有任何信息的空段
    {
      return;
    }

    transmit(msg);

    if (!timer_running_) // 启动计时器
    {
        timer_running_ = true; 
        timer_elapsed_ = 0;
    }

    next_abs_seqno_ += msg.sequence_length(); 
    outstanding_bytes_ += msg.sequence_length(); // 更新已send未ack字节数
    outstanding_segments_.emplace(std::move(msg)); // 插入队列
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return { Wrap32::wrap(next_abs_seqno_, isn_), false, {}, false, input_.has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  window_size_ = msg.window_size; // 更新窗口
  if (msg.window_size == 0)
  {
    zero_window_ = true;
  }

  if (!msg.ackno.has_value())
  {
    return;
  }

  uint64_t abs_ackno {msg.ackno.value().unwrap(isn_, next_abs_seqno_)};

  if (abs_ackno > next_abs_seqno_) // 不可能的ack
  {
    return;
  }
  
  bool acked_any {false};
  while (!outstanding_segments_.empty())
  {
    TCPSenderMessage seg { outstanding_segments_.front() };

    if (seg.seqno.unwrap(isn_, next_abs_seqno_) + seg.sequence_length() <= abs_ackno) // 已ack, pop
    {
      outstanding_bytes_ -= seg.sequence_length();
      outstanding_segments_.pop();

      acked_any = true;
    }

    else
    {
      break;
    }
  }

  if (acked_any)
  {
    RTO_ms_ = initial_RTO_ms_;
    timer_elapsed_ = 0;
    consecutive_retransmissions_ = 0;

    if (outstanding_segments_.empty())
    {
      timer_running_ = false;
      timer_elapsed_ = 0;
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if (!timer_running_)
  {
    return;
  }

  timer_elapsed_ += ms_since_last_tick;

  if (timer_elapsed_ >= RTO_ms_) // 超时
  {
    if (outstanding_segments_.empty())  
    {
      return;
    }

    transmit(outstanding_segments_.front());  // 重传

    if (!zero_window_)
    {
      ++consecutive_retransmissions_;
      RTO_ms_ *= 2;
    }

    timer_elapsed_ = 0; // 重置计时器
  }
}

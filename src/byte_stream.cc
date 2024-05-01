#include "byte_stream.hh"
#include <algorithm>
#include <utility>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

bool Writer::is_closed() const
{
    return is_closed_;
}

void Writer::push( string data )
{ 
    (void)data;
  
    if (is_closed() == true)
    {
        set_error();
        return;
    }
    

    if (available_capacity() == 0 || data.empty() )
        return;

    const auto push_len = std::min(available_capacity(), data.size() );

    if (data.size() > push_len)
    {
        data = data.substr(0, push_len); //截断
    }

    buffer_deque_.push_back(std::move(data) ) ; //将数据推送到缓冲区
    view_deque_.emplace_back(buffer_deque_.back().c_str(), push_len); //

    buffered_bytes_counter_ += push_len;
    pushed_bytes_counter_ += push_len;
}

void Writer::close()
{
    is_closed_ = true;
}

uint64_t Writer::available_capacity() const
{
    return capacity_ - buffered_bytes_counter_;
}

uint64_t Writer::bytes_pushed() const
{
    return pushed_bytes_counter_;
}

bool Reader::is_finished() const
{
    return is_closed_ && pushed_bytes_counter_ ==  popped_bytes_counter_;
}

uint64_t Reader::bytes_popped() const
{
    return popped_bytes_counter_;
}

std::string_view Reader::peek() const
{
    if (view_deque_.empty() )
        return {};

    return view_deque_.front();
}

void Reader::pop( uint64_t len )
{
    (void)len;

    auto pop_len = std::min(len, buffered_bytes_counter_);

    while (pop_len > 0)
    {
        if (view_deque_.empty() )   return;

        auto sz = view_deque_.front().size();

        if (pop_len < sz) //
        {
            view_deque_.front().remove_prefix(pop_len); 
            //无需对buffer_queue更改

            buffered_bytes_counter_ -= pop_len;
            popped_bytes_counter_ += pop_len;

            return;
        }

        else // pop_len > sz
        {
            view_deque_.pop_front();
            buffer_deque_.pop_front();

            pop_len = pop_len - sz; //更新pop_len

            buffered_bytes_counter_ -= sz;
            popped_bytes_counter_ += sz;
        }
    }
}

uint64_t Reader::bytes_buffered() const
{
    return buffered_bytes_counter_;
}

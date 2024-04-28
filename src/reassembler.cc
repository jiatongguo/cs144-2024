#include "reassembler.hh"

void Reassembler::insert( uint64_t first_index, std::string data, bool is_last_substring )
{
    if (data.empty() )
    {
        if (is_last_substring == true and output_.writer().bytes_pushed() == first_index)
            output_.writer().close();
        return;
    }

    if (is_last_substring == true)
    {
        is_last_ = true;
        end_mark_ = first_index + data.size();
    }

    //stream is closed or capacity is not enough
    if (output_.writer().is_closed() == true or output_.writer().available_capacity() == 0)
        return;

    first_unassembled_index_ = output_.writer().bytes_pushed();
    first_unaccepted_index_ = first_unassembled_index_ + output_.writer().available_capacity();

    //out of range
    if (first_index + data.size() <=  first_unassembled_index_ or first_index >= first_unaccepted_index_)
        return;

    //handle overlapping with has pushed
    if (first_index < first_unassembled_index_)
    {
        data.erase(0, first_unassembled_index_ - first_index);
        first_index = first_unassembled_index_;
    }

    //remove the unaccepted bytes
    if (first_index + data.size() >= first_unaccepted_index_)
    {
        data.resize(first_unaccepted_index_ - first_index);
        is_last_substring = false; //not fininshed
    }

    auto last_index = first_index + data.size() - 1; //the right bound
    auto left_it = buffer_map_.lower_bound(first_index);
    //handle the left overlapping
    if (left_it != buffer_map_.begin() ) //not the first element but can be not found(return the end iter)
    {
        auto prev = std::prev(left_it);
        //left overlapping
        if (prev->first + prev->second.size() - 1 >= first_index)
        {
            //data is fully overlapped
            if (prev->first + prev->second.size() - 1 >= last_index)
                return;

            data = prev->second.substr(0, first_index - prev->first) + data; //merge
            first_index = prev->first;
            pending_bytes_counter_ -= prev->second.size();
            buffer_map_.erase(prev);
        }
    }

    else
    { //is the first position
        if (not buffer_map_.empty() )
        {
            if (first_index == left_it->first and last_index <= left_it->first + left_it->second.size() - 1)
                return;
        }
    }
    //handle right overlapping
    auto right_it = buffer_map_.lower_bound(last_index);
    if (right_it != buffer_map_.begin() )
    {
        auto prev = std::prev(right_it);
        if (last_index <= prev->first + prev->second.size() - 1)
        {
            data += prev->second.substr(last_index - prev->first);
            last_index = prev->first + prev->second.size() - 1;
        }
    }

    //erase [left_it, right_it)
    while (left_it != right_it)
    {
        pending_bytes_counter_ -= left_it->second.size();
        left_it = buffer_map_.erase(left_it);
    }

    //handle the right_it
    if (right_it != buffer_map_.end() and right_it->first == last_index)
    {
        //size == 1 越界
        if (right_it->second.size() > 1)
        {
            data += right_it->second.substr(1);
            last_index = right_it->first + right_it->second.size() - 1;
        }

        pending_bytes_counter_ -= right_it->second.size();
        buffer_map_.erase(right_it);
    }

    //check the bytes after
    if (first_index == first_unassembled_index_)
    {
        first_unassembled_index_= last_index + 1;
        output_.writer().push(data);

        auto it2 = buffer_map_.lower_bound(first_unassembled_index_);
        while (it2 != buffer_map_.end() )
        {
            if (it2->first != output_.writer().bytes_pushed() )
            {
                break;
            }

            else
            {
                pending_bytes_counter_ -= it2->second.size();
                output_.writer().push(it2->second);
                it2 = buffer_map_.erase(it2);
            }
        }
    }

    else
    {
        buffer_map_[first_index] = data;
        pending_bytes_counter_ += data.size();
    }

    if (is_last_ == true and output_.writer().bytes_pushed() == end_mark_)
        output_.writer().close();
}

uint64_t Reassembler::bytes_pending() const
{
    return pending_bytes_counter_;
}                                                         

#include "reassembler.hh"

void Reassembler::insert(uint64_t first_index, std::string data, bool is_last_substring)
{
    if ( is_last_substring == true )
    {
        is_last_ = true;
        end_index_ = first_index + data.size();
    }

    if ( data.empty() )
    {
        if (is_last_substring == true and output_.writer().bytes_pushed() == first_index)
        {
            output_.writer().close();
        }
        return;
    }

    //stream is closed or capacity is not enough
    if (output_.writer().is_closed() == true or output_.writer().available_capacity() == 0)
    {
        return;
    }

    first_unassembled_index_ = output_.writer().bytes_pushed();
    first_unaccepted_index_ = first_unassembled_index_ + output_.writer().available_capacity();

    //out of range
    if ( first_index + data.size() <= first_unassembled_index_ or first_index >= first_unaccepted_index_ )
    {
        return;
    }

    //handle overlapping with pushed
    if ( first_index < first_unassembled_index_ )
    {
        data.erase(0, first_unassembled_index_ - first_index);
        first_index = first_unassembled_index_;
    }

    //remove the unaccepted bytes
    if ( first_index + data.size() >= first_unaccepted_index_ )
    {
        data.resize(first_unaccepted_index_ - first_index);
        is_last_substring = false; //not fininshed
    }

    auto last_index = first_index + data.size() - 1; //the right bound

    //handle the left overlapping
    auto left_it = buffer_map_.upper_bound( first_index );
    if ( left_it != buffer_map_.begin() ) //not the first element but can be not found(return the end iter)
    {
        auto prev = std::prev( left_it );
        //left overlapping
        if ( prev->first + prev->second.size() - 1 >= first_index )
        {
            //data is fully overlapped
            if ( prev->first + prev->second.size() - 1 >= last_index )
            {
                return;
            }

            data = prev->second.substr( 0, first_index - prev->first ) + data; //merge
            first_index = prev->first;
            pending_bytes_counter_ -= prev->second.size();
            buffer_map_.erase(prev);
        }
    }

    //handle right overlapping
    auto right_it = buffer_map_.upper_bound( last_index );
    if ( right_it != buffer_map_.begin() )
    {
        auto prev = std::prev( right_it );
        if ( last_index <= prev->first + prev->second.size() - 1 )
        {
            data += prev->second.substr( last_index - prev->first + 1 ); //!!
            last_index = prev->first + prev->second.size() - 1;
        }
    }

    while ( left_it != right_it )
    {
        pending_bytes_counter_ -= left_it->second.size();
        left_it = buffer_map_.erase( left_it );
    } //erase [left_it, right_it)

    pending_bytes_counter_ += data.size();
    buffer_map_.emplace_hint(left_it, first_index, std::move( data ) );

    //check the bytes after
    while ( not buffer_map_.empty() )
    {
        auto it = buffer_map_.begin();
        if ( it->first != output_.writer().bytes_pushed() )
        {
            break;
        }

        pending_bytes_counter_ -= it->second.size();
        output_.writer().push( it->second );
        buffer_map_.erase(it);
    }

    if ( is_last_ == true and output_.writer().bytes_pushed() == end_index_ )
    {
        output_.writer().close();
    }
}

uint64_t Reassembler::bytes_pending() const
{
    return pending_bytes_counter_;
}
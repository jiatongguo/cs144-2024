#include "wrapping_integers.hh"

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
    //mod 2^32, remaining the low bits
    return zero_point + static_cast<uint32_t> (n);
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
    uint32_t offset = this->raw_value_ - wrap( checkpoint, zero_point ).raw_value_; //calc the offest
    uint64_t abs_seq = checkpoint + offset;
    /*
     * negative number wraparound
     * [-2^31, 2^31 -1] -2^31 + 2^32 == 2^31
     *
     * [0, 2^32 - 1]
     */
    //find the closest
    if ( offset > ( 1u << 31 ) and abs_seq >= ( 1ul << 32 ) )
    {
        abs_seq -= 1ul << 32;
    }

    return abs_seq;
}

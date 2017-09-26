//
//  Stream.hxx
//  IPC
//
//  Created by Brandon on 2017-09-24.
//  Copyright © 2017 Brandon. All rights reserved.
//

#ifndef STREAM_HXX_INCLUDED
#define STREAM_HXX_INCLUDED

#include <vector>
#include <list>
#include <string>
#include <istream>
#include <streambuf>
#include <cstring>


/********************************************//**
 * @brief A class for wrapping a memory buffer as an std::basic_streambuf directly.
 *
 * @param char_type - Type of buffer. char, wchar_t, unsigned char, etc.
 * @param traits_type - Type traits for the specified char_type.
 *                      Default value = std::char_traits<char_type>.
 ***********************************************/
template<typename char_type, typename traits_type = std::char_traits<char_type>>
class DirectStreamBuffer : public std::basic_streambuf<char_type, traits_type>
{
public:
    /********************************************//**
     * @brief Constructs a direct memory stream from a given buffer.
     *
     * @param buffer char_type* - Buffer to be used by the stream.
     * @param buffer_size std::size_t - Size of the buffer provided.
     *
     ***********************************************/
    DirectStreamBuffer(char_type* buffer, std::size_t buffer_size)
    {
        std::basic_streambuf<char_type, traits_type>::pubsetbuf(nullptr, 0);
        std::basic_streambuf<char_type, traits_type>::setp(buffer, buffer + buffer_size);
        std::basic_streambuf<char_type, traits_type>::setg(buffer, buffer, buffer + buffer_size);
    }
};




/********************************************//**
 * @brief A class for wrapping a memory buffer as a IO stream directly.
 *
 * @param char_type - Type of buffer. char, wchar_t, unsigned char, etc.
 * @param traits_type - Type traits for the specified char_type.
 *                      Default value = std::char_traits<char_type>.
 ***********************************************/
template<typename char_type, typename traits_type = std::char_traits<char_type>>
class DirectStream : public std::basic_iostream<char_type, traits_type>
{
protected:
    DirectStreamBuffer<char_type, traits_type> sbuf;

public:
    /********************************************//**
     * @brief Constructs a direct memory stream from a given buffer.
     *
     * @param buffer char_type* - Buffer to be used by the stream.
     * @param buffer_size std::size_t - Size of the buffer provided.
     *
     ***********************************************/
    DirectStream(char_type* buffer, std::size_t buffer_size)
        : std::basic_iostream<char_type, traits_type>(nullptr), sbuf(buffer, buffer_size)
    {std::basic_iostream<char_type, traits_type>::init(&sbuf);}
};




/********************************************//**
 * @brief A CRTP base class for wrapping a buffer as an std::basic_streambuf.
 *        All derived classes MUST implement the read, write, seek and tell functions.
 *
 * @param stream_type - Type of the derived class.
 * @param char_type - Type of buffer. char, wchar_t, unsigned char, etc.
 * @param traits_type - Type traits for the specified char_type.
 *                      Default value = std::char_traits<char_type>.
 ***********************************************/
template<typename stream_type, typename char_type, typename traits_type = std::char_traits<char_type>>
class StreamBuffer : public std::basic_streambuf<char_type, traits_type>
{
protected:
    using parent_type = std::basic_streambuf<char_type, traits_type>;
    using int_type = typename parent_type::int_type;
    using pos_type = typename parent_type::pos_type;
    using off_type = typename parent_type::off_type;
    using seekdir = typename std::ios_base::seekdir;
    using openmode = typename std::ios_base::openmode;

private:
    char_type* pBuffer;
    std::size_t pBufferSize;
    std::size_t bBufferSize;

    /********************************************//**
     * @brief Synchronises the back-buffer with the underlying buffer IFF Buffered-IO is enabled.
     *
     * @return int - Zero upon success; Negative one otherwise.
     *
     ***********************************************/
    int sync();


    /********************************************//**
     * @brief Flushes the back-buffer to the underlying buffer IFF Buffered-IO is enabled.
     *
     * @return int_type - traits_type::eof() upon failure. Amount of char_type's flushed otherwise.
     *
     ***********************************************/
    int_type flush();


    /********************************************//**
     * @brief Reads from the char_type from the underlying buffer at the current controlled input sequence
     *        without advancing the current pointer. If no char_type is available, traits_type::eof() is returned.
     *
     * @return int_type - traits_type::eof() upon failure. Current character in the controlled input sequence otherwise.
     *
     ***********************************************/
    int_type underflow();


    /********************************************//**
     * @brief Reads from the char_type from the underlying buffer at the current controlled output sequence
     *        without advancing the current pointer. If no char_type is available, traits_type::eof() is returned.
     *
     * @return int_type - traits_type::eof() upon failure. Current character in the controlled output sequence otherwise.
     *
     ***********************************************/
    int_type overflow(int_type c  = traits_type::eof());


    /********************************************//**
     * @brief - Alters the position of one or more of the controlled sequences and advances the current pointer.
     *
     * @param pos off_type - Offset value for the position pointer relative to dir.
     * @param dir seekdir - Can be std::ios_base::beg, std::ios_base::cur, std::ios_base::end.
     *                      Determines where the offset should start.
     * @param openmode mode - Determines which control sequence should be altered.
     *                        Can be: std::ios_base::in or std::ios_base::out or both. Defaults to both.
     * @return pos_type - Position within the stream after the alteration.
     *
     ***********************************************/
    pos_type seekoff(off_type pos, seekdir dir, openmode mode = parent_type::in | parent_type::out);


    /********************************************//**
     * @brief - Alters the position of one or more of the controlled sequences and advances the current pointer.
     *
     * @param pos pos_type - Absolute offset value for the position pointer.
     * @param openmode mode - Determines which control sequence should be altered.
     *                        Can be: std::ios_base::in or std::ios_base::out or both. Defaults to both.
     * @return pos_type - Position within the stream after the alteration.
     *
     ***********************************************/
    pos_type seekpos(pos_type pos, openmode mode = parent_type::in | parent_type::out);

public:
    /********************************************//**
     * @brief StreamBuffer constructor. Constructs a stream buffer from a given buffer via the CRTP pattern.
     *        Derived classes must maintain their own buffer's life-time. This base class needs to know the size
     *        of the back-buffer! NOT the actual derived class's buffer.
     *
     * @param bBufferSize std::size_t - Size of the back-buffer to be used. Defaults to 0 for Direct-IO.
     * @param pBufferSize std::size_t - Size of the put-back buffer to be used. Defaults to sizeof(char_type). Can be 0.
     *
     ***********************************************/
    StreamBuffer(std::size_t bBufferSize = 0, std::size_t pBufferSize = sizeof(char_type));


    /********************************************//**
     * @brief Destructor for cleaning up the back-buffer if used.
     ***********************************************/
    virtual ~StreamBuffer();


    /********************************************//**
     * @brief Copy construction is disabled. Copying a StreamBuffer is not allowed.
     *
     * @param other const StreamBuffer& - StreamBuffer to be copied.
     *
     ***********************************************/
    StreamBuffer(const StreamBuffer& other) = delete;


    /********************************************//**
     * @brief Copy assignment is disabled. Copying a StreamBuffer is not allowed.
     *
     * @param other const StreamBuffer& other - StreamBuffer to be copied.
     *
     ***********************************************/
    StreamBuffer& operator = (const StreamBuffer& other) = delete;


    /********************************************//**
     * @brief Functions to be overriden/implemented by the derived class.
     *
     * std::size_t read(char_type* data, std::size_t amount, std::size_t element_size);
     * std::size_t write(char_type* data, std::size_t amount, std::size_t element_size);
     * bool seek(std::size_t position, seekdir direction);
     * pos_type tell();
     *
     ***********************************************/
};

template<typename stream_type, typename char_type, typename traits_type>
StreamBuffer<stream_type, char_type, traits_type>::StreamBuffer(std::size_t bBufferSize, std::size_t pBufferSize) : pBuffer(nullptr), pBufferSize(std::max(pBufferSize, static_cast<std::size_t>(1))), bBufferSize(std::max(bBufferSize, this->pBufferSize) + this->pBufferSize)
{
    pBuffer = new char_type[this->bBufferSize];
    char_type* end = pBuffer + this->bBufferSize;
    parent_type::setg(end, end, end);
    parent_type::setp(pBufferSize > 0 ? pBuffer : nullptr, pBufferSize > 0 ? pBuffer : nullptr);
}

template<typename stream_type, typename char_type, typename traits_type>
StreamBuffer<stream_type, char_type, traits_type>::~StreamBuffer()
{
    sync();
    delete[] pBuffer;
}

template<typename stream_type, typename char_type, typename traits_type>
int StreamBuffer<stream_type, char_type, traits_type>::sync()
{
    return flush() == traits_type::eof() ? -1 : 0;
}

template<typename stream_type, typename char_type, typename traits_type>
typename StreamBuffer<stream_type, char_type, traits_type>::int_type StreamBuffer<stream_type, char_type, traits_type>::flush()
{
    std::ptrdiff_t amount = parent_type::pptr() - parent_type::pbase();
    if(amount > 0 && static_cast<std::ptrdiff_t>(static_cast<stream_type*>(this)->write(parent_type::pbase(), amount, sizeof(char_type))) == amount)
    {
        parent_type::pbump(-amount);
        return amount;
    }

    return traits_type::eof();
}

template<typename stream_type, typename char_type, typename traits_type>
typename StreamBuffer<stream_type, char_type, traits_type>::int_type StreamBuffer<stream_type, char_type, traits_type>::underflow()
{
    if(parent_type::gptr() < parent_type::egptr())
    {
        return traits_type::to_int_type(*parent_type::gptr());
    }

    char_type* offset = pBuffer;
    if(parent_type::eback() == pBuffer)
    {
        std::memmove(pBuffer, parent_type::egptr() - pBufferSize, pBufferSize);
        offset += pBufferSize;
    }

    std::size_t amount = static_cast<stream_type*>(this)->read(offset, bBufferSize - (offset - pBuffer), sizeof(char_type));
    if(amount != 0)
    {
        parent_type::setg(pBuffer, offset, offset + amount);
        return traits_type::to_int_type(*parent_type::gptr());
    }

    return traits_type::eof();
}

template<typename stream_type, typename char_type, typename traits_type>
typename StreamBuffer<stream_type, char_type, traits_type>::int_type StreamBuffer<stream_type, char_type, traits_type>::overflow(typename StreamBuffer<stream_type, char_type, traits_type>::int_type c)
{
    if(!parent_type::pptr() && c != traits_type::eof())
    {
        char_type ch = traits_type::to_char_type(c);
        return static_cast<stream_type*>(this)->write(&ch, 1, sizeof(char_type)) == 1 ? c : traits_type::eof();
    }

    if(parent_type::pptr() && c != traits_type::eof())
    {
        *parent_type::pptr() = c;
        parent_type::pbump(1);
    }

    return flush() == traits_type::eof() ? traits_type::eof() : c;
}

template<typename stream_type, typename char_type, typename traits_type>
typename StreamBuffer<stream_type, char_type, traits_type>::pos_type StreamBuffer<stream_type, char_type, traits_type>::seekoff(typename StreamBuffer<stream_type, char_type, traits_type>::off_type pos, std::ios_base::seekdir dir, std::ios_base::openmode mode)
{
    if(mode & std::ios_base::in)
    {
        if(mode & std::ios_base::out)
        {
            return pos_type(off_type(-1));
        }

        if(static_cast<stream_type*>(this)->seek(pos, dir))
        {
            return pos_type(off_type(-1));
        }

        parent_type::setg(parent_type::eback(), parent_type::egptr(), parent_type::egptr());
    }
    else if(mode & std::ios_base::out)
    {
        if(static_cast<stream_type*>(this)->seek(pos, dir))
        {
            return pos_type(off_type(-1));
        }
        parent_type::setp(parent_type::pbase(), parent_type::epptr());
    }
    else
    {
        return pos_type(off_type(-1));
    }

    return pos_type(static_cast<stream_type*>(this)->tell());
}

template<typename stream_type, typename char_type, typename traits_type>
typename StreamBuffer<stream_type, char_type, traits_type>::pos_type StreamBuffer<stream_type, char_type, traits_type>::seekpos(typename StreamBuffer<stream_type, char_type, traits_type>::pos_type pos, std::ios_base::openmode mode)
{
    return parent_type::seekoff(off_type(pos), std::ios_base::beg, mode);
}




/********************************************//**
 * @brief A class for serialising all data-types & containers into a direct buffer.
 ***********************************************/
class Stream
{
private:
    DirectStream<char> Data;

    /********************************************//**
     * @brief Determines if two types are the exact same when decayed at compile time.
     *
     * @param T - Type to compare to.
     * @param U - Type that is decayed first and compared to T.
     * @return std::integral_constant<bool, true> if the types matched, std::integral_constant<bool, false> otherwise.
     *
     ***********************************************/
    template<typename T, typename U>
    struct is_same_decay : public std::integral_constant<bool, std::is_same<T, typename std::decay<U>::type>::value> {};


    /********************************************//**
     * @brief Compares a type to a char*, or const char* when decayed at compile time.
     *
     * @param T - The type to decay and compare.
     * @return std::integral_constant<bool, true> if the types matched, std::integral_constant<bool, false> otherwise.
     *
     ***********************************************/
    template<typename T>
    struct is_cstring : public std::integral_constant<bool, is_same_decay<char*, T>::value || is_same_decay<const char*, T>::value> {};


    /********************************************//**
     * @brief Compares a type to a wchar_t*, or const wchar_t* when decayed at compile time.
     *
     * @param T - The type to decay and compare.
     * @return std::integral_constant<bool, true> if the types matched, std::integral_constant<bool, false> otherwise.
     *
     ***********************************************/
    template<typename T>
    struct is_wcstring : public std::integral_constant<bool, is_same_decay<wchar_t*, T>::value || is_same_decay<const wchar_t*, T>::value> {};


public:
    /********************************************//**
     * @brief Constructs a stream class.
     *
     * @param Buffer void* - Buffer to serialise/de-serialise data to/from.
     * @param BufferSize std::size_t - Size of the specified buffer.
     *
     ***********************************************/
    Stream(void* Buffer, std::size_t BufferSize) : Data(static_cast<char*>(Buffer), BufferSize) {}


    /********************************************//**
     * @brief Reads a value from the underlying buffer. Amount to read is specified by sizeof(T).
     *         Where T represents the type of value to be read.
     *
     * @param T& Value - Buffer or Variable to hold the value read from the underlying buffer.
     * @return void
     *
     ***********************************************/
    template<typename T>
    void Read(T &Value);


    /********************************************//**
     * @brief Writes a value to the underlying buffer. Amount to write is specified by sizeof(T).
     *         Where T represents the type of value to be written.
     *
     * @param const T& Value - Value containing data to be written to the underlying buffer.
     * @return void
     *
     ***********************************************/
    template<typename T>
    void Write(const T &Value);


    /********************************************//**
     * @brief Insertion operator for inserting data into the underlying buffer.
     *
     * @param const T& Value - Buffer or Variable containing data to be inserted into the underlying buffer.
     * @return Stream& Reference to the current Stream for chain insertion. Returns a reference *this.
     *
     * Same as calling the Read function.
     *
     ***********************************************/
    template<typename T>
    typename std::enable_if<!is_cstring<T>::value && !is_wcstring<T>::value, Stream&>::type operator << (const T &Value);


    /********************************************//**
     * @brief Extraction operator for extracting data from the underlying buffer.
     *
     * @param T& Value - Buffer or Variable in which to store the extracted data from the underlying buffer.
     * @return Stream& Reference to the current Stream for chain extraction. Returns a reference to *this.
     *
     * Same as calling the Write function.
     *
     ***********************************************/
    template<typename T>
    typename std::enable_if<!is_cstring<T>::value && !is_wcstring<T>::value, Stream&>::type operator >> (T &Value);


    /********************************************//**
     * @brief Insertion operator for inserting the contents of an std::vector into the underlying buffer.
     *
     * @param const std::vector<T, Allocator>& Value - An std::vector containing the data to be inserted into the underlying buffer.
     * @return Stream& Reference to the current Stream for chain insertion. Returns a reference to *this.
     *
     ***********************************************/
    template<typename T, typename Allocator>
    Stream& operator << (const std::vector<T, Allocator> &Value);


    /********************************************//**
     * @brief Insertion operator for inserting the contents of an std::list into the underlying buffer.
     *
     * @param const std::list<T, Allocator>& Value - An std::list containing the data to be inserted into the underlying buffer.
     * @return Stream& Reference to the current Stream for chain insertion. Returns a reference to *this.
     *
     ***********************************************/
    template<typename T, typename Allocator>
    Stream& operator << (const std::list<T, Allocator> &Value);


    /********************************************//**
     * @brief Insertion operator for inserting the contents of an std::basic_string into the underlying buffer.
     *
     * @param const std::basic_string<T, Allocator>& Value - An std::basic_string containing the data to be inserted into the underlying buffer.
     * @return Stream& Reference to the current Stream for chain insertion. Returns a reference to *this.
     *
     ***********************************************/
    template<typename T, typename Allocator>
    Stream& operator << (const std::basic_string<T, Allocator> &Value);


    /********************************************//**
     * @brief Extraction operator for extracting the contents of the underlying buffer into an std::vector.
     *
     * @param std::vector<T, Allocator>& Value - An std::vector in which to store the extracted data from the underlying buffer.
     * @return Stream& Reference to the current Stream for chain insertion. Returns a reference to *this.
     *
     ***********************************************/
    template<typename T, typename Allocator>
    Stream& operator >> (std::vector<T, Allocator> &Value);


    /********************************************//**
     * @brief Extraction operator for extracting the contents of the underlying buffer into an std::list.
     *
     * @param std::list<T, Allocator>& Value - An std::list in which to store the extracted data from the underlying buffer.
     * @return Stream& Reference to the current Stream for chain insertion. Returns a reference to *this.
     *
     ***********************************************/
    template<typename T, typename Allocator>
    Stream& operator >> (std::list<T, Allocator> &Value);


    /********************************************//**
     * @brief Extraction operator for extracting the contents of the underlying buffer into an std::basic_string.
     *
     * @param std::basic_string<T, Allocator>& Value - An std::basic_string in which to store the extracted data from the underlying buffer.
     * @return Stream& Reference to the current Stream for chain insertion. Returns a reference to *this.
     *
     ***********************************************/
    template<typename T, typename Allocator>
    Stream& operator >> (std::basic_string<T, Allocator> &Value);
};


template<typename T>
void Stream::Read(T &Value) {Data.read(reinterpret_cast<char*>(&Value), sizeof(T));}

template<typename T>
void Stream::Write(const T &Value) {Data.write(reinterpret_cast<const char*>(&Value), sizeof(T));}

template<typename T>
typename std::enable_if<!Stream::is_cstring<T>::value && !Stream::is_wcstring<T>::value, Stream&>::type Stream::operator << (const T &Value)
{
    Data.write(reinterpret_cast<const char*>(&Value), sizeof(T));
    return *this;
}

template<typename T>
typename std::enable_if<!Stream::is_cstring<T>::value && !Stream::is_wcstring<T>::value, Stream&>::type Stream::operator >> (T &Value)
{
    Data.read(reinterpret_cast<char*>(&Value), sizeof(T));
    return *this;
}



template<typename T, typename Allocator>
Stream& Stream::operator << (const std::vector<T, Allocator> &Value)
{
    *this << Value.size();
    for(auto it = Value.begin(); it != Value.end(); ++it)
    {
        *this << (*it);
    }
    return *this;
}

template<typename T, typename Allocator>
Stream& Stream::operator << (const std::list<T, Allocator> &Value)
{
    *this << Value.size();
    for(auto it = Value.begin(); it != Value.end(); ++it)
    {
        *this << (*it);
    }
    return *this;
}

template<typename T, typename Allocator>
Stream& Stream::operator << (const std::basic_string<T, Allocator> &Value)
{
    *this << Value.size();
    for(auto it = Value.begin(); it != Value.end(); ++it)
    {
        *this << (*it);
    }
    return *this;
}

template<typename T, typename Allocator>
Stream& Stream::operator >> (std::vector<T, Allocator> &Value)
{
    typename std::vector<T, Allocator>::size_type Size;
    *this >> Size;
    Value.reserve(Size);
    for(typename std::vector<T, Allocator>::size_type I = 0; I < Size; ++I)
    {
        T Temp;
        *this >> Temp;
        Value.emplace_back(Temp);
    }
    return *this;
}

template<typename T, typename Allocator>
Stream& Stream::operator >> (std::list<T, Allocator> &Value)
{
    typename std::list<T, Allocator>::size_type Size;
    *this >> Size;
    for(typename std::list<T, Allocator>::size_type I = 0; I < Size; ++I)
    {
        T Temp;
        *this >> Temp;
        Value.emplace_back(Temp);
    };
    return *this;
}

template<typename T, typename Allocator>
Stream& Stream::operator >> (std::basic_string<T, Allocator> &Value)
{
    typename std::string::size_type Size;
    *this >> Size;
    Value.reserve(Size);
    for(typename std::string::size_type I = 0; I < Size; ++I)
    {
        T Temp;
        *this >> Temp;
        Value += Temp;
    }
    return *this;
}

#endif // STREAM_HXX_INCLUDED

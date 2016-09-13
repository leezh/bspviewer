#include "filestream.hpp"

FileStream::FileStream(const std::string& path) :
    file(PHYSFS_openRead(path.c_str()))
{
}

FileStream::~FileStream()
{
    PHYSFS_close(file);
}

bool FileStream::isOpen()
{
    return (file != NULL);
}

sf::Int64 FileStream::read(void* data, sf::Int64 size)
{
    if (!file)
    {
        return 0;
    }
    PHYSFS_sint64 readBytes = PHYSFS_read(file, data, 1, size);
    if (readBytes < 0)
    {
        return 0;
    }
    return readBytes;
}

sf::Int64 FileStream::seek(sf::Int64 position)
{
    if (!file)
    {
        return -1;
    }
    if (!PHYSFS_seek(file, position))
    {
        return -1;
    }
    return position;
}

sf::Int64 FileStream::tell()
{
    if (!file)
    {
        return -1;
    }
    return PHYSFS_tell(file);
}

sf::Int64 FileStream::getSize()
{
    if (!file)
    {
        return -1;
    }
    return PHYSFS_fileLength(file);
}

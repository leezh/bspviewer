#include "filestream.hpp"
#include <physfs.h>

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

std::optional<std::size_t> FileStream::read(void* data, std::size_t size)
{
    if (!file)
    {
        return 0;
    }
    PHYSFS_sint64 readBytes = PHYSFS_readBytes(file, data, size);
    if (readBytes < 0)
    {
        return 0;
    }
    return readBytes;
}

std::optional<std::size_t> FileStream::seek(std::size_t position)
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

std::optional<std::size_t> FileStream::tell()
{
    if (!file)
    {
        return -1;
    }
    return PHYSFS_tell(file);
}

std::optional<std::size_t> FileStream::getSize()
{
    if (!file)
    {
        return -1;
    }
    return PHYSFS_fileLength(file);
}
